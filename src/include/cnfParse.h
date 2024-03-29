#ifndef __CNF_PARSE_HPP__
#define __CNF_PARSE_HPP__

/*
 * Operational mode defines for transmit/receive mode
 */
#define OPMODE_TX_RX 1
#define OPMODE_TX 2
#define OPMODE_RX 3

/*
 * Language defines for baseband type
 */
#define LANG_MATLAB 1
#define LANG_PYTHON 2
#define LANG_GNURADIO 3

/*! \typedef struct cfgData_s
 * \brief This structure contains edge node configuration information
 *
 * This structure contains all the parameters read in from the configuration files, and their translations to parameters
 * for each edge node.
 */
typedef struct cfgData_s {
	int logicId;
	int opMode;
	char matDir[128];
	char mTopFile[128];
	char mLogFile[128];
	int nsamps;
	double rate;
	char subdev[32];
	double freq;
	double txgain;
	double rxgain;
	double bw;
	char devAddr[128];
	char channels[128];
	char antennas[128];
	int lang;
} cfgData_t;

int sysCfgParse(char *ymlFn, char *serverIp, int *serverPort);
int usrCfgParse(char *ymlFn, cfgData_t *cfg);

#endif
