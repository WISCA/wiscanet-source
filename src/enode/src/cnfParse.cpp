#include <stdlib.h>
#include <iostream>
#include <sstream>

#include "tinyxml.h"

#include "cnfParse.h"

using namespace std;

int sysCfgParse(char *xmlFn, char *serverIp, int *serverPort) 
{
    char opModeStr[64];

    TiXmlDocument doc (xmlFn);
    bool loadOkay = doc.LoadFile();

    TiXmlDocument doc2 (xmlFn);
    bool loadOkay2 = doc2.LoadFile();

    if(!loadOkay)
    {
        printf("file load error %s\n", xmlFn);
        exit(1);
    }
    TiXmlNode* rootConfig = 0;
    TiXmlNode* sysConfig = 0;
    TiXmlElement* sysElement = 0;

    // xml parsing
    rootConfig = doc.FirstChild ("SysConfig");
    assert(rootConfig);

    // server IP addr
    sysConfig = rootConfig->FirstChild();
    assert(sysConfig);
    sysElement = sysConfig->ToElement();
    assert(sysElement);
    strcpy(serverIp, sysElement->GetText());
    printf("server IP address = %s\n", serverIp);

    // server port
    sysConfig = sysConfig->NextSibling();
    assert(sysConfig);
    sysElement = sysConfig->ToElement();
    assert(sysElement);
    *serverPort = atoi(sysElement->GetText());
    printf("server Port = %d\n", *serverPort);

    return 0;
}

int usrCfgParse(char *xmlFn, cfgData_t *cfg)
{
    char opModeStr[64];

    TiXmlDocument doc (xmlFn);
    bool loadOkay = doc.LoadFile();

    TiXmlDocument doc2 (xmlFn);
    bool loadOkay2 = doc2.LoadFile();

    if(!loadOkay)
    {
        printf("file load error %s\n", xmlFn);
        exit(1);
    }
    TiXmlNode* rootConfig = 0;
    TiXmlNode* usrConfig = 0;
    TiXmlElement* usrElement = 0;

    // xml parsing
    rootConfig = doc.FirstChild ("UsrConfig");
    assert(rootConfig);

    printf("\n\n============== enode user configuration =============\n");

    // logical ID
    usrConfig = rootConfig->FirstChild();
    assert(usrConfig);
    usrElement = usrConfig->ToElement();
    assert(usrElement);
    cfg->logicId = atoi(usrElement->GetText()); 
    printf("logcal ID = %d\n", cfg->logicId);

    // operation mode
    usrConfig = usrConfig->NextSibling();
    assert(usrConfig);
    usrElement = usrConfig->ToElement();
    assert(usrElement);
    strcpy(opModeStr, usrElement->GetText());
    if(strcmp("TX/RX", opModeStr) == 0) cfg->opMode = OPMODE_TX_RX;
    else if(strcmp("TX", opModeStr) == 0) cfg->opMode = OPMODE_TX;
    else cfg->opMode = OPMODE_RX;
    printf("opMode = %s\n", opModeStr);

    // macMode directory
    usrConfig = usrConfig->NextSibling();
    assert(usrConfig);
    usrElement = usrConfig->ToElement();
    assert(usrElement);
    strcpy(cfg->macMode, usrElement->GetText());
    printf("macMode = %s\n", cfg->macMode);

    // tx slot
    usrConfig = usrConfig->NextSibling();
    assert(usrConfig);
    usrElement = usrConfig->ToElement();
    assert(usrElement);
    cfg->tSlot = atoi(usrElement->GetText()); 
    printf("tx slot = %d\n", cfg->tSlot);

    // matlab directory
    usrConfig = usrConfig->NextSibling();
    assert(usrConfig);
    usrElement = usrConfig->ToElement();
    assert(usrElement);
    strcpy(cfg->matDir, usrElement->GetText());
    printf("matDir = %s\n", cfg->matDir);

    // top matlab file
    usrConfig = usrConfig->NextSibling();
    assert(usrConfig);
    usrElement = usrConfig->ToElement();
    assert(usrElement);
    strcpy(cfg->mTopFile, usrElement->GetText());
    printf("mTopFile = %s\n", cfg->mTopFile);

    // matlab log analssys file
    usrConfig = usrConfig->NextSibling();
    assert(usrConfig);
    usrElement = usrConfig->ToElement();
    assert(usrElement);
    strcpy(cfg->mLogFile, usrElement->GetText());
    printf("mLogFile = %s\n", cfg->mLogFile);

    // nsamps
    usrConfig = usrConfig->NextSibling();
    assert(usrConfig);
    usrElement = usrConfig->ToElement();
    assert(usrElement);
    cfg->nsamps = atoi(usrElement->GetText());
    printf("nsamps = %d\n", cfg->nsamps);

    // rate
    usrConfig = usrConfig->NextSibling();
    assert(usrConfig);
    usrElement = usrConfig->ToElement();
    assert(usrElement);
    cfg->rate = atof(usrElement->GetText());
    printf("rate = %.1f\n", cfg->rate);

    // subdev
    usrConfig = usrConfig->NextSibling();
    assert(usrConfig);
    usrElement = usrConfig->ToElement();
    assert(usrElement);
    strcpy(cfg->subdev, usrElement->GetText());
    printf("subdev = %s\n", cfg->subdev);

    // freq
    usrConfig = usrConfig->NextSibling();
    assert(usrConfig);
    usrElement = usrConfig->ToElement();
    assert(usrElement);
    cfg->freq = atof(usrElement->GetText());
    printf("freq = %.1f\n", cfg->freq);

    // txgain
    usrConfig = usrConfig->NextSibling();
    assert(usrConfig);
    usrElement = usrConfig->ToElement();
    assert(usrElement);
    cfg->txgain = atof(usrElement->GetText());
    printf("txgain = %.1f\n", cfg->txgain);

    // rxgain
    usrConfig = usrConfig->NextSibling();
    assert(usrConfig);
    usrElement = usrConfig->ToElement();
    assert(usrElement);
    cfg->rxgain = atof(usrElement->GetText());
    printf("rxgain = %.1f\n", cfg->rxgain);

    // bw
    usrConfig = usrConfig->NextSibling();
    assert(usrConfig);
    usrElement = usrConfig->ToElement();
    assert(usrElement);
    cfg->bw = atof(usrElement->GetText());
    printf("bw = %.1f\n", cfg->bw);

    printf("=====================================================\n");

    return 0;
}
