//
// Copyright 2010-2011,2014 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <complex>
#include <csignal>
#include <fstream>
#include <iostream>
#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>

#include "verStr.h"

#define TX_PORT 9940
#define RX_PORT 9944
#define RC_PORT 9945

typedef struct __attribute__((__packed__)) {
    double startTime;
    size_t numChans;
    int16_t refPower;
} transmit_controlfmt;

#define recv_worker_args(format)                                                                           \
	(usrp, format, wirefmt, file, spb, total_num_samps, channel_nums, total_time, bw_summary, stats, null, \
	 enable_size_map, continue_on_bad_packet)

namespace po = boost::program_options;
namespace pt = boost::posix_time;
typedef std::function<uhd::sensor_value_t(const std::string&)> get_sensor_fn_t;

static bool stop_signal_called = false;
void sig_int_handler(int) { stop_signal_called = true; }

template <typename samp_type>
void recv_worker(uhd::usrp::multi_usrp::sptr usrp, const std::string &cpu_format, const std::string &wire_format,
                      const std::string &file, size_t samps_per_buff, unsigned long long num_requested_samples,
                      std::vector<size_t> channel_nums, double time_requested = 0.0, bool bw_summary = false,
                      bool stats = false, bool null = false, bool enable_size_map = false,
                      bool continue_on_bad_packet = false) {
	std::cout << "================== Receive Worker =================\n";
	// time variables for scheduling
	uhd::time_spec_t time_now, rxTime;

	size_t num_total_samps = 0;

	// udp socket
	struct sockaddr_in si_other, si_me;
	int sockfd, slen;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) exit(1);
	memset((char *)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(RX_PORT);
	if (inet_aton("127.0.0.1", (struct in_addr *)&si_other.sin_addr) == 0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
	memset((char *)&si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(RC_PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sockfd, (struct sockaddr *)&si_me, sizeof(si_me)) == -1) {
		fprintf(stderr, "bind() error\n");
		exit(1);
	}

	uhd::rx_metadata_t md;
	bool overflow_message = true;

	typedef std::map<size_t, size_t> SizeMap;
	SizeMap mapSizes;

	// wait rx control command
	int rResult;
	char cmdBuf[16];
	double *stime = (double *)cmdBuf;
	double realRxTime, realNowTime;
	double timeout;
	size_t rxUnit;
	uint16_t numChannels;

	while (1) {
		rResult = recvfrom(sockfd, cmdBuf, 16, 0, (struct sockaddr *)&si_me, (socklen_t *)&slen);
		if (rResult < 0) {
			printf("Receiving command buffer failed (Error %d)\r\n", rResult);
		}
		rResult = recvfrom(sockfd, &numChannels, sizeof(uint16_t), 0, (struct sockaddr *)&si_me, (socklen_t *)&slen);
		if (rResult < 0) {
			printf("Receiving number of channels failed (Error %d)\r\n", rResult);
		}
		rxTime = uhd::time_spec_t(*stime);
		time_now = usrp->get_time_now(0);
		realRxTime = rxTime.get_real_secs();
		realNowTime = time_now.get_real_secs();
		printf("[USRP Control] RX CMD for rxTime=%f, time_now =%f, numChannels = %d\r\n", realRxTime, realNowTime,
		       numChannels);

		if (realRxTime < realNowTime) {
			printf("[USRP Control] Error: RX time has already passed\n");
			exit(1);
		}

		timeout = realRxTime - realNowTime + 1;  // Put some slack in there for huge applications, doing MIMO

		// create a receive streamer
		uhd::stream_args_t stream_args(cpu_format, wire_format);
		stream_args.channels = channel_nums;
		uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

		uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
		stream_cmd.num_samps = num_requested_samples * channel_nums.size();

		printf("[USRP Control] Using %ld of %ld available channels\r\n", channel_nums.size(),
		       usrp->get_rx_num_channels());

		rxUnit = rx_stream->get_max_num_samps();
		// allocate buffers to receive with samples (one buffer per channel)
		printf("[USRP Control] Allocating %ld buffers of %llu samples each\r\n", channel_nums.size(),
		       num_requested_samples);
		std::vector<std::vector<samp_type>> rx_buffers(channel_nums.size(),
		                                               std::vector<samp_type>(num_requested_samples));

		rxUnit = num_requested_samples;

		// rx streamer cmd
		num_total_samps = 0;
		stream_cmd.stream_now = false;
		stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE;
		stream_cmd.time_spec = rxTime;
		// Tells all channels to stream
		rx_stream->issue_stream_cmd(stream_cmd);

		// create a vector of pointers to point to each of the channel buffers
		std::vector<samp_type *> buff_ptrs;
		for (size_t i = 0; i < rx_buffers.size(); i++) {
			buff_ptrs.push_back(&rx_buffers[i].front());
		}

		while (num_requested_samples > num_total_samps) {
			size_t num_rx_samps = rx_stream->recv(buff_ptrs, rxUnit, md, timeout);
			for (size_t i = 0; i < rx_buffers.size(); i++) {
				buff_ptrs[i] += num_rx_samps / rx_buffers.size();
			}

			if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
				std::cout << boost::format("Timeout while streaming") << std::endl;
				break;
			}
			if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
				if (overflow_message) {
					overflow_message = false;
					std::cerr << boost::format(
					                 "Got an overflow indication. Please consider the following:\n"
					                 "  Your write medium must sustain a rate of %fMB/s.\n"
					                 "  Dropped samples will not be written to the file.\n"
					                 "  Please modify this example for your purposes.\n"
					                 "  This message will not appear again.\n") %
					                 (usrp->get_rx_rate() * sizeof(samp_type) / 1e6);
				}
				continue;
			}
			if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
				std::string error = str(boost::format("Receiver error: %s") % md.strerror());
				if (continue_on_bad_packet) {
					std::cerr << error << std::endl;
					continue;
				} else
					throw std::runtime_error(error);
			}
			if (enable_size_map) {
				SizeMap::iterator it = mapSizes.find(num_rx_samps);
				if (it == mapSizes.end()) mapSizes[num_rx_samps] = 0;
				mapSizes[num_rx_samps] += 1;
			}
			printf("[USRP Control] Device RX Loop: %lu samples\r\n", num_rx_samps);
			num_total_samps += num_rx_samps;
		}

		printf("[USRP Control] Received %ld total samples from device\r\n", num_total_samps);
		/*
		   {
		   for (size_t i=0; i<num_total_samps; i++) {
		   printf("%d %d\r\n", std::real(rx_buffers[0][i]), std::imag(rx_buffers[0][i]));
		   }
		   }
		   */
		time_now = usrp->get_time_now(0);

		size_t count = 0, txTotal = 0, txUnit = 4000, txLen = 0, txResult = 0;
		// send rx packet to matlab through udp socket
		for (int i = 0; i < numChannels; i++) {
			size_t txSamps = num_total_samps;
			printf("[USRP Control] Sending channel %d\r\n", i);
			//		size_t count=0, txTotal=0, txUnit=1000, txLen=0, txResult=0;
			count = 0, txTotal = 0, txUnit = 2000, txLen = 0, txResult = 0;

			while (txTotal < txSamps) {
				if ((txSamps - txTotal) > txUnit)
					txLen = txUnit;
				else
					txLen = txSamps - txTotal;
				txResult = sendto(sockfd, (&rx_buffers[i].front() + (txUnit * count)), txLen * sizeof(samp_type), 0,
				                  (struct sockaddr *)&si_other, sizeof(si_other));
				if (txResult < 0) break;
				count++;
				txTotal += (txResult / sizeof(samp_type));
				usleep(20);  // intentional delay for proper context switching
				// printf("-- RX : txTotal = %d samps\n", txTotal);
			}
			// printf("-- RX : txTotal = %d samps\n", txTotal);
		}
		// send zero size termination packet
		sendto(sockfd, &rx_buffers[0].front(), 0, 0, (struct sockaddr *)&si_other, sizeof(si_other));
		// printf("-- RX : zero size packet\n");

		printf("[USRP Control] Sent at time=%f, len=%ld (samples) \r\n", time_now.get_real_secs(), txTotal);
	}
}

bool check_locked_sensor(std::vector<std::string> sensor_names,
    const char* sensor_name,
    get_sensor_fn_t get_sensor_fn,
    double setup_time)
{
    if (std::find(sensor_names.begin(), sensor_names.end(), sensor_name)
        == sensor_names.end())
        return false;

    auto setup_timeout = std::chrono::steady_clock::now()
                         + std::chrono::milliseconds(int64_t(setup_time * 1000));
    bool lock_detected = false;

    std::cout << boost::format("Waiting for \"%s\": ") % sensor_name;
    std::cout.flush();

    while (true) {
        if (lock_detected and (std::chrono::steady_clock::now() > setup_timeout)) {
            std::cout << " locked." << std::endl;
            break;
        }
        if (get_sensor_fn(sensor_name).to_bool()) {
            std::cout << "+";
            std::cout.flush();
            lock_detected = true;
        } else {
            if (std::chrono::steady_clock::now() > setup_timeout) {
                std::cout << std::endl;
                throw std::runtime_error(
                    str(boost::format(
                            "timed out waiting for consecutive locks on sensor \"%s\"")
                        % sensor_name));
            }
            std::cout << "_";
            std::cout.flush();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << std::endl;
    return true;
}

uhd::time_spec_t get_system_time(void) {
	pt::ptime time_now = pt::microsec_clock::universal_time();
	pt::time_duration time_dur = time_now - pt::from_time_t(0);
	return uhd::time_spec_t(time_t(time_dur.total_seconds()), long(time_dur.fractional_seconds()),
	                        double(pt::time_duration::ticks_per_second()));
}

int synch_to_gps(uhd::usrp::multi_usrp::sptr usrp) {
	int gps_unlocked = 0;

	// Lock mboard clocks
	usrp->set_clock_source("gpsdo", 0);
	usrp->set_time_source("gpsdo", 0);
	int numMB = usrp->get_num_mboards();
	std::cout << "Synchronizing for " << numMB << " Main Boards" << std::endl;
	if (numMB > 1) {
		std::cout << "Since there are multiple motherboards detected, it is assumed the first (addr0) is the master."
		          << std::endl;
		std::cout << "Now syncing the rest of the motherboards to the provided reference clock." << std::endl;
		for (int i = 1; i < numMB; i++) {
			std::cout << "Setting MBoard: " << i << "'s clock reference" << std::endl;
			usrp->set_clock_source("external", i);
			usrp->set_time_source("external", i);
		}
	}

	std::cout << "Synchronizing mboard " << 0 << ": " << usrp->get_mboard_name(0) << std::endl;
	std::vector<std::string> sensor_names = usrp->get_mboard_sensor_names(0);
	if (std::find(sensor_names.begin(), sensor_names.end(), "ref_locked") != sensor_names.end()) {
		std::cout << "Waiting for reference lock..." << std::flush;
		bool ref_locked = false;
		for (int i = 0; i < 30 and not ref_locked; i++) {
			ref_locked = usrp->get_mboard_sensor("ref_locked", 0).to_bool();
			if (not ref_locked) {
				std::cout << "." << std::flush;
				boost::this_thread::sleep(boost::posix_time::seconds(1));
			}
		}
		if (ref_locked) {
			std::cout << "LOCKED" << std::endl;
		} else {
			std::cout << "FAILED" << std::endl;
			throw uhd::runtime_error("Failed to lock to GPSDO 10 MHz Reference");
		}
	} else {
		std::cout << boost::format("ref_locked sensor not present on this board.\n");
	}

	// Wait for GPS lock
	while (1) {
		bool gps_locked = usrp->get_mboard_sensor("gps_locked", 0).to_bool();
		if (gps_locked) {
			std::cout << boost::format("GPS Locked\n");
			break;
		} else {
			std::cerr << "WARNING:  GPS not locked - time will not be accurate until locked" << std::endl;
			//            std::cout << "WARNING:  GPS not locked - time will not be accurate until locked" << std::endl;
			if (gps_unlocked++ > 10) {
				//                std::cerr << "Terminate uControl because of GPS lock fail\n";
				//                exit(1);
				break;
			}
		}
	}

	// Set to GPS time
	uhd::time_spec_t gps_time = uhd::time_spec_t(time_t(usrp->get_mboard_sensor("gps_time", 0).to_int()));
	usrp->set_time_next_pps(gps_time + 1.0, 0);
	if (numMB > 1) {
		std::cout << "Now syncing the rest of the motherboards to the master's GPS time" << std::endl;
		for (int i = 1; i < numMB; i++) {
			std::cout << "Setting MBoard: " << i << " to GPS time" << std::endl;
			usrp->set_time_next_pps(gps_time + 1.0, i);
		}
	}

	// Wait for it to apply
	// The wait is 2 seconds because N-Series has a known issue where
	// the time at the last PPS does not properly update at the PPS edge
	// when the time is actually set.
	boost::this_thread::sleep(boost::posix_time::seconds(2));

	// Check times
	gps_time = uhd::time_spec_t(time_t(usrp->get_mboard_sensor("gps_time", 0).to_int()));
	uhd::time_spec_t time_last_pps = usrp->get_time_last_pps(0);
	std::cout << "USRP time: " << (boost::format("%0.9f") % time_last_pps.get_real_secs()) << std::endl;
	std::cout << "GPSDO time: " << (boost::format("%0.9f") % gps_time.get_real_secs()) << std::endl;
	if (gps_time.get_real_secs() == time_last_pps.get_real_secs())
		std::cout << std::endl << "SUCCESS: USRP time synchronized to GPS time" << std::endl << std::endl;
	else
		std::cerr << std::endl << "ERROR: Failed to synchronize USRP time to GPS time" << std::endl << std::endl;

	return 0;
}

void transmit_worker(uhd::usrp::multi_usrp::sptr usrp, size_t total_num_samps, std::vector<size_t> channel_nums) {
	std::cout << "================== Transmit Worker =================\n";

	// variables for timer
	uhd::time_spec_t txTime, time_now, prev_txtime;

	prev_txtime = uhd::time_spec_t(0.0);

	// metadata for transmission
	uhd::tx_metadata_t md;

	// udp socket
	struct sockaddr_in si_me;
	int sockfd, slen;
	size_t rLen;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) exit(1);
	memset((char *)&si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(TX_PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sockfd, (struct sockaddr *)&si_me, sizeof(si_me)) == -1) exit(1);

	// create a transmit streamer
	uhd::stream_args_t stream_args("fc32", "sc16");
	stream_args.channels = channel_nums;
	uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

	// buffer for transmission
    transmit_controlfmt *controlBuf;
    static_assert(sizeof(transmit_controlfmt) == (sizeof(double) + sizeof(size_t) + sizeof(int16_t))); // Make sure this covers the right memory space
    // Don't waste space by overextending the buffer
	std::vector<std::complex<float>> buff(total_num_samps * channel_nums.size() + sizeof(transmit_controlfmt));
	// information
	char *udpBuf;
	size_t rResult;
	double *start_time;
	double timeout;
	size_t txLen;
	size_t rSamLen;
	size_t *numChans;
    int16_t *refPower;
	while (1) {
		udpBuf = (char *)&buff[0];
		rLen = 0;
		rResult = 0;

		do {
			udpBuf += rResult;
			rLen += rResult;
			rResult = recvfrom(sockfd, udpBuf, buff.size(), 0, (struct sockaddr *)&si_me, (socklen_t *)&slen);
		} while (rResult > 0);
		rSamLen = (rLen - sizeof(transmit_controlfmt)) / sizeof(std::complex<float>);

		time_now = usrp->get_time_now(0);
		//start_time = (double *)(&buff[0] + rSamLen - 1);
		//numChans = (size_t *)(&buff[0] + rSamLen);
        controlBuf = (transmit_controlfmt *)(&buff[0] + rSamLen);
        start_time = &controlBuf->startTime;
        numChans = &controlBuf->numChans;
        refPower = &controlBuf->refPower;
		printf("[USRP Control] TX: Received %ld bytes (%ld samples) from MATLAB for %ld channels, with reference power %d dBm.\n", rLen, rSamLen,
		       *numChans, *refPower);
		// printf("start_time = %f, prev_time = %f, time_now = %f\n", *start_time, prev_txtime.get_real_secs(),
		// time_now.get_real_secs());

        // Configure TX Power Reference for all channels
		for (size_t i = 0; i < *numChans; i++) {
            try {
                usrp->set_tx_power_reference(*refPower, i);
            } catch (uhd::not_implemented_error &e){
                std::cout << "[USRP Control] Reference Power not supported, falling back to default gain" << std::endl << e.what() << std::endl;
            }
        }

		// txtime management
		txTime = uhd::time_spec_t(*start_time);
		if (prev_txtime.get_real_secs() > time_now.get_real_secs()) {
			printf("[USRP Control] Scheduling error: prev_time = %f, time_now = %f\n", prev_txtime.get_real_secs(),
			       time_now.get_real_secs());
			exit(1);
		}
		prev_txtime = txTime;

		// md setup
		md.start_of_burst = true;
		md.end_of_burst = false;
		md.has_time_spec = true;
		md.time_spec = txTime;

		std::vector<std::vector<std::complex<float>>> tx_buffers(channel_nums.size(),
		                                                         std::vector<std::complex<float>>(total_num_samps));

		for (size_t i = 0; i < *numChans; i++) {
			tx_buffers[i].assign(buff.begin() + (i * total_num_samps), buff.begin() + ((i + 1) * total_num_samps));
		}

		// create a vector of pointers to point to each of the channel buffers
		std::vector<std::complex<float> *> buff_ptrs;
		for (size_t i = 0; i < tx_buffers.size(); i++) {
			buff_ptrs.push_back(&tx_buffers[i].front());
		}
		// send the contents of tx buffer
		timeout = txTime.get_real_secs() - time_now.get_real_secs() + 0.1;

		txLen = tx_stream->send(buff_ptrs, total_num_samps, md, timeout);
		if (txLen < total_num_samps) {
			printf("tx error actual_tx_len = %ld, request_len = %ld\n", txLen, total_num_samps);
			exit(1);
		}
		printf("[USRP Control] Transmit: assigned_tx_time = %f, time_now = %f\n", prev_txtime.get_real_secs(),
		       time_now.get_real_secs());
		// printf("txBufLoc = %d, txLen = %d\n", txBufLoc, txLen);

		// send a mini EOB packet
		md.start_of_burst = false;
		md.has_time_spec = false;
		md.end_of_burst = true;
		tx_stream->send("", 0, md);
	}
}

int UHD_SAFE_MAIN(int argc, char *argv[]) {
	uhd::set_thread_priority_safe();

	std::cerr << "uControl Version: " << verStr << std::endl;
	std::cout << "uControl Version: " << verStr << std::endl;

	// variables to be set by po
	std::string args, file, type, ant, subdev, ref, wirefmt, opmode, channel_list;
	size_t total_num_samps, spb;
	double rate, freq, txgain, rxgain, bw, total_time, setup_time;

	// setup the program options
	po::options_description desc("Allowed options");
	desc.add_options()("help", "help message")("args", po::value<std::string>(&args)->default_value("type=b200"),
	                                           "multi uhd device address args")(
	    "file", po::value<std::string>(&file)->default_value("usrp_samples.dat"),
	    "name of the file to write binary samples to")("type", po::value<std::string>(&type)->default_value("short"),
	                                                   "sample type: double, float, or short")(
	    "nsamps", po::value<size_t>(&total_num_samps)->default_value(50000), "total number of samples to receive")(
	    "duration", po::value<double>(&total_time)->default_value(0), "total number of seconds to receive")(
	    "time", po::value<double>(&total_time), "(DEPRECATED) will go away soon! Use --duration instead")(
	    "spb", po::value<size_t>(&spb)->default_value(10000), "samples per buffer")(
	    "rate", po::value<double>(&rate)->default_value(1e6), "rate of incoming samples")(
	    "freq", po::value<double>(&freq)->default_value(2.0E9), "RF center frequency in Hz")(
	    "txgain", po::value<double>(&txgain)->default_value(55), "txgain for the RF chain")(
	    "rxgain", po::value<double>(&rxgain)->default_value(55), "rxgain for the RF chain")(
	    "ant", po::value<std::string>(&ant)->default_value("TX/RX"), "antenna selection")(
	    "subdev", po::value<std::string>(&subdev)->default_value("A:A"), "subdevice specification")(
	    "bw", po::value<double>(&bw)->default_value(100E6), "analog frontend filter bandwidth in Hz")(
	    "ref", po::value<std::string>(&ref)->default_value("gpsdo"), "reference source (internal, external, mimo)")(
	    "wirefmt", po::value<std::string>(&wirefmt)->default_value("sc16"), "wire format (sc8 or sc16)")(
	    "setup", po::value<double>(&setup_time)->default_value(1.0), "seconds of setup time")(
	    "channels", po::value<std::string>(&channel_list)->default_value("0"),
	    "which channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")(
	    "progress", "periodically display short-term bandwidth")("stats", "show average bandwidth on exit")(
	    "sizemap", "track packet size and display breakdown on exit")("null", "run without writing to file")(
	    "continue", "don't abort on a bad packet")("skip-lo", "skip checking LO lock status")(
	    "opmode", po::value<std::string>(&opmode)->default_value("TX/RX"), "node operation mode (TX/RX, TX, RX)");
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	// print the help message
	if (vm.count("help")) {
		std::cout << boost::format("WISCANet USRP Controller %s") % desc << std::endl;
		std::cout << std::endl << "This is the WISCANet USRP Controller\n" << std::endl;
		return ~0;
	}

	bool bw_summary = vm.count("progress") > 0;
	bool stats = vm.count("stats") > 0;
	bool null = vm.count("null") > 0;
	bool enable_size_map = vm.count("sizemap") > 0;
	bool continue_on_bad_packet = vm.count("continue") > 0;

	if (enable_size_map)
		std::cout << "Packet size tracking enabled - will only recv one packet at a time!" << std::endl;

	// Create the RX USRP Device
	std::cout << std::endl;
	std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
	uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

	// Create the TX USRP Device
	std::cout << std::endl;
	std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
	uhd::usrp::multi_usrp::sptr tx_usrp = uhd::usrp::multi_usrp::make(args);

	// always select the subdevice first, the channel mapping affects the other settings
	if (vm.count("subdev")) usrp->set_rx_subdev_spec(subdev);
	if (vm.count("subdev")) tx_usrp->set_tx_subdev_spec(subdev);

	std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;
	std::cout << boost::format("Using Device: %s") % tx_usrp->get_pp_string() << std::endl;

	// set the sample rate
	if (rate <= 0.0) {
		std::cerr << "Please specify a valid sample rate" << std::endl;
		return ~0;
	}
	std::cout << boost::format("Setting TX and RX Rate: %f Msps...") % (rate / 1e6) << std::endl;
	usrp->set_rx_rate(rate);
	std::cout << boost::format("Actual RX Rate: %f Msps...") % (usrp->get_rx_rate() / 1e6) << std::endl << std::endl;
	tx_usrp->set_tx_rate(rate);
	std::cout << boost::format("Actual TX Rate: %f Msps...") % (tx_usrp->get_tx_rate() / 1e6) << std::endl << std::endl;

	// Detect which channels to use
	std::vector<std::string> channel_strings;
	std::vector<size_t> channel_nums;
	boost::split(channel_strings, channel_list, boost::is_any_of("\"',"));
	for (size_t ch = 0; ch < channel_strings.size(); ch++) {
		size_t chan = std::stoi(channel_strings[ch]);
		if (chan >= usrp->get_rx_num_channels()) {
			throw std::runtime_error("Invalid channel(s) specified.");
		} else
			channel_nums.push_back(std::stoi(channel_strings[ch]));
	}
	size_t numRxChannels = usrp->get_rx_num_channels();
	size_t numTxChannels = tx_usrp->get_tx_num_channels();
	uhd::time_spec_t cmd_time;
	// set the center frequency
	if (vm.count("freq")) {  // with default of 0.0 this will always be true
		std::cout << boost::format("Setting RX Freq: %f MHz...") % (freq / 1e6) << std::endl;
		uhd::tune_request_t tune_request(freq);
		// Disable CORDIC rotation, to achieve phase sync between cycles
		tune_request.dsp_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
		tune_request.dsp_freq = 0.0;
		// we will tune the frontends in 200ms from now
		cmd_time = usrp->get_time_now() + uhd::time_spec_t(0.2);
		// sets command time on all devices
		usrp->set_command_time(cmd_time);
		for (size_t i = 0; i < numRxChannels; i++) {
			usrp->set_rx_freq(tune_request, i);
		}
		// end timed commands
		usrp->clear_command_time();
		std::cout << boost::format("Actual RX Freq: %f MHz...") % (usrp->get_rx_freq() / 1e6) << std::endl << std::endl;
		std::cout << boost::format("Setting TX Freq: %f MHz...") % (freq / 1e6) << std::endl;
		uhd::tune_request_t tx_tune_request(freq);
		// Disable CORDIC rotation, to achieve phase sync between cycles
		tune_request.dsp_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
		tune_request.dsp_freq = 0.0;
		// we will tune the frontends in 200ms from now
		cmd_time = tx_usrp->get_time_now() + uhd::time_spec_t(0.2);
		// sets command time on all devices
		tx_usrp->set_command_time(cmd_time);
		for (size_t i = 0; i < numTxChannels; i++) {
			tx_usrp->set_tx_freq(tune_request, i);
		}
		// end timed commands
		tx_usrp->clear_command_time();
		std::cout << boost::format("Actual TX Freq: %f MHz...") % (tx_usrp->get_tx_freq() / 1e6) << std::endl
		          << std::endl;
	}

	// set the rf gain
	if (vm.count("rxgain")) {
		std::cout << boost::format("Setting RX Gain: %f dB...") % rxgain << std::endl;
		// we will tune the frontends in 200ms from now
		cmd_time = usrp->get_time_now() + uhd::time_spec_t(0.2);
		// sets command time on all devices
		usrp->set_command_time(cmd_time);
		for (size_t i = 0; i < numRxChannels; i++) {
			usrp->set_rx_gain(rxgain, i);
		}
		// end timed commands
		usrp->clear_command_time();
		std::cout << boost::format("Actual RX Gain: %f dB...") % usrp->get_rx_gain() << std::endl << std::endl;
	}
	if (vm.count("txgain")) {
		std::cout << boost::format("Setting TX Gain: %f dB...") % txgain << std::endl;
		// we will tune the frontends in 200ms from now
		cmd_time = tx_usrp->get_time_now() + uhd::time_spec_t(0.2);
		// sets command time on all devices
		tx_usrp->set_command_time(cmd_time);
		for (size_t i = 0; i < numTxChannels; i++) {
			tx_usrp->set_tx_gain(txgain, i);
		}
		// end timed commands
		tx_usrp->clear_command_time();
		std::cout << boost::format("Actual TX Gain: %f dB...") % tx_usrp->get_tx_gain() << std::endl << std::endl;
	}

	// set the IF filter bandwidth
	if (vm.count("bw")) {
		std::cout << boost::format("Setting RX Bandwidth: %f MHz...") % (bw / 1e6) << std::endl;
		// we will tune the frontends in 200ms from now
		cmd_time = usrp->get_time_now() + uhd::time_spec_t(0.2);
		// sets command time on all devices
		usrp->set_command_time(cmd_time);
		for (size_t i = 0; i < numRxChannels; i++) {
			usrp->set_rx_bandwidth(bw, i);
		}
		// end timed commands
		usrp->clear_command_time();
		std::cout << boost::format("Actual RX Bandwidth: %f MHz...") % (usrp->get_rx_bandwidth() / 1e6) << std::endl
		          << std::endl;
		std::cout << boost::format("Setting TX Bandwidth: %f MHz...") % (bw / 1e6) << std::endl;
		// we will tune the frontends in 200ms from now
		cmd_time = tx_usrp->get_time_now() + uhd::time_spec_t(0.2);
		// sets command time on all devices
		tx_usrp->set_command_time(cmd_time);
		for (size_t i = 0; i < numTxChannels; i++) {
			tx_usrp->set_tx_bandwidth(bw, i);
		}
		// end timed commands
		tx_usrp->clear_command_time();
		std::cout << boost::format("Actual TX Bandwidth: %f MHz...") % (tx_usrp->get_tx_bandwidth() / 1e6) << std::endl
		          << std::endl;
	}

	// set the antenna
	if (vm.count("ant")) {
		// we will tune the frontends in 200ms from now
		cmd_time = usrp->get_time_now() + uhd::time_spec_t(0.2);
		// sets command time on all devices
		usrp->set_command_time(cmd_time);
		for (size_t i = 0; i < numRxChannels; i++) {
			usrp->set_rx_antenna(ant, i);
		}
		// end timed commands
		usrp->clear_command_time();
		// we will tune the frontends in 200ms from now
		cmd_time = tx_usrp->get_time_now() + uhd::time_spec_t(0.2);
		// sets command time on all devices
		tx_usrp->set_command_time(cmd_time);
		for (size_t i = 0; i < numTxChannels; i++) {
			tx_usrp->set_tx_antenna(ant, i);
		}
		// end timed commands
		tx_usrp->clear_command_time();
	}

	boost::this_thread::sleep(boost::posix_time::seconds{static_cast<long>(setup_time)});  // allow for some setup time

	// check Ref and LO Lock detect
	if (not vm.count("skip-lo")) {
		check_locked_sensor(usrp->get_rx_sensor_names(0), "lo_locked",
		                    boost::bind(&uhd::usrp::multi_usrp::get_rx_sensor, usrp, boost::placeholders::_1, 0), setup_time);
		if (ref == "mimo")
			check_locked_sensor(usrp->get_mboard_sensor_names(0), "mimo_locked",
			                    boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, usrp, boost::placeholders::_1, 0), setup_time);
		if (ref == "external")
			check_locked_sensor(usrp->get_mboard_sensor_names(0), "ref_locked",
			                    boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, usrp, boost::placeholders::_1, 0), setup_time);
		check_locked_sensor(tx_usrp->get_tx_sensor_names(0), "lo_locked",
		                    boost::bind(&uhd::usrp::multi_usrp::get_tx_sensor, tx_usrp, boost::placeholders::_1, 0), setup_time);
		if (ref == "mimo")
			check_locked_sensor(tx_usrp->get_mboard_sensor_names(0), "mimo_locked",
			                    boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, tx_usrp, boost::placeholders::_1, 0), setup_time);
		if (ref == "external")
			check_locked_sensor(tx_usrp->get_mboard_sensor_names(0), "ref_locked",
			                    boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, tx_usrp, boost::placeholders::_1, 0), setup_time);
	}

	if (total_num_samps == 0) {
		std::signal(SIGINT, &sig_int_handler);
		std::cout << "Press Ctrl + C to stop streaming..." << std::endl;
	}

	// synchronization
	synch_to_gps(usrp);
	// This second one isn't actually necessary, because its acting on the same underlying motherboards.
	// synch_to_gps(tx_usrp);

	// start thread
	if (opmode == "TX/RX" || opmode == "TX") {
		boost::thread_group transmit_thread;
		transmit_thread.create_thread(boost::bind(&transmit_worker, tx_usrp, total_num_samps, channel_nums));
	}
	if (opmode == "TX/RX" || opmode == "RX") {
		if (type == "double")
			recv_worker<std::complex<double>> recv_worker_args("fc64");
		else if (type == "float")
			recv_worker<std::complex<float>> recv_worker_args("fc32");
		else if (type == "short")
			recv_worker<std::complex<short>> recv_worker_args("sc16");
		else
			throw std::runtime_error("Unknown type " + type);
	} else {
		while (1) sleep(1);
	}

	// finished
	std::cout << std::endl << "Done!" << std::endl << std::endl;

	return EXIT_SUCCESS;
}
