#ifndef __MSG_COM_H__
#define __MSG_COM_H__

#define MSG_PAYLOAD_LEN 256

typedef enum ctrlMsgType {
    MSG_ENODE_REG=100,
    MSG_ENODE_DL,
    MSG_ENODE_RUN,
    MSG_ENODE_STOP,
    MSG_ENODE_LOG,
    MSG_ENODE_TERM,
    MSG_ENODE_REG_ACK=200,
    MSG_ENODE_DL_ACK,
    MSG_ENODE_RUN_ACK,
    MSG_ENODE_RUN_CMP_IND,
    MSG_ENODE_LOG_ACK,
    MSG_USR_CMD = 300
} ctrlMsgType_t; 

typedef struct msgHdr {
    ctrlMsgType_t type;
    unsigned short int   seq;
} msgHdr_t;

typedef struct ctrlMsg {
   msgHdr_t hdr;
   unsigned int pload[MSG_PAYLOAD_LEN/4];
} ctrlMsg_t;

typedef struct cMsgEnodeReg {
   char ipaddr[32];
} cMsgEnodeReg_t;

typedef struct cMsgEnodeRegAck {
   unsigned char enodeId;
} cMsgEnodeRegAck_t;

typedef struct cMsgEnodeRun {
   double start_time;
} cMsgEnodeRunReq_t;

typedef struct cMsgEnodeDlInd_s {
   int dummy;
} cMsgEnodeDlInd_t, cMsgEnodeStopReq_t;

typedef struct cMsgEnodeDlAck_s {
   int ret;
} cMsgEnodeDlAck_t;

typedef struct cMsgEnodeRunAck {
   unsigned char enodeId;
   int ret; 
} cMsgEnodeRunAck_t;

typedef struct cMsgEnodeCmpInd {
   unsigned char enodeId;
   int ret; 
} cMsgEnodeRunCmpInd_t;

typedef struct cMsgEnodeLogReq {
   int dummy;
} cMsgEnodeLogReq_t;

typedef struct cMsgEnodeLogAck {
   unsigned char enodeId;
   int ret; 
} cMsgEnodeLogAck_t;

typedef struct cMsgEnodeTermReq {
   int dummy;
} cMsgEnodeTermReq_t;

typedef enum usrCmdType {
    CMD_PRT_STATUS = 1,
    CMD_DL_MATLAB = 2,
    CMD_EX_MATLAB = 4,
    CMD_STOP_MATLAB = 8,
    CMD_CLT_LOG = 16,
    CMD_LOG_ANAL = 32
} usrCmd_t;

typedef struct cMsgUsrCmd {
   usrCmd_t usrCmd;
} cMsgUsrCmd_t;

#define MSG_LEN(aa) (sizeof(ctrlMsg_t) - MSG_PAYLOAD_LEN + sizeof(aa))

#endif
