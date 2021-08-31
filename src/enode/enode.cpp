#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <csignal>
#include <fstream>
#include <iostream>

#include "cnfParse.h"
#include "msgdef.h"
#include "verStr.h"

using namespace std;

/*! \def RADIO_WAIT
 *  \brief Number of seconds to wait before launching MATLAB.  This time is spent launching uControl and initializing
 * the radio
 */
#define RADIO_WAIT 20

int sockfd;
int myId;
pid_t matPid, uCtrlPid;
char serverIp[64];
int serverPort;
cfgData_t cfg;

/**
 * Sends registration message to cnode
 *
 * Finds enode ip address, builds registration message and sends registration to control node (cnode)
 */
void txMsgEnodeReg() {
	ctrlMsg_t msg;
	cMsgEnodeReg_t *pload;
	struct sockaddr_in localAddr;
	socklen_t addrLen;

	// get client ipaddr
	addrLen = sizeof(struct sockaddr_in);
	getsockname(sockfd, (struct sockaddr *)&localAddr, &addrLen);

	// build reg message
	msg.hdr.type = MSG_ENODE_REG;
	pload = (cMsgEnodeReg_t *)msg.pload;
	strcpy(pload->ipaddr, inet_ntoa(localAddr.sin_addr));

	// send message
	cout << "<-- msgEnodeReg" << endl;
	write(sockfd, &msg, MSG_LEN(cMsgEnodeReg_t));
}

/**
 * Open TCP connection to the control node (cnode)
 *
 * Opens and connects to the control node over TCP
 */
void openSocket() {
	int r;
	struct sockaddr_in serv_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		cout << "socket() error\n";
		exit(1);
	}

	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(serverPort);
	inet_pton(AF_INET, serverIp, &serv_addr.sin_addr);

	r = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (r < 0) {
		cout << "connect error\n";
		exit(1);
	}
}

/**
 * Updates node ID from registration acknowledgement
 *
 * Extracts assigned node ID from return payload, configures edge node (enode) to use this, and prints receipt of
 * control node acknowledgement \param pload Received payload from control node of type cMsgEnodeRegAck_t
 */
void rxMsgEnodeRegAck(cMsgEnodeRegAck_t *pload) {
	// update node id
	myId = pload->enodeId;
	cout << "--> msgEnodeRegAck " << myId << endl;
}

/**
 * Sends acknowledgement of MATLAB execution
 *
 * Sends a message to the control node acknowleding execution of the MATLAB program.  Prints that it has done so.
 */
void txMsgEnodeRunAck() {
	ctrlMsg_t msg;
	cMsgEnodeRunAck_t *pload;

	// build ack message
	msg.hdr.type = MSG_ENODE_RUN_ACK;
	pload = (cMsgEnodeRunAck_t *)msg.pload;
	pload->enodeId = myId;

	// send message
	cout << "<-- msgEnodeRunAck" << endl;
	write(sockfd, &msg, MSG_LEN(cMsgEnodeRunAck_t));
}

void txMsgEnodeDlAck(int ret) {
	ctrlMsg_t msg;
	cMsgEnodeDlAck_t *pload;

	// build ack message
	msg.hdr.type = MSG_ENODE_DL_ACK;
	pload = (cMsgEnodeDlAck_t *)msg.pload;
	pload->ret = ret;

	// send message
	cout << "<-- msgEnodeDlAck" << endl;
	write(sockfd, &msg, MSG_LEN(cMsgEnodeDlAck_t));
}

int rxMsgEnodeDlInd(cMsgEnodeDlInd_t *pload) {
	char cnfFName[128] = "../mat/usrconfig.yml";
	char cmdBuf[256];
	int ret = -1;

	// user matlab code configuration parsing

	// file existence checking
	if (access(cnfFName, F_OK) == 0) {
		// read configuration
		if (usrCfgParse(cnfFName, &cfg) == 0) {
			// write logical ID file
			sprintf(cmdBuf, "echo %d > ../mat/%s/logicId", cfg.logicId, cfg.matDir);
			system(cmdBuf);
			ret = 0;
		}
	}

	txMsgEnodeDlAck(ret);

	return ret;
}

void txMsgEnodeRunCmpInd() {
	ctrlMsg_t msg;
	cMsgEnodeRunCmpInd_t *pload;

	// build ack message
	msg.hdr.type = MSG_ENODE_RUN_CMP_IND;
	pload = (cMsgEnodeRunCmpInd_t *)msg.pload;
	pload->enodeId = myId;

	// send message
	cout << "<-- msgEnodeRunCmpInd" << endl;
	write(sockfd, &msg, MSG_LEN(cMsgEnodeRunCmpInd_t));
}

void rxMsgEnodeRunReq(cMsgEnodeRunReq_t *pload) {
	char cbuf[2048];
	char logFn[1024];
	long start_time;

	// start_time
	start_time = pload->start_time;

	// remove previous log files
	system("rm -f ../log/*");

	// execut usrp control software
	uCtrlPid = fork();
	if (uCtrlPid == 0) {  // child
#ifdef SUDO_MODE
		sprintf(
		    cbuf,
		    "sudo ./uControl --args %s --opmode %s --nsamps %d --rate %f --subdev %s --freq %f --txgain %f --rxgain %f --bw %f --channels %s --ant %s &> ../conlog/usrpLog",
		    cfg.devAddr,
		    (cfg.opMode == OPMODE_TX_RX) ? "TX/RX"
		    : (cfg.opMode == OPMODE_RX)  ? "RX"
		                                 : "TX",
		    cfg.nsamps, cfg.rate, cfg.subdev, cfg.freq, cfg.txgain, cfg.rxgain, cfg.bw, cfg.channels, cfg.antennas);
#else
		sprintf(
		    cbuf,
		    "./uControl --args %s --opmode %s --nsamps %d --rate %f --subdev %s --freq %f --txgain %f --rxgain %f --bw %f --channels %s --ant %s &> ../conlog/usrpLog",
		    cfg.devAddr,
		    (cfg.opMode == OPMODE_TX_RX) ? "TX/RX"
		    : (cfg.opMode == OPMODE_RX)  ? "RX"
		                                 : "TX",
		    cfg.nsamps, cfg.rate, cfg.subdev, cfg.freq, cfg.txgain, cfg.rxgain, cfg.bw, cfg.channels, cfg.antennas);
#endif
		cout << "\n\n\n=====================================================\n";
		cout << " [enode] start of USRP Control\n";
		cout << "=====================================================\n";
		printf("%s\n", cbuf);
		system(cbuf);
		cout << "=====================================================\n";
		cout << " [enode] end of USRP Control\n";
		cout << "=====================================================\n";
		exit(1);
	}

	// idle loop
	{
		size_t count = 0;
		cout << "Waiting for uControl startup (Radio initialization)" << endl;
		while (1) {
			sleep(1);
			printf("%02ld/%d\n", count, RADIO_WAIT);
			if (count++ > (RADIO_WAIT - 1)) break;
		}
	}
	getcwd(logFn, sizeof(logFn));
	strcat(logFn, "/../conlog/matlab.log");

	// execute matlab software
	matPid = fork();
	if (matPid == 0) {
		if (cfg.lang == LANG_PYTHON) {
			cout << "=====================================================\n";
			cout << " [enode] start of Python\n";
			cout << "=====================================================\n";
			printf("start_time = %ld \n", start_time);
			sprintf(cbuf, "cd ../mat; mkdir -p ../log; cd %s; python %s.py %ld < /dev/null | tee %s", cfg.matDir,
			        cfg.mTopFile, start_time, logFn);
			printf("%s\n", cbuf);
			system(cbuf);

			cout << "=====================================================\n";
			cout << " [enode] end of Python\n";
			cout << "=====================================================\n";

			txMsgEnodeRunCmpInd();

			exit(1);

		} else if (cfg.lang == LANG_MATLAB) {
			cout << "=====================================================\n";
			cout << " [enode] start of MATLAB\n";
			cout << "=====================================================\n";
			printf("start_time = %ld \n", start_time);
			sprintf(cbuf, "cd ../mat; mkdir -p ../log; cd %s; matlab -nodisplay -r %s\\(%ld\\) < /dev/null | tee %s",
			        cfg.matDir, cfg.mTopFile, start_time, logFn);
			printf("%s\n", cbuf);
			system(cbuf);

			cout << "=====================================================\n";
			cout << " [enode] end of MATLAB\n";
			cout << "=====================================================\n";

			txMsgEnodeRunCmpInd();

			exit(1);
        } else if (cfg.lang == LANG_GNURADIO) {
            cout << "=====================================================\n";
			cout << " [enode] start of GNURadio\n";
			cout << "=====================================================\n";
			printf("start_time = %ld \n", start_time);
			sprintf(cbuf, "cd ../mat; mkdir -p ../log; cd %s; python %s.py --start-time %ld < /dev/null | tee %s", cfg.matDir,
			        cfg.mTopFile, start_time, logFn);
			printf("%s\n", cbuf);
			system(cbuf);

			cout << "=====================================================\n";
			cout << " [enode] end of GNURadio\n";
			cout << "=====================================================\n";

			txMsgEnodeRunCmpInd();

			exit(1);
		} else {
			printf("No valid runtime configuration!\n");
			exit(1);
		}
	}

	// idle loop
	{
		size_t count = 0;
		//        cout << "wait for MATLAB startup\n";
		while (1) {
			sleep(1);
			//            printf("%02d/5\n", count);
			if (count++ > 4) break;
		}
	}

	// send ack msg
	txMsgEnodeRunAck();
}

void rxMsgEnodeStopReq(cMsgEnodeStopReq_t *pload) {
	char cbuf[2048];

	sprintf(cbuf, "ps -A | grep MATLAB | awk '{print $1}' | xargs kill -9 > /dev/null");
	system(cbuf);
#ifdef SUDO_MODE
	sprintf(cbuf, "ps -A | grep uControl | awk '{print $1}' | sudo xargs kill -9 > /dev/null");
#else
	sprintf(cbuf, "ps -A | grep uControl | awk '{print $1}' | xargs kill -9 > /dev/null");
#endif
	system(cbuf);
}

void txMsgEnodeLogAck() {
	ctrlMsg_t msg;
	cMsgEnodeLogAck_t *pload;

	// build ack message
	msg.hdr.type = MSG_ENODE_LOG_ACK;
	pload = (cMsgEnodeLogAck_t *)msg.pload;
	pload->enodeId = myId;

	// send message
	cout << "<-- msgEnodeLogAck" << endl;
	write(sockfd, &msg, MSG_LEN(cMsgEnodeLogAck_t));
}

void rxMsgEnodeLogReq(cMsgEnodeLogReq_t *pload) {
	char cbuf[2048];

	// build, copy, and remove log file
	sprintf(cbuf, "cd ../log; rm -rf *; cp ../mat/%s/*.mat .; rm -f ../mat/%s/*.mat; tar cfz wisca_log.tgz *",
	        cfg.matDir, cfg.matDir);
	system(cbuf);

	// send ack msg
	txMsgEnodeLogAck();
}

void rxMsgEnodeTermReq(cMsgEnodeTermReq_t *pload) {
	system("ps -A | grep MATLAB | awk '{print $1}' | xargs kill -9 > /dev/null");
#ifdef SUDO_MODE
	system("ps -A | grep uControl | awk '{print $1}' | sudo xargs kill -9 > /dev/null");
#else
	system("ps -A | grep uControl | awk '{print $1}' | xargs kill -9 > /dev/null");
#endif
}

void mainLoop() {
	char rxbuf[1024];
	int size;
	ctrlMsg_t *ctrlMsg;

	ctrlMsg = (ctrlMsg_t *)rxbuf;
	while (1) {
		size = read(sockfd, rxbuf, 1024);
		if (size <= 0) return;
		switch (ctrlMsg->hdr.type) {
			case MSG_ENODE_REG_ACK:
				rxMsgEnodeRegAck((cMsgEnodeRegAck_t *)ctrlMsg->pload);
				break;
			case MSG_ENODE_DL:
				cout << "--> msgEnodeDlInd" << endl;
				rxMsgEnodeDlInd((cMsgEnodeDlInd_t *)ctrlMsg->pload);
				break;
			case MSG_ENODE_RUN:
				cout << "--> msgEnodeRunReq" << endl;
				rxMsgEnodeRunReq((cMsgEnodeRunReq_t *)ctrlMsg->pload);
				break;
			case MSG_ENODE_STOP:
				cout << "--> msgEnodeStopReq" << endl;
				rxMsgEnodeStopReq((cMsgEnodeStopReq_t *)ctrlMsg->pload);
				break;
			case MSG_ENODE_LOG:
				cout << "--> msgEnodeLogReq" << endl;
				rxMsgEnodeLogReq((cMsgEnodeLogReq_t *)ctrlMsg->pload);
				break;
			case MSG_ENODE_TERM:
				cout << "--> msgEnodeTermReq" << endl;
				rxMsgEnodeTermReq((cMsgEnodeTermReq_t *)ctrlMsg->pload);
				exit(1);
				break;
			default:
				cout << "--> Undefined msg" << endl;
				break;
		}
	}
}

int main() {
	char cnfFName[64] = "sysconfig.yml";

	cout << "\n\n";
	cout << "=================================================\n";
	cout << "=  Enode control software\n";
	cout << "=================================================\n";
	cout << "=  VERSION: " << verStr << "\n";
	cout << "=================================================\n";
	cout << endl;

	matPid = uCtrlPid = 0;

	// read configuration
	sysCfgParse(cnfFName, serverIp, &serverPort);

	openSocket();
	signal(SIGCHLD, SIG_IGN);

	txMsgEnodeReg();
	mainLoop();
}
