#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <chrono>
#include <csignal>
#include <future>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "cnfParse.h"
#include "msgdef.h"
#include "sysconf.h"
#include "verStr.h"

using namespace std;

/*! \def PORT
 *  \brief The port number cnode listens on for edge node connections
 */
#define PORT 9000
/*! \def BACKLOG
 *  \brief The maximum number of queued pending connections for cnode's sockets
 *
 *  The maximum number of queued pending connections for cnode's sockets.  If it grows above this number, any connecting
 * client may be rejected or have to retry
 */
#define BACKLOG 20
/*! \def STIME_MARGIN
 *  \brief The number of seconds given for all components to start up before the first MATLAB function may execute
 *
 * This margin is given from the time enodeRunReq kicked off.  It is baked in before it is sent to the enodes.  The
 * enodes then use that time to command MATLAB to start the programs execution at that time.
 */
#define STIME_MARGIN 35

char userName[128]; /*!< Contains the username that cnode is running as.  This is also expected to be the same as the
                      username enode is running as.  It is used to SSH to any enodes */
struct nodeInfo *sysConf; /*!< System configuration structure containing MAX_ENODE nodeInfos */
vector<int> fdList;       /*!< List of all file descriptions opened by the network controller */

int dlEnodeCount;
int dlEnodeAckCount;
int runEnodeCount;
int runEnodeCmpCount;
int logEnodeCount;
int logEnodeAckCount;

int ss, controlFd[2];

std::atomic<bool> shutdownFlag; /*!< Flag to shutdown network controller when exiting */

std::mutex sysconfMtx;  // Mutex to be held before and after reading/writing to sysConf

/**
 * Called before any exit, cleanly shutdowns any open sockets.
 *
 * Cleanly shutdowns opened sockets.  Iterates through the fdList and shuts down each socket, this prevents waiting for
 * the system to reap the sockets and allows the program to be properly restarted and recover from crashes
 */
void destroySockets() {
	int fd;

	while (fdList.size() != 0) {
		fd = fdList.back();
		fdList.pop_back();
		shutdown(fd, SHUT_RDWR);
		close(fd);
	}
}

/**
 * Called before any exit, frees all relevant memory.
 *
 * Cleanly frees all memory, prevents leaks. Frees sysconf and any other allocated structures.
 */
void cleanUpMemory() { free(sysConf); }

/**
 * Determine and return first unused node
 *
 * Iterates through sysConf and finds first unconnected node (state of 0)
 * \return sysConf index of first free node
 */
int getFreeNodeInfo() {
	int i;
	sysconfMtx.lock();
	for (i = 0; i < MAX_ENODE; i++) {
		if (sysConf[i].state == 0) {
			sysconfMtx.unlock();
			return i;
		}
	}
	sysconfMtx.unlock();

	return -1;
}
/**
 * Get next active node starting at curIdx
 *
 * \return sysConf index of next active ndoe
 */
int getActiveNodeIdx(int curIdx) {
	int i;
	sysconfMtx.lock();
	for (i = curIdx + 1; i < MAX_ENODE; i++) {
		if (sysConf[i].state == 1) {
			sysconfMtx.unlock();
			return i;
		}
	}
	sysconfMtx.unlock();
	return -1;
}

/**
 * Determine if node was previously connected, based on provided ip address
 *
 * \param ipaddr ipaddr of new node to compare against
 * \return sysConf index of previously connected node, -1 otherwise
 */
int checkPrevNodeHistory(char *ipaddr) {
	int i;
	sysconfMtx.lock();
	for (i = 0; i < MAX_ENODE; i++) {
		if (sysConf[i].state == 1) {
			if (strcmp(sysConf[i].ipaddr, ipaddr) == 0) {
				sysConf[i].opState = OP_IDLE;
				sysconfMtx.unlock();
				return i;
			}
		}
	}
	sysconfMtx.unlock();
	return -1;
}

/**
 * Print out information for all nodes
 */
void printNodeInfo() {
	int i;

	cout << "================== NODE INFO ====================\n";
	cout << setw(6) << "Node #"
	     << " : " << setw(3) << "FD #"
	     << " : " << setw(15) << left << "IP Address"
	     << " : " << setw(14) << "State" << endl;
	sysconfMtx.lock();
	for (i = 0; i < MAX_ENODE; i++) {
		if (sysConf[i].state) {
			cout << setw(6) << i << " : " << setw(4) << sysConf[i].sockFd << " : " << setw(15) << left
			     << sysConf[i].ipaddr << " : " << setw(14) << sysConf[i].opState << endl;
		}
	}
	sysconfMtx.unlock();
	cout << endl;
}

/**
 * Sends Acknowledgement message to an ENode
 *
 * Sends acknowledgement payload to the specific enode
 * \param nodeID ID Number of Enode to send to
 * \param sock Control socket fd
 */
void txMsgEnodeRegAck(int sock, int nodeId) {
	ctrlMsg_t msg;
	cMsgEnodeRegAck_t *pload;

	msg.hdr.type = MSG_ENODE_REG_ACK;
	pload = (cMsgEnodeRegAck_t *)msg.pload;
	pload->enodeId = nodeId;

	write(sock, &msg, MSG_LEN(cMsgEnodeRegAck_t));
}

/**
 * Handles enode registration message
 *
 * When a registration is received, a nodeInfo structure is populated in sysConf with its parameters.
 * Log directories are also created if they do not already exist
 * If an enode was previously connected to this cnode, it will recognize and reconnect
 * \param sock
 * \param pload
 * \param size
 */
int rxMsgEnodeReg(int sock, cMsgEnodeReg_t *pload, int size) {
	int nodeId;
	char command[512];
	int recFlag = 0;

	cout << "\n<-- msgEnodeReg (" << pload->ipaddr << ")" << endl;

	// check previous history
	if ((nodeId = checkPrevNodeHistory(pload->ipaddr)) != -1) {
		sysconfMtx.lock();
		printf("reconnected node (%s)\n", sysConf[nodeId].ipaddr);
		sysconfMtx.unlock();
		recFlag = 1;
	}

	// update node database
	if (recFlag == 0) {
		nodeId = getFreeNodeInfo();
		if (nodeId == -1) {
			cout << "No more space for nodeInfo\n";
			destroySockets();
			cleanUpMemory();
			return -ENOMEM;
		}
		sysconfMtx.lock();
		sysConf[nodeId].state = 1;
		sysConf[nodeId].sockFd = sock;
		strcpy(sysConf[nodeId].ipaddr, pload->ipaddr);
		sysConf[nodeId].usrFlag = 0;
		sysConf[nodeId].opState = OP_IDLE;
		sysconfMtx.unlock();
		// print updated node info
		// printNodeInfo();

		// create log directories
		sprintf(command, "mkdir -p ../log/enode_%d", nodeId);
		system(command);
	} else {
		sysconfMtx.lock();
		sysConf[nodeId].sockFd = sock;
		sysconfMtx.unlock();
	}

	// send acknowledge
	txMsgEnodeRegAck(sock, nodeId);
	return 0;
}

/**
 * Received execution acknowledgment from edge nodes
 *
 * Updates sysConf state for the enode, acknowledge that it ran the program, prints message on cnode
 *
 * \param pload payload received from edge node
 */
void rxMsgEnodeRunAck(int sock, cMsgEnodeRunAck_t *pload, int size) {
	int nodeId;

	cout << "\n<-- msgEnodeRunAck\n";

	// update node database
	nodeId = pload->enodeId;
	sysconfMtx.lock();
	sysConf[nodeId].opState = OP_RUN;
	sysconfMtx.unlock();

	// print updated node info
	// printNodeInfo();
}

/*
 * Receive completion message from edge node
 *
 * Updates sysConf state for the enode, acknowledge that it has completed running, prints message on cnode
 *
 * \param pload payload recieved from edge node
 */
void rxMsgEnodeRunCmpInd(int sock, cMsgEnodeRunCmpInd_t *pload, int size) {
	int nodeId;

	cout << "\n<-- msgEnodeRunCmpInd\n";

	// update node database
	nodeId = pload->enodeId;
	sysconfMtx.lock();
	sysConf[nodeId].opState = OP_CMP;
	sysconfMtx.unlock();

	runEnodeCmpCount++;
}

/*
 * Receive log creation acknowledgment and retrieve and unpack files from edge node
 *
 * Receives log tarfile ack from edge nodes, retrieves tar file from edge node, unpacks tar from edge node and then
 * makes a backup and handles log file history
 *
 * \param sock Receiving socket id
 * \param pload Payload received from edge node
 * \param size Payload size (in bytes)
 */
void rxMsgEnodeLogAck(int sock, cMsgEnodeLogAck_t *pload, int size) {
	int nodeId;
	char tbuf[128];
	char cbuf[512];
	time_t now = time(0);

	nodeId = pload->enodeId;
	printf("\n<-- msgEnodeLogAck (%d)\n", nodeId);

	// log copy
	sysconfMtx.lock();
	sprintf(cbuf, "mkdir -p ../log/enode_%d; cd ../log/enode_%d; scp %s@%s:wdemo/run/enode/log/wisca_log.tgz .", nodeId,
	        nodeId, userName, sysConf[nodeId].ipaddr);
	system(cbuf);
	sprintf(cbuf, "ssh %s@%s 'rm -f wdemo/run/enode/log/*'", userName, sysConf[nodeId].ipaddr);
	system(cbuf);
	sysconfMtx.unlock();
	// decompress tar file
	sprintf(cbuf, "cd ../log/enode_%d; tar xfz wisca_log.tgz; rm wisca_log.tgz", nodeId);
	system(cbuf);

	logEnodeAckCount++;

	// backup data log
	if (logEnodeAckCount == logEnodeCount) {
		strftime(tbuf, 128, "%Y_%m_%d_%H_%M_%S", localtime(&now));
		sprintf(cbuf, "mkdir -p ../log_history/log_%s; cp -R ../log/* ../log_history/log_%s", tbuf, tbuf);
		system(cbuf);
	}
}

/*
 * Send download indicator to edge node
 *
 * Sends downloaded configuration and MATLAB files downloaded indicator to edge nodes. *
 */
void txEnodeDlInd() {
	int cIdx = -1;
	ctrlMsg_t msg;
	int sock;

	msg.hdr.type = MSG_ENODE_DL;

	while ((cIdx = getActiveNodeIdx(cIdx)) != -1) {
		sysconfMtx.lock();
		if (sysConf[cIdx].usrFlag == 1) {
			printf("--> enodeDlInd to enode %d\n", cIdx);
			sock = sysConf[cIdx].sockFd;
			write(sock, &msg, MSG_LEN(cMsgEnodeDlInd_t));
		}
		sysconfMtx.unlock();
	}
}

/*
 * Received download acknowledgment from edge node
 *
 * \param sock Receiving socket id
 * \param pload Payload received from edge node
 * \param size Payload size (in bytes)
 */
void rxMsgEnodeDlAck(int sock, cMsgEnodeDlAck_t *pload, int size) {
	cout << "<-- enodeDlAck\n";

	if (pload->ret == 0) dlEnodeAckCount++;
}

/*
 * Wait for download acknowledgment from all edge nodes
 *
 * Timeout for 50ms in between checking if all of the edge nodes have reported in that they have received their
 * downloaded files
 */
int waitDlAck() {
	for (size_t i = 0; i < 4; i++) {
		usleep(500000);
		if (dlEnodeAckCount >= dlEnodeCount) return 0;
	}

	return -1;
}

/*
 * Download MATLAB program files to edge nodes
 *
 * Iterate over all edge nodes, and download selected MATLAB files to them via tar/scp
 */
int downloadMatFiles() {
	int cIdx = -1;
	char cbuf[1024];
	char cnfFName[128];
	char ibuf[128];
	FILE *fp;
	int matchFlag = 0;
	char ipaddr[128];
	cfgData_t cfg;

	fp = fopen("../../usr/cfg/iplist", "r"); //TODO -  make this robust and actually use the current user's home path.
    //TODO - add a configuration file, and make it have configurable file locations
	if (fp == NULL) {
		printf("failed to open usr/cfg/iplist\n");
		return -1;
	}

	while ((cIdx = getActiveNodeIdx(cIdx)) != -1) {
		sysconfMtx.lock();
		sysConf[cIdx].usrFlag = 0;
		sysconfMtx.unlock();
	}

	dlEnodeCount = 0;
	dlEnodeAckCount = 0;
	while (fgets(ibuf, 128, fp) != NULL) {
		cIdx = -1;
		matchFlag = 0;
		sscanf(ibuf, "%s", ipaddr);

		while ((cIdx = getActiveNodeIdx(cIdx)) != -1) {
			sysconfMtx.lock();
			if (strcmp(sysConf[cIdx].ipaddr, ipaddr) == 0) {
				matchFlag = 1;
				sysConf[cIdx].usrFlag = 1;
				sysconfMtx.unlock();
				break;
			}
			sysconfMtx.unlock();
		}
		if (matchFlag == 0) {
			printf("Err : Node %s is not connected\n", ipaddr);
			return -1;
		}

		// parse config file
		sysconfMtx.lock();
        //TODO - same thing here, make it look in a config directory, or users home path, rather than something relative
		sprintf(cnfFName, "../../usr/cfg/usrconfig_%s.yml", sysConf[cIdx].ipaddr);
		sysconfMtx.unlock();
		printf("cnfFName = %s\n", cnfFName);
		if (access(cnfFName, F_OK) == -1) continue;  // skip if configuration does not exist.

		// parse config file
		usrCfgParse(cnfFName, &cfg);
		sysconfMtx.lock();
		strcpy(sysConf[cIdx].matDir, cfg.matDir);
		strcpy(sysConf[cIdx].mLogFile, cfg.mLogFile);
		sysconfMtx.unlock();

		// config file copy
		sysconfMtx.lock();
		sprintf(cbuf, "cp ../../usr/cfg/usrconfig_%s.yml ../../usr/mat/usrconfig.yml", sysConf[cIdx].ipaddr);
		sysconfMtx.unlock();
		system(cbuf);

		// install matlab file
		sysconfMtx.lock();
		sprintf(cbuf, "cd ../../usr; tar cfz umat.tgz mat; scp umat.tgz %s@%s:wdemo/run/enode", userName,
		        sysConf[cIdx].ipaddr);
		system(cbuf);
		sprintf(cbuf, "ssh %s@%s 'cd wdemo/run/enode; tar xfz umat.tgz; rm umat.tgz'", userName, sysConf[cIdx].ipaddr);
		system(cbuf);
		sysconfMtx.unlock();
		remove("../../usr/umat.tgz");

		dlEnodeCount++;
	}
	printf("\n");

	fclose(fp);

	txEnodeDlInd();

	/*
	   if(waitDlAck() == -1) return -1;
	   else return 0;
	   */
	return 0;
}

int enodeRunReq() {
	ctrlMsg_t msg;
	cMsgEnodeRunReq_t *pload;
	struct timespec spec;
	time_t start_time;
	struct tm lt;
	int cIdx = -1;
	int sock;
	char res[128];

	msg.hdr.type = MSG_ENODE_RUN;

	pload = (cMsgEnodeRunReq_t *)msg.pload;

	clock_gettime(CLOCK_REALTIME, &spec);
	start_time = spec.tv_sec + STIME_MARGIN;

	localtime_r(&start_time, &lt);
	strftime(res, sizeof(res), "%S:%M:%H", &lt);

	runEnodeCmpCount = 0;
	runEnodeCount = 0;

	while ((cIdx = getActiveNodeIdx(cIdx)) != -1) {
		sysconfMtx.lock();
		if (sysConf[cIdx].usrFlag == 1) {
			printf("node start time = %ld, %s\n", start_time, res);
			pload->start_time = (double)start_time;
			printf("--> enodeRunReq (node-%d)(%s)(%f)\n", cIdx, sysConf[cIdx].ipaddr, pload->start_time);
			sock = sysConf[cIdx].sockFd;
			write(sock, &msg, MSG_LEN(cMsgEnodeRunReq_t));
			sysConf[cIdx].opState = OP_START;
			runEnodeCount++;
		}
		sysconfMtx.unlock();
	}

	return 0;
}

/*
 * Send stop execution request to edge nodes
 *
 * Send stop execution request to edge nodes, will attempt to kill uControl and MATLAB (user program)
 */
int enodeStopReq() {
	ctrlMsg_t msg;
	int cIdx = -1;
	int sock;

	msg.hdr.type = MSG_ENODE_STOP;

	while ((cIdx = getActiveNodeIdx(cIdx)) != -1) {
		sysconfMtx.lock();
		printf("--> enodeStopReq (node-%d)(%s)\n", cIdx, sysConf[cIdx].ipaddr);
		sock = sysConf[cIdx].sockFd;
		sysconfMtx.unlock();
		write(sock, &msg, MSG_LEN(cMsgEnodeStopReq_t));
	}

	return 0;
}

int enodeLogReq() {
	ctrlMsg_t msg;
	int cIdx = -1;
	int sock;
	char cbuf[1024];

	msg.hdr.type = MSG_ENODE_LOG;

	logEnodeCount = 0;
	logEnodeAckCount = 0;
	while ((cIdx = getActiveNodeIdx(cIdx)) != -1) {
		sysconfMtx.lock();
		if (sysConf[cIdx].usrFlag == 1) {
			sprintf(cbuf, "cd ../log/enode_%d; rm -f *", cIdx);
			system(cbuf);
			printf("--> enodeLogReq (node-%d)(%s)\n", cIdx, sysConf[cIdx].ipaddr);
			sock = sysConf[cIdx].sockFd;
			write(sock, &msg, MSG_LEN(cMsgEnodeLogReq_t));
			logEnodeCount++;
		}
		sysconfMtx.unlock();
	}

	return 0;
}

int logAnalysis() {
	char cbuf[1024];
	int cIdx = -1;
	FILE *fp;

	// top analysis matlab script generation
	fp = fopen("../log/logAnalysis.m", "w");
	if (fp == NULL) {
		printf("Failed to open logAnalysis.m\n");
		destroySockets();
		cleanUpMemory();
		return -ENOENT;
	}
	while ((cIdx = getActiveNodeIdx(cIdx)) != -1) {
		sysconfMtx.lock();
		if (sysConf[cIdx].usrFlag == 1) {
			if (strcmp("NULL", sysConf[cIdx].mLogFile) == 0) continue;
			fprintf(fp, "cd enode_%d;\n", cIdx);
			fprintf(fp, "%s;\n", sysConf[cIdx].mLogFile);
			fprintf(fp, "cd ..;\n");
		}
		sysconfMtx.unlock();
	}
	fclose(fp);

	// analysis matlab file copy
	while ((cIdx = getActiveNodeIdx(cIdx)) != -1) {
		sysconfMtx.lock();
		if (sysConf[cIdx].usrFlag == 1) {
			if (strcmp("NULL", sysConf[cIdx].mLogFile) == 0) continue;
			sprintf(cbuf, "cp ../../usr/mat/%s/%s.m ../log/enode_%d", sysConf[cIdx].matDir, sysConf[cIdx].mLogFile,
			        cIdx);
			system(cbuf);
		}
		sysconfMtx.unlock();
	}

	// perfrom analysis with log files
	sprintf(cbuf, "cd ../log; matlab -r logAnalysis");
	system(cbuf);
	return 0;
}
/*
 * Send termination request to edge node
 *
 * Send termination request to edge node.  Edge node will close connections and exit
 */
void enodeTermReq() {
	ctrlMsg_t msg;
	int cIdx = -1;
	int sock;

	msg.hdr.type = MSG_ENODE_TERM;

	while ((cIdx = getActiveNodeIdx(cIdx)) != -1) {
		sysconfMtx.lock();
		printf("--> enodeTermReq (node-%d)(%s)\n", cIdx, sysConf[cIdx].ipaddr);
		sock = sysConf[cIdx].sockFd;
		write(sock, &msg, MSG_LEN(cMsgEnodeTermReq_t));
		sysconfMtx.unlock();
	}
}

void remoteEnodeStop() {
	FILE *fp;
	char ibuf[128], ipaddr[128];
	char cmdBuf[1024];

	// read iplist file
	fp = fopen("../../usr/cfg/iplist", "r");
	if (fp == NULL) {
		printf("failed to open usr/cfg/iplist\n");
		return;
	}

	// remote logic and execute enode program
	while (fgets(ibuf, 128, fp) != NULL) {
		sscanf(ibuf, "%s", ipaddr);
		sprintf(cmdBuf, "ssh %s@%s 'cd wdemo/run/enode/tools; appkill %s; appkill %s; appkill %s'", userName, ipaddr,
		        "enode", "MATLAB", "uControl");
		printf("%s\n", cmdBuf);
		system(cmdBuf);
	}
}

void remoteEnodeStart() {
	FILE *fp;
	char ibuf[128], ipaddr[128];
	char cmdBuf[1024];

	// read iplist file
	fp = fopen("../../usr/cfg/iplist", "r");
	if (fp == NULL) {
		printf("failed to open usr/cfg/iplist\n");
		return;
	}

	// remote logic and execute enode program
	while (fgets(ibuf, 128, fp) != NULL) {
		sscanf(ibuf, "%s", ipaddr);
		sprintf(cmdBuf, "ssh %s@%s 'cd wdemo/run/enode/bin; nohup ./enode  &> ../conlog/enode.log < /dev/null &'",
		        userName, ipaddr);
		printf("%s\n", cmdBuf);
		system(cmdBuf);
	}
}

int dlNExecution() {
	cout << "-- download and execution\n" << endl;

	downloadMatFiles();
	enodeRunReq();

	if (waitDlAck() < 0) {
		printf("Download error \n");
		return -1;
	}

	return 0;
}

int waitLogAck() {
	for (size_t i = 0; i < 5; i++) {
		usleep(500000);
		if (logEnodeAckCount >= logEnodeCount) return 0;
	}

	return -1;
}

int logColNAnal() {
	if (enodeStopReq() == -1) return -1;
	if (enodeLogReq() == -1) return -1;
	if (waitLogAck() == -1) return -1;
	logAnalysis();

	return 0;
}

void rxMsgUsrCmd(int sock, cMsgUsrCmd_t *pload, int size) {
	printf("<-- usrCmd : %d\n", pload->usrCmd);

	switch (pload->usrCmd) {
		case CMD_PRT_STATUS:
			break;
		case CMD_DL_MATLAB:
			downloadMatFiles();
			break;
		case CMD_EX_MATLAB:
			enodeRunReq();
			break;
		case CMD_STOP_MATLAB:
			enodeStopReq();
			break;
		case CMD_CLT_LOG:
			enodeLogReq();
			break;
		case CMD_LOG_ANAL:
			logAnalysis();
			break;
			/*
			 * TODO: Is this case ever hit?
			 * This case is not enumerated in the struct, it may not ever exist, it triggers a warning
			 */
		case (CMD_DL_MATLAB | CMD_EX_MATLAB):
			dlNExecution();
			break;
		default:
			printf("unknown command\n");
	}
}

int nodeControl(int s) {
	int size, ret_code;
	char buffer[2048];
	ctrlMsg_t *ctrlMsg;

	ctrlMsg = (ctrlMsg_t *)buffer;
	size = read(s, buffer, 1024);
	if (size <= 0) return -1;
	switch (ctrlMsg->hdr.type) {
		case MSG_ENODE_REG:
			ret_code = rxMsgEnodeReg(s, (cMsgEnodeReg_t *)(ctrlMsg->pload), size);
			if (ret_code < 0) {
				printf("Stubbed handling for failed registration (Return code: %d)\r\n", ret_code);
			}
			break;
		case MSG_ENODE_DL_ACK:
			rxMsgEnodeDlAck(s, (cMsgEnodeDlAck_t *)(ctrlMsg->pload), size);
			break;
		case MSG_ENODE_RUN_ACK:
			rxMsgEnodeRunAck(s, (cMsgEnodeRunAck_t *)(ctrlMsg->pload), size);
			break;
		case MSG_ENODE_RUN_CMP_IND:
			rxMsgEnodeRunCmpInd(s, (cMsgEnodeRunCmpInd_t *)(ctrlMsg->pload), size);
			break;
		case MSG_ENODE_LOG_ACK:
			rxMsgEnodeLogAck(s, (cMsgEnodeLogAck_t *)(ctrlMsg->pload), size);
			break;
		case MSG_USR_CMD:
			rxMsgUsrCmd(s, (cMsgUsrCmd_t *)(ctrlMsg->pload), size);
			break;
		default:
			printf("unknown message : %X %X %X %X\n", buffer[0], buffer[1], buffer[2], buffer[3]);
			break;
	}
	cout << "wiscanet> ";
	return size;
}

void controllerStart() {
	int i;
	int sc;
	struct sockaddr_in client_addr;
	socklen_t addrlen;
	fd_set active_fd_set, read_fd_set;

	FD_ZERO(&active_fd_set);
	FD_SET(ss, &active_fd_set);
	FD_SET(controlFd[0], &active_fd_set);
	addrlen = sizeof(client_addr);
	while (!shutdownFlag) {
		read_fd_set = active_fd_set;
		int ret_code = select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL);
		if (ret_code < 0) {
			printf("Select error: %d\r\n", ret_code);
			break;
		}
		for (i = 0; i < FD_SETSIZE; ++i) {
			if (FD_ISSET(i, &read_fd_set)) {
				if (i == ss) {  // server
					sc = accept(ss, (struct sockaddr *)&client_addr, (socklen_t *)&addrlen);
					fdList.push_back(sc);
					FD_SET(sc, &active_fd_set);
				} else if (i == controlFd[0]) {
					break;
				} else {  // client
					if (nodeControl(i) < 0) {
						close(i);
						FD_CLR(i, &active_fd_set);
					}
				}
			}
		}
	}
	cout << "Network controller has shutdown" << endl;
}

void controllerInit() {
	int i, r;
	struct sockaddr_in server_addr;

	// state information init
	sysconfMtx.lock();
	sysConf = (struct nodeInfo *)malloc(MAX_ENODE * sizeof(struct nodeInfo));
	for (i = 0; i < MAX_ENODE; i++) {
		sysConf[i].state = NODE_FREE;
	}
	sysconfMtx.unlock();

	// socket init
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);

	ss = socket(AF_INET, SOCK_STREAM, 0);
	if (ss < 0) {
		printf("Socket open error: %d\r\n", ss);
		destroySockets();
		cleanUpMemory();
		exit(ss);
	}
	fdList.push_back(ss);

	r = bind(ss, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (r < 0) {
		printf("Bind error: %d\r\n", r);
		destroySockets();
		cleanUpMemory();
		exit(r);
	}

	r = listen(ss, BACKLOG);
	if (r < 0) {
		printf("Listen error: %d\r\n", r);
		destroySockets();
		cleanUpMemory();
		exit(r);
	}
}

/**
 * Gets the username that cnode is running as
 *
 * This username is expected to be the same on all cnode and enodes, and has passwordless SSH enabled between nodes for
 * it. Typically it used to store the username in the global character array userName of length 128.
 *
 * @param userName The username is stored in this character array.
 */
void getUserName(char *userName) {
	struct passwd *pass;
	pass = getpwuid(getuid());
	strcpy(userName, pass->pw_name);
}

/**
 * Main function, starts the cnode
 *
 * Prints the main banner, and then spins off the network controller thread.
 * The socket (network) controller than handles accepting enode connections
 * Also runs the graphical interface once the network controller is started
 * which displays the main TUI (terminal user interface) options.
 * Handles the users input, and in turn sends commands to the network controller thread to execute commands and handle
 * enode responses
 * \return 0 for success, or any error codes surfaced in underlying components
 */
int main() {
	// Get username we are runningas (uid)
	getUserName(userName);
	// Set thread shutdown flag to false
	shutdownFlag = false;
	if (pipe(controlFd) < 0) {
		printf("Error opening thread control pipe");
		exit(1);
	}
	// Clear the screen and we're off!
	system("clear");
	cout << "=================================================" << endl;
	cout << "=                  WISCA SDR-Network            =" << endl;
	cout << "=================================================" << endl;
	cout << "=                  VERSION: " << verStr << "               =" << endl;

	controllerInit();
	// Launch network controller thread
	thread controllerThread(controllerStart);
	// Start graphical interface
	int choice, ret_code;
	string buffer = "";

	while (1) {
		cout << "=================================================\n";
		cout << " a. Remote Control Menu" << endl;
		cout << " 1. Print enode status" << endl;
		cout << " 2. Download user MATLAB code" << endl;
		cout << " 3. Execute MATLAB code" << endl;
		cout << " 4. Stop MATLAB code" << endl;
		cout << " 5. Collect log" << endl;
		cout << " 6. Log analysis" << endl;
		cout << " 0. Exit" << endl;
		cout << "=================================================" << endl << endl;
		cout << "wiscanet> ";

		getline(cin, buffer);
		cout << endl;
		if (buffer.size() == 0) continue;

		system("clear");

		if (buffer[0] == 'a') {
			cout << "=================================================" << endl;
			cout << " 1. remote enode start" << endl;
			cout << " 2. remote enode stop" << endl;
			cout << " 3. matlab code download and start" << endl;
			cout << " 4. stop, log collection and log analysis" << endl;
			cout << "=================================================" << endl << endl;
			cout << "wiscanet> ";

			getline(cin, buffer);
			cout << endl;
			if (buffer.size() == 0) continue;

			system("clear");

			choice = atoi(buffer.c_str());
			switch (choice) {
				case 1:
					remoteEnodeStart();
					break;
				case 2:
					enodeTermReq();
					break;
				case 3:
					dlNExecution();
					break;
				case 4:
					logColNAnal();
				default:
					break;
			}
		} else {
			choice = atoi(buffer.c_str());
			switch (choice) {
				case 1:
					printNodeInfo();
					break;
				case 2:
					downloadMatFiles();
					break;
				case 3:
					enodeRunReq();
					break;
				case 4:
					enodeStopReq();
					break;
				case 5:
					enodeLogReq();
					break;
				case 6:
					ret_code = logAnalysis();
					if (ret_code < 0) {
						return ret_code;
					}
					break;
				case 0:
					enodeTermReq();
					cout << endl;
					cout << "WISCANet is shutting down..." << endl;
					sleep(1);
					// Set atomic shutdown flag for controller thread
					shutdownFlag = true;
					write(controlFd[1], 0, 1);  // Write a byte to the controlFd to trigger the end of the system
					controllerThread.join();    // Wait for controller thread to shutdown and join
					destroySockets();           // Destroy sockets used
					cleanUpMemory();            // Free all remaining memory
					cout << "Shutdown complete." << endl;
					cout << "=================================================" << endl;
					return 0;
				default:
					break;
			}
		}
	}
}
