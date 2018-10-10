#ifndef __UI_SRV_H__
#define __UI_SRV_H__

#define BUF_LEN 128

struct nodeConf {
    char eIp[BUF_LEN];
    int lId;
    char trxs[BUF_LEN];
    char mmod[BUF_LEN];
    int tslot;
    char matDir[BUF_LEN];
    char usrMat[BUF_LEN];
    char logMat[BUF_LEN];
};

struct netConf {
    int nodeNum;
    char cIp[BUF_LEN];
    struct nodeConf *eConf;
};

void xmlFileWrite(struct netConf *config);

#endif
