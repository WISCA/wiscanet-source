#include <arpa/inet.h>
#include <matrix.h>
#include <mex.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <tmwtypes.h>
#include <unistd.h>

#define CBLEN 32
#define DBLEN (2000 * 8)

int rxSockfd = -1;
int rxCnSockfd = -1;
int txSockfd = -1;
struct sockaddr_in si_other, si_me, si_rcon;

void txInit(char *ipaddr, int portNum) {
	if ((txSockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		mexPrintf("socket() error\n");
		return;
	}

	memset((char *)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(portNum);
	if (inet_aton(ipaddr, (struct in_addr *)&si_other.sin_addr) == 0) {
		mexPrintf("inet_aton() failed\n");
		return;
	}
}

void rxInit(char *ipaddr, int portNum, int cportNum) {
	if ((rxSockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		mexPrintf("socket() error\n");
		return;
	}

	memset((char *)&si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(portNum);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(rxSockfd, (struct sockaddr *)&si_me, sizeof(si_me)) == -1) {
		mexPrintf("bind() failed\n");
		return;
	}

	if ((rxCnSockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		mexPrintf("rx control socket() error\n");
		return;
	}

	memset((char *)&si_rcon, 0, sizeof(si_rcon));
	si_rcon.sin_family = AF_INET;
	si_rcon.sin_port = htons(cportNum);
	if (inet_aton(ipaddr, (struct in_addr *)&si_rcon.sin_addr) == 0) {
		mexPrintf("inet_aton() failed\n");
		return;
	}
}

void sendPacket(double start_time, char *buf, int len, size_t numChans, int16_t refPower) {
	int totalTx = 0, txLen, txUnit = 4095, txResult = -1;
	struct timespec spec;
	double ctime;
	useconds_t stime;

	while (totalTx < len) {
		if ((len - totalTx) > txUnit)
			txLen = txUnit;
		else
			txLen = len - totalTx;
		txResult = sendto(txSockfd, (buf + totalTx), txLen, 0, (struct sockaddr *)&si_other, sizeof(si_other));
		if (txResult == -1) break;
		totalTx += txResult;
	}
	sendto(txSockfd, (char *)&start_time, sizeof(double), 0, (struct sockaddr *)&si_other, sizeof(si_other));
	totalTx += sizeof(double);
	sendto(txSockfd, &numChans, sizeof(numChans), 0, (struct sockaddr *)&si_other, sizeof(si_other));
	totalTx += sizeof(uint16_t);
	sendto(txSockfd, &refPower, sizeof(refPower), 0, (struct sockaddr *)&si_other, sizeof(si_other));
	totalTx += sizeof(int16_t);
	sendto(txSockfd, buf, 0, 0, (struct sockaddr *)&si_other, sizeof(si_other));

	/* wait tx completion */
	if (start_time != 0.0) {
		clock_gettime(CLOCK_REALTIME, &spec);
		ctime = (double)(spec.tv_sec) + (double)(spec.tv_nsec / 1E9);
		stime = (useconds_t)((start_time - ctime) * 1E6) + 200000;
		usleep(stime);
	}

	mexPrintf("[Local USRP Mex] Sent %d bytes for %d channels, at %f\n", totalTx, numChans, start_time);
}

void controlRecv(double start_time, uint16_t numChans) {
	/* send control packet */
	sendto(rxCnSockfd, (char *)&start_time, 16, 0, (struct sockaddr *)&si_rcon, sizeof(si_rcon));
	sendto(rxCnSockfd, &numChans, sizeof(numChans), 0, (struct sockaddr *)&si_rcon, sizeof(si_rcon));
}

int recvPacket(char *buf, int len) {
	int retval = 0;
	int slen;

	unsigned int buf_pos = 0;
	unsigned int rxunit = 4000 * 2;
	while (1) {
		unsigned int readlen = len - buf_pos;

		if (readlen > 0) {
			retval = recvfrom(rxSockfd, (buf + buf_pos), rxunit, 0, (struct sockaddr *)&si_me, (socklen_t *)&slen);

			if (retval == 0) {
				mexPrintf("[Local USRP Mex] Completed one receiving cycle\n");
			}
			if (retval < 0) {
				perror("recvfrom() or recv()");
				mexPrintf("[Local USRP Mex] Receive error: %d\n", retval);
				break;
			}
		}

		readlen = retval > 0 ? retval : 0;
		buf_pos += readlen;

		if (buf_pos >= len) break;
	}
	return buf_pos;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
	char cmd[CBLEN];
	char ipaddr[CBLEN];
	int portNum;
	int cportNum;
	short *sBuf;
	int len;
	const int *dim_array;
	mwSize dims[1];
	int n;
	double start_time;

	if (nrhs < 1) {
		mexPrintf("[Local USRP Mex] Need more input arguments\n");
		return;
	}

	mxGetString(prhs[0], cmd, CBLEN);

	if (!strcmp("txinit", cmd)) {
		mexPrintf("[Local USRP Mex] Initializing Transmit operation\n");
		mxGetString(prhs[1], ipaddr, CBLEN);
		portNum = mxGetScalar(prhs[2]);
		mexPrintf("%s\n", ipaddr);
		mexPrintf("%d\n", portNum);
		txInit(ipaddr, portNum);
		if (txSockfd <= 0) {
			mexErrMsgTxt("Socket open error\n");
			txSockfd = -1;
		}
		plhs[0] = mxCreateDoubleScalar(txSockfd);
		nlhs = 1;
	} else if (!strcmp("rxinit", cmd)) {
		mexPrintf("[Local USRP Mex] Initializing Receive operation\n");
		mxGetString(prhs[1], ipaddr, CBLEN);
		portNum = mxGetScalar(prhs[2]);
		cportNum = mxGetScalar(prhs[3]);
		mexPrintf("%s\n", ipaddr);
		mexPrintf("%d, %d\n", portNum, cportNum);
		rxInit(ipaddr, portNum, cportNum);
		if (rxSockfd <= 0) {
			mexErrMsgTxt("Socket open error\n");
			rxSockfd = -1;
		}
		plhs[0] = mxCreateDoubleScalar(rxSockfd);
		nlhs = 1;

	} else if (!strcmp("write", cmd)) {
		if (txSockfd <= 0) {
			mexPrintf("socket is not ready\n");
			return;
		}
		dim_array = mxGetDimensions(prhs[1]);
		len = dim_array[2] * sizeof(float);
		sBuf = (short *)mxGetData(prhs[1]);
		start_time = mxGetScalar(prhs[2]);
		size_t numChans = mxGetScalar(prhs[3]);
		int16_t refPower = mxGetScalar(prhs[4]);
		mexPrintf("[Local USRP Mex] Sending data to USRP Control for transmission at %f,for %d channels, with reference power: %d dBm\n", start_time,
		          numChans, refPower);
		sendPacket(start_time, (char *)sBuf, len, numChans, refPower);
	} else if (!strcmp("read", cmd)) {
		if (rxCnSockfd <= 0) {
			mexPrintf("rx control socket is not ready\n");
			return;
		}

		if (rxSockfd <= 0) {
			mexPrintf("rx socket is not ready\n");
			return;
		}

		len = mxGetScalar(prhs[1]);

		start_time = mxGetScalar(prhs[2]);
		uint16_t numChans = mxGetScalar(prhs[3]);
		mexPrintf("[Local USRP Mex] Initializing USRP Control Read at %f, for %d channels\n", start_time, numChans);

		dims[0] = len * numChans;
		plhs[1] = mxCreateNumericArray(1, dims, mxINT16_CLASS, mxREAL);
		sBuf = (short *)mxGetPr(plhs[1]);

		controlRecv(start_time, numChans);
		n = recvPacket((char *)sBuf, numChans * len * sizeof(short));
		plhs[0] = mxCreateDoubleScalar(n / sizeof(short) / 2);
		mexPrintf("[Local USRP Mex] Received %d bytes for %d channels at %f\n", n, numChans, start_time);
		nlhs = 2;
	} else if (!strcmp("close", cmd)) {
		mexPrintf("[Local USRP Mex] Shutting down\n");
		if (txSockfd > 0) close(txSockfd);
		if (rxSockfd > 0) close(rxSockfd);
		if (rxCnSockfd > 0) close(rxCnSockfd);
		txSockfd = -1;
		rxSockfd = -1;
		rxCnSockfd = -1;
	}

	return;
}
