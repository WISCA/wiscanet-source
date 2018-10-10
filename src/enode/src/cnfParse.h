#ifndef __CNF_PARSE_HPP__
#define __CNF_PARSE_HPP__

#define OPMODE_TX_RX 1
#define OPMODE_TX    2
#define OPMODE_RX    3

typedef struct cfgData_s {
    int logicId;
    int opMode;
    char macMode[128];
    int tSlot;
    char matDir[128];
    char mTopFile[128];
    char mLogFile[128];
    int  nsamps;
    double rate;
    char subdev[32];
    double freq;
    double txgain;
    double rxgain;
    double bw;
} cfgData_t;


int sysCfgParse(char *xmlFn, char *serverIp, int *serverPort);
int usrCfgParse(char *xmlFn, cfgData_t *cfg);

#endif

