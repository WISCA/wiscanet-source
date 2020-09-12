#include "cnfParse.h"

#include <stdlib.h>

#include <iostream>
#include <sstream>
#include <cstring>

#include <yaml-cpp/yaml.h>

int sysCfgParse(char* ymlFn, char* serverIp, int* serverPort) {
    YAML::Node sysCfg = YAML::LoadFile(ymlFn);

    const std::string server_ip = sysCfg["server_ip"].as<std::string>();
    const int server_port = sysCfg["server_port"].as<int>();

	// yml parsing
	strcpy(serverIp, server_ip.c_str());
	printf("server IP address = %s\n", serverIp);

	// server port
	*serverPort = server_port;
	printf("server Port = %d\n", *serverPort);

	return 0;
}

int usrCfgParse(char* ymlFn, cfgData_t* cfg) {
	char opModeStr[64];

    YAML::Node usrCfg = YAML::LoadFile(ymlFn);

    printf("\n\n============== enode user configuration =============\n");

	// logical ID
	const int logicalId = usrCfg["logic_id"].as<int>();
    cfg->logicId = logicalId;
	printf("Logical ID = %d\n", cfg->logicId);

	// operation mode
    const std::string op_mode = usrCfg["op_mode"].as<std::string>();
	strcpy(opModeStr, op_mode.c_str());
	if (strcmp("TX/RX", opModeStr) == 0)
		cfg->opMode = OPMODE_TX_RX;
	else if (strcmp("TX", opModeStr) == 0)
		cfg->opMode = OPMODE_TX;
	else
		cfg->opMode = OPMODE_RX;
	printf("Operating Mode = %s\n", opModeStr);

	// macMode directory
    const std::string mac_mode = usrCfg["mac_mode"].as<std::string>();
	strcpy(cfg->macMode, mac_mode.c_str());
	printf("MAC Mode = %s\n", cfg->macMode);

	// tx slot
	const int timeSlot = usrCfg["time_slot"].as<int>();
    cfg->tSlot = timeSlot;
	printf("Time Slot = %d\n", cfg->tSlot);

	// matlab directory
	const std::string mat_dir = usrCfg["matlab_dir"].as<std::string>();
    strcpy(cfg->matDir, mat_dir.c_str());
	printf("MATLAB Directory = %s\n", cfg->matDir);

	// MATLAB Function File to execute
    const std::string mat_top = usrCfg["matlab_func"].as<std::string>();
	strcpy(cfg->mTopFile, mat_top.c_str());
	printf("MATLAB Function = %s\n", cfg->mTopFile);

	// MATLAB Log Analysis File
    const std::string mat_log = usrCfg["matlab_log"].as<std::string>();
	strcpy(cfg->mLogFile, mat_log.c_str());
	printf("MATLAB Log Parser = %s\n", cfg->mLogFile);

	// nsamps
    const int num_samples = usrCfg["num_samples"].as<int>();
	cfg->nsamps = num_samples;
	printf("Number of Samples = %d\n", cfg->nsamps);

	// rate
    const double samp_rate = usrCfg["sample_rate"].as<double>();
	cfg->rate = samp_rate;
	printf("Sample Rate = %.1f\n Hz", cfg->rate);

	// subdev
    const std::string sub_dev = usrCfg["subdevice"].as<std::string>();
	strcpy(cfg->subdev, sub_dev.c_str());
	printf("Subdevice Spec = %s\n", cfg->subdev);

	// freq
    const double center_freq = usrCfg["freq"].as<double>();
	cfg->freq = center_freq;
	printf("Center Frequency = %.1f Hz\n", cfg->freq);

	// txgain
    const double tx_gain = usrCfg["tx_gain"].as<double>();
	cfg->txgain = tx_gain;
	printf("Transmit Gain = %.1f dB\n", cfg->txgain);

	// rxgain
    const double rx_gain = usrCfg["rx_gain"].as<double>();
	cfg->rxgain = rx_gain;
	printf("Receive Gain = %.1f dB\n", cfg->rxgain);

	// bw
    const double band_width = usrCfg["bandwidth"].as<double>();
	cfg->bw = band_width;
	printf("Bandwidth = %.1f Hz\n", cfg->bw);

    // Device Address
    const std::string dev_addr = usrCfg["device_addr"].as<std::string>();
	strcpy(cfg->devAddr, dev_addr.c_str());
	printf("Device Address = %s\n", cfg->devAddr);

    // Channel Configuration
    const std::string chan_spec = usrCfg["channels"].as<std::string>();
	strcpy(cfg->channels, chan_spec.c_str());
	printf("Channel Spec = %s\n", cfg->channels);

    // Channel Configuration
    const std::string ant_spec = usrCfg["antennas"].as<std::string>();
	strcpy(cfg->antennas, ant_spec.c_str());
	printf("Antenna Spec = %s\n", cfg->antennas);

	printf("=====================================================\n");

	return 0;
}
