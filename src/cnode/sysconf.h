#ifndef __SYS_CONF_H__
#define __SYS_CONF_H__

#define MAX_ENODE 64
#define NODE_FREE 0
#define NODE_USED 1

typedef enum opstate { OP_IDLE, OP_START, OP_RUN, OP_CMP } opState_t;

struct nodeInfo {
	char state;
	int sockFd;
	char ipaddr[32];
	opState_t opState;
	int usrFlag = 0;
	char macMode[128] = "UMAC";
	char matDir[128] = ".";
	char mLogFile[64] = "NULL";
};

extern struct nodeInfo *sysConf;

#endif
