#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "mex.h"

#define CBLEN 32
#define DBLEN (16384 * 10) /* max 2048 complex float samples */

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

void sendPacket(char *buf, int len) {
	int totalTx = 0, txLen, txUnit = 4096, txResult = -1;

	while (totalTx < len) {
		if ((len - totalTx) > txUnit)
			txLen = txUnit;
		else
			txLen = len - totalTx;
		txResult = sendto(txSockfd, (buf + totalTx), txLen, 0, (struct sockaddr *)&si_other, sizeof(si_other));
		if (txResult == -1) break;
		/*
		   mexPrintf("totalTx = %d, txLen = %d\n", totalTx, txLen);
		   */
		totalTx += txResult;
	}
	/*
	   mexPrintf("totalTx = %d\n", totalTx);
	   */

	/* termination packet */
	sendto(txSockfd, buf, 0, 0, (struct sockaddr *)&si_other, sizeof(si_other));

	/*
	   {
	   int i;
	   float* fdat = (float *)buf;
	   for(i=0; i<len/8; i++) {
	   mexPrintf("%d %f %f\n", i, fdat[i*2], fdat[i*2+1]);
	   }
	   }
	   */

	/*
	   mexPrintf("TX %d bytes\n", totalTx);
	   */
}

void controlRecv(double start_time) {
	/* send control packet */
	sendto(rxCnSockfd, (char *)&start_time, 16, 0, (struct sockaddr *)&si_rcon, sizeof(si_rcon));
}

int recvPacket(char *buf, int len) {
	int n, slen;
	n = recvfrom(rxSockfd, buf, DBLEN, 0, (struct sockaddr *)&si_me, (socklen_t *)&slen);
	/*
	   mexPrintf("recvPacket(), n=%d, len=%d\n", n, len);
	   */
	return n;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
	char cmd[CBLEN];
	char ipaddr[CBLEN];
	int portNum, cportNum;
	short *sBuf;
	int len;
	const int *dim_array;
	mwSize dims[1];
	int n;
	double start_time;

	if (nrhs < 1) {
		mexPrintf("need more input arguments\n");
		return;
	}

	mxGetString(prhs[0], cmd, CBLEN);
	/*
	   mexPrintf("%s\n", cmd);
	   */

	if (!strcmp("txinit", cmd)) {
		mexPrintf("Init TX operation\n");
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
		mexPrintf("Init RX operation\n");
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
		/*
		   mexPrintf("Write operation\n");
		   */
		if (txSockfd <= 0) {
			mexPrintf("socket is not ready\n");
			return;
		}
		dim_array = mxGetDimensions(prhs[1]);
		len = dim_array[1] * sizeof(float);
		sBuf = (short *)mxGetData(prhs[1]);
		sendPacket((char *)sBuf, len);
	} else if (!strcmp("read", cmd)) {
		/*
		   mexPrintf("read operation\n");
		   */
		if (rxSockfd <= 0) {
			mexPrintf("socket is not ready\n");
			return;
		}
		len = mxGetScalar(prhs[1]);
		dims[0] = len;
		plhs[1] = mxCreateNumericArray(1, dims, mxINT16_CLASS, mxREAL);
		sBuf = (short *)mxGetPr(plhs[1]);
		n = recvPacket((char *)sBuf, len * sizeof(short));
		plhs[0] = mxCreateDoubleScalar(n / sizeof(short));
		nlhs = 2;
	} else if (!strcmp("rcon", cmd)) {
		if (rxCnSockfd <= 0) {
			mexPrintf("rx control socket is not ready\n");
			return;
		}
		start_time = mxGetScalar(prhs[1]);
		mexPrintf("gradio_mex: read control operation : start_time = %f\n", start_time);
		controlRecv(start_time);
		nlhs = 0;
	} else if (!strcmp("close", cmd)) {
		mexPrintf("close operation\n");
		if (txSockfd > 0) close(txSockfd);
		if (rxSockfd > 0) close(rxSockfd);
		if (rxCnSockfd > 0) close(rxCnSockfd);
		txSockfd = -1;
		rxSockfd = -1;
		rxCnSockfd = -1;
	}

	return;
}
