#include <stdio.h>
#include <stdlib.h>
#include <mongoose.h>
#include "ui_srv.h"
#include "msgdef.h"

static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;
struct netConf nConf;
int nConfInit = 0;

int sockfd;
char serverIp[64] = "10.206.201.85";
int serverPort = 9000;

void txUsrCmdMsg(usrCmd_t cmd)
{
    ctrlMsg_t msg;
    cMsgUsrCmd_t *pload;
    struct sockaddr_in localAddr;
    socklen_t addrLen;

    // build reg message
    msg.hdr.type = MSG_USR_CMD;
    pload = (cMsgUsrCmd_t *)msg.pload;
    pload->usrCmd = cmd;
   
    // send message
    printf("<-- msgUsrCmd\n");
    write(sockfd, &msg, MSG_LEN(cMsgUsrCmd_t));
}

void openSocket()
{
    int r;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        printf("socket() error\n");
        exit(1);
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIp, &serv_addr.sin_addr);

    r = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if(r < 0)
    {
        printf("connect error\n");
        exit(1);
    }
}

static void writeScript() {

    int i;
    FILE *fp;

    fp = fopen("matInst", "w");
    if(fp == NULL) {
        printf("matInst open error\n");
        exit(1);
    }

    fprintf(fp, "#!/bin/bash\n");

    for(i=0; i<nConf.nodeNum; i++) {
        fprintf(fp, "../cli/uInstall -t CFG -e %s -i %d -m %s -M %s -s %d -d %s -b %s -l %s\n",
                nConf.eConf[i].eIp, nConf.eConf[i].lId, nConf.eConf[i].trxs, 
                nConf.eConf[i].mmod, nConf.eConf[i].tslot, nConf.eConf[i].matDir,
                nConf.eConf[i].usrMat, nConf.eConf[i].logMat);
    }
    fprintf(fp, "../cli/uInstall -t INST -c %s\n", nConf.cIp);

    system("chmod -f 755 ./matInst");

    fclose(fp);
}

static int parseIntCfg(struct http_message *hm, int idx, char *idBuf) {

    char idBufIdx[128];
    char lineBuf[128];

    sprintf(idBufIdx, "%s%d", idBuf, idx);
    mg_get_http_var(&hm->body, idBufIdx, lineBuf, 128);
    return atoi(lineBuf);
}

static int parseIntJson( struct json_token *toks, char *path) {

    struct json_token *tp;
    char buf[1024];
    int i;

    tp = find_json_token(toks, path);
    for(i=0; i<tp->len; i++) buf[i] = tp->ptr[i];
    buf[i] = '\0';

    return atoi(buf);
}


static void parseStrCfg(struct http_message *hm, int idx, char *idBuf, char *lineBuf) {

    char idBufIdx[128];

    sprintf(idBufIdx, "%s%d", idBuf, idx);
    mg_get_http_var(&hm->body, idBufIdx, lineBuf, BUF_LEN);
}

static void parseStrJson(struct json_token *toks, char *path, char *dest) {

    struct json_token *tp;
    int i;

    tp = find_json_token(toks, path);
    for(i=0; i<tp->len; i++) dest[i] = tp->ptr[i];
    dest[i] = '\0';
}

static void handle_cfg(struct mg_connection *nc, struct http_message *hm) {

    int i, j, k, idx;
    char lineBuf[1024];
    char idArray[][64] = {"eIp", "lId", "trxs", "mmod", "tslot", "matDir", "usrMat", "logMat"};
    struct json_token toks[1000], *tp;

    printf("-- cfg requested :\n");
    parse_json(hm->body.p, hm->body.len, toks, sizeof(toks));

    // node number
    nConf.nodeNum = parseIntJson(toks, "nodeNum");
    printf("nodeNum = %d\n", nConf.nodeNum);

    // cnode IP
    parseStrJson(toks, "cIp", nConf.cIp);
    printf("cIp = %s\n", nConf.cIp);

    // resource allocation
    if(nConfInit == 0) nConfInit = 1;
    else free(nConf.eConf);
    nConf.eConf = (struct nodeConf *)malloc(sizeof(struct nodeConf) * nConf.nodeNum);

    // prase node configuration
    for(i=0; i<nConf.nodeNum; i++) {
        idx = 0;
        sprintf(lineBuf, "enode[%d]", i);
        tp  = find_json_token(toks, lineBuf);

        parseStrJson(tp, idArray[idx++], nConf.eConf[i].eIp);
        nConf.eConf[i].lId = parseIntJson(tp, idArray[idx++]);
        parseStrJson(tp, idArray[idx++], nConf.eConf[i].trxs);
        parseStrJson(tp, idArray[idx++], nConf.eConf[i].mmod);
        nConf.eConf[i].tslot = parseIntJson(tp, idArray[idx++]);
        parseStrJson(tp, idArray[idx++], nConf.eConf[i].matDir);
        parseStrJson(tp, idArray[idx++], nConf.eConf[i].usrMat);
        parseStrJson(tp, idArray[idx++], nConf.eConf[i].logMat);
    }

    // write shell script
    writeScript();

    // Send response
    mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\n\r\n%.*s",
            (unsigned long) hm->body.len, (int) hm->body.len, hm->body.p);
   
}

static void handle_save(struct mg_connection *nc, struct http_message *hm) {

    char fname[128];
    struct json_token toks[1000], *tp;
    FILE *fp;

    printf("-- save requested :\n");
    parse_json(hm->body.p, hm->body.len, toks, sizeof(toks));

    // file name
    parseStrJson(toks, "fname", fname);
    printf("fname = %s\n", fname);

    fp = fopen(fname, "w");
    fwrite(hm->body.p, hm->body.len, 1, fp);
    fclose(fp);

    /* Send response */
    mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\n\r\n%.*s",
            (unsigned long) hm->body.len, (int) hm->body.len, hm->body.p);
}

static void handle_boot(struct mg_connection *nc, struct http_message *hm) {

    printf("-- boot requested\n");

    /* Send response */
    mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\n\r\n%.*s",
            (unsigned long) hm->body.len, (int) hm->body.len, hm->body.p);
}

static void handle_run(struct mg_connection *nc, struct http_message *hm) {

    printf("-- run requested\n");

    /* install user matlab code */
    system("./matInst");
    
    /* download & execution */
    printf("-- cmd = %d\n", (CMD_DL_MATLAB | CMD_EX_MATLAB));
    txUsrCmdMsg((usrCmd_t)(CMD_DL_MATLAB | CMD_EX_MATLAB));

    /* Send response */
    mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\n\r\n%.*s",
            (unsigned long) hm->body.len, (int) hm->body.len, hm->body.p);
}

static void handle_log_col(struct mg_connection *nc, struct http_message *hm) {

    printf("-- log collection requested\n");

    /* download & execution */
    printf("-- cmd = %d\n", (CMD_CLT_LOG));
    txUsrCmdMsg(CMD_CLT_LOG);

    /* Send response */
    mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\n\r\n%.*s",
            (unsigned long) hm->body.len, (int) hm->body.len, hm->body.p);
}

static void handle_log_anal(struct mg_connection *nc, struct http_message *hm) {

    printf("-- log analysis requested\n");

    /* download & execution */
    printf("-- cmd = %d\n", CMD_LOG_ANAL);
    txUsrCmdMsg(CMD_LOG_ANAL);

    /* Send response */
    mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\n\r\n%.*s",
            (unsigned long) hm->body.len, (int) hm->body.len, hm->body.p);
}

static void handle_ssi_call(struct mg_connection *nc, const char *param) {

    printf("handle_ssi_call()\n");
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {

    struct http_message *hm = (struct http_message *) ev_data;

    switch (ev) {
        case MG_EV_HTTP_REQUEST:
//printf("hm->uri = %s\n", hm->uri);
            if (mg_vcmp(&hm->uri, "/save") == 0) {
                handle_save(nc, hm);                    /* Handle RESTful call */
            }
            else if(mg_vcmp(&hm->uri, "/cfg") == 0) {
                handle_cfg(nc, hm);
            } 
            else if(mg_vcmp(&hm->uri, "/boot") == 0) {
                handle_boot(nc, hm);
            } 
            else if(mg_vcmp(&hm->uri, "/run") == 0) {
                handle_run(nc, hm);
            } 
            else if(mg_vcmp(&hm->uri, "/log/col") == 0) {
                handle_log_col(nc, hm);
            } 
            else if(mg_vcmp(&hm->uri, "/log/anal") == 0) {
                handle_log_anal(nc, hm);
            } 
            else {
                mg_serve_http(nc, hm, s_http_server_opts);  /* Serve static content */
            }
            break;
        case MG_EV_SSI_CALL:
            handle_ssi_call(nc, (char *)ev_data);
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[]) {

    struct mg_mgr mgr;
    struct mg_connection *nc;
    char *p, path[512];

    /* open socket */
    openSocket();

    /* mgr init */
    mg_mgr_init(&mgr, NULL);
    nc = mg_bind(&mgr, s_http_port, ev_handler);
    mg_set_protocol_http_websocket(nc);
    s_http_server_opts.document_root = "./web_root";
    s_http_server_opts.auth_domain = "example.com";
    //mgr.hexdump_file = "/dev/stdout";

    /* If our current directory */
    if (argc > 0 && (p = strrchr(argv[0], '/'))) {
        snprintf(path, sizeof(path), "%.*s/web_root", (int)(p - argv[0]), argv[0]);
        s_http_server_opts.document_root = path;
    }

    printf("Starting device configurator on port %s\n", s_http_port);
    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);

    return 0;
}
