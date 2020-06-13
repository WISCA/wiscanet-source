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

#include <uhd/types/tune_request.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <fstream>
#include <csignal>
#include <complex>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include "verStr.h"

#define TX_PORT 9940
#define RX_PORT 9944
#define RC_PORT 9945

#define MAX_SLOT 4

namespace po = boost::program_options;
namespace po = boost::program_options;
namespace pt = boost::posix_time;
typedef boost::function<uhd::sensor_value_t (const std::string&)> get_sensor_fn_t;

double sched_unit = 0.25;

static bool stop_signal_called = false;
void sig_int_handler(int){stop_signal_called = true;}

template<typename samp_type> void recv_UMAC_worker(
		uhd::usrp::multi_usrp::sptr usrp,
		const std::string &cpu_format,
		const std::string &wire_format,
		const std::string &file,
		size_t samps_per_buff,
		unsigned long long num_requested_samples,
		double time_requested = 0.0,
		bool bw_summary = false,
		bool stats = false,
		bool null = false,
		bool enable_size_map = false,
		bool continue_on_bad_packet = false,
		size_t tslot = 0
){

	// time variables for scheduling
	uhd::time_spec_t time_now, rxTime;

	size_t num_total_samps = 0;

	// udp socket
	struct sockaddr_in si_other, si_me;
	int sockfd, slen;
	int rLen;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) exit(1);
	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(RX_PORT);
	if (inet_aton("127.0.0.1", (struct in_addr *)&si_other.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(RC_PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sockfd, (struct sockaddr *)&si_me, sizeof(si_me))==-1) {
		fprintf(stderr, "bind() error\n");
		exit(1);
	}

	uhd::rx_metadata_t md;
	std::vector<samp_type> buff(num_requested_samples);
	bool overflow_message = true;

	typedef std::map<size_t,size_t> SizeMap;
	SizeMap mapSizes;

	// wait rx control command
	int rResult;
	char cmdBuf[16];
	double *stime = (double *)cmdBuf;
	double realRxTime, realNowTime, timeDiff;
	double timeout;
	size_t rxLoc;
	size_t rxUnit;

	rResult = recvfrom(sockfd, cmdBuf, 16, 0, (struct sockaddr *)&si_me, (socklen_t *)&slen);
	rxTime = uhd::time_spec_t(*stime);
	time_now = usrp->get_time_now(0);
	realRxTime = rxTime.get_real_secs();
	realNowTime = time_now.get_real_secs();
	printf("rx ctrl cmd rxTime=%f, time_now =%f\n", realRxTime, realNowTime);

	if(realRxTime < realNowTime) {
		printf("error : rx time already passed\n");
		exit(1);
	}

	timeout = realRxTime - realNowTime + 0.1;

	// create a receive streamer
	uhd::stream_args_t stream_args(cpu_format, wire_format);
	uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

	uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
	stream_cmd.num_samps = num_requested_samples;

	rxUnit = rx_stream->get_max_num_samps();

	rxUnit = num_requested_samples;

	while(1) {

		// rx streamer cmd
		num_total_samps = 0;
		stream_cmd.stream_now = false;
		stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE;
		stream_cmd.time_spec = rxTime;

		rx_stream->issue_stream_cmd(stream_cmd);

		rxLoc = 0;

		while(num_requested_samples > num_total_samps) {

			/*
            size_t num_rx_samps = rx_stream->recv(&buff.front(), buff.size(), md, timeout, enable_size_map);
			 */
			size_t num_rx_samps = rx_stream->recv(&buff[rxLoc], rxUnit, md, timeout, enable_size_map);
			rxLoc += num_rx_samps;

			if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
				std::cout << boost::format("Timeout while streaming") << std::endl;
				break;
			}
			if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW){
				if (overflow_message) {
					overflow_message = false;
					std::cerr << boost::format(
							"Got an overflow indication. Please consider the following:\n"
							"  Your write medium must sustain a rate of %fMB/s.\n"
							"  Dropped samples will not be written to the file.\n"
							"  Please modify this example for your purposes.\n"
							"  This message will not appear again.\n"
					) % (usrp->get_rx_rate()*sizeof(samp_type)/1e6);
				}
				continue;
			}
			if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE){
				std::string error = str(boost::format("Receiver error: %s") % md.strerror());
				if (continue_on_bad_packet){
					std::cerr << error << std::endl;
					continue;
				}
				else throw std::runtime_error(error);
			}
			if (enable_size_map) {
				SizeMap::iterator it = mapSizes.find(num_rx_samps);
				if (it == mapSizes.end()) mapSizes[num_rx_samps] = 0;
				mapSizes[num_rx_samps] += 1;
			}

			num_total_samps += num_rx_samps;
		}

		printf("-- num_total_samps = %d\n", num_total_samps);

		/*
{
for (size_t i=0; i<num_total_samps; i++) {
    printf("%d %d\n", std::real(buff[i]), std::imag(buff[i]));
}
}
		 */

		time_now = usrp->get_time_now(0);

		// send rx packet to matlab through udp socket
		size_t txSamps = num_total_samps;

//		size_t count=0, txTotal=0, txUnit=1000, txLen=0, txResult=0;
		size_t count=0, txTotal=0, txUnit=4000, txLen=0, txResult=0;

		while(txTotal < txSamps) {
			if((txSamps - txTotal) > txUnit) txLen = txUnit;
			else txLen = txSamps - txTotal;
			txResult = sendto(sockfd, (&buff.front() + (txUnit * count)), txLen * sizeof(samp_type), 0, (struct sockaddr *)&si_other, sizeof(si_other));
			if(txResult == -1) break;
			count++;
			txTotal += (txResult/sizeof(samp_type));
			usleep(20); // intentional delay for proper context switching
			//printf("-- RX : txTotal = %d samps\n", txTotal);
		}
		//printf("-- RX : txTotal = %d samps\n", txTotal);

		// send zero size termination packet
		sendto(sockfd, &buff.front(), 0, 0, (struct sockaddr *)&si_other, sizeof(si_other));
		//printf("-- RX : zero size packet\n");

		printf("RX : time=%f, len=%d(smpls)\n", time_now.get_real_secs(), txTotal);

		// wait RX control command
		rResult = recvfrom(sockfd, cmdBuf, 1024, 0, (struct sockaddr *)&si_me, (socklen_t *)&slen);
		rxTime = uhd::time_spec_t(*stime);
		time_now = usrp->get_time_now(0);
		realRxTime = rxTime.get_real_secs();
		realNowTime = time_now.get_real_secs();

		printf("rx ctrl cmd rxTime=%f, time_now =%f\n", realRxTime, realNowTime);

		timeout = realRxTime - realNowTime + 0.1;

	}
}

template<typename samp_type> void recv_TDMA_worker(
		uhd::usrp::multi_usrp::sptr usrp,
		const std::string &cpu_format,
		const std::string &wire_format,
		const std::string &file,
		size_t samps_per_buff,
		unsigned long long num_requested_samples,
		double time_requested = 0.0,
		bool bw_summary = false,
		bool stats = false,
		bool null = false,
		bool enable_size_map = false,
		bool continue_on_bad_packet = false,
		size_t tslot = 0
){

	printf("================== RX TDMA worker (tslot:%d) =================\n", tslot);

	// time variables for scheduling
	uhd::time_spec_t last_pps, time_now, rxTime;
	size_t cslot;

	unsigned long long num_total_samps = 0;

	// udp data socket
	struct sockaddr_in si_other, si_me;
	int sockfd, slen;
	int rLen;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) exit(1);
	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(RX_PORT);
	if (inet_aton("127.0.0.1", (struct in_addr *)&si_other.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(RC_PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sockfd, (struct sockaddr *)&si_me, sizeof(si_me))==-1) {
		fprintf(stderr, "bind() error\n");
		exit(1);
	}

	uhd::rx_metadata_t md;
	std::vector<samp_type> buff(num_requested_samples + 5000);
	bool overflow_message = true;
	int rxUnit;
	int rxLoc;

	typedef std::map<size_t,size_t> SizeMap;
	SizeMap mapSizes;

	// wait rx control command
	int rResult;
	char cmdBuf[1024];
	printf("wait RX start command\n");
	rResult = recvfrom(sockfd, cmdBuf, 1024, 0, (struct sockaddr *)&si_me, (socklen_t *)&slen);
	printf("recv RX start command\n");

	// create a receive streamer
	uhd::stream_args_t stream_args(cpu_format,wire_format);
	uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

	// setup streamer
	uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
	stream_cmd.num_samps = num_requested_samples;

	rxUnit = rx_stream->get_max_num_samps();
	cslot = 0;

	while(1) {

		// compute rxTime
		last_pps = usrp->get_time_last_pps(0);
		time_now = usrp->get_time_now(0);
		if(cslot == 0) rxTime = last_pps + uhd::time_spec_t(1.0); // next pps
		else rxTime = rxTime + uhd::time_spec_t(sched_unit);

		// rx streamer cmd
		num_total_samps = 0;
		stream_cmd.stream_now = false;
		stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE;
		stream_cmd.time_spec = rxTime;
		rx_stream->issue_stream_cmd(stream_cmd);

		//printf("cslot=%d, %f, %f, %f\n", cslot, last_pps, time_now, rxTime);

		rxLoc = 0;

		while(not stop_signal_called and (num_requested_samples > num_total_samps or num_requested_samples == 0)) {

			/*
            size_t num_rx_samps = rx_stream->recv(&buff.front(), buff.size(), md, 3.0, enable_size_map);
			 */
			size_t num_rx_samps = rx_stream->recv(&buff[rxLoc], rxUnit, md, 3.0, enable_size_map);
			rxLoc += num_rx_samps;


			if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
				std::cout << boost::format("Timeout while streaming") << std::endl;
				break;
			}
			if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW){
				if (overflow_message) {
					overflow_message = false;
					std::cerr << boost::format(
							"Got an overflow indication. Please consider the following:\n"
							"  Your write medium must sustain a rate of %fMB/s.\n"
							"  Dropped samples will not be written to the file.\n"
							"  Please modify this example for your purposes.\n"
							"  This message will not appear again.\n"
					) % (usrp->get_rx_rate()*sizeof(samp_type)/1e6);
				}
				continue;
			}
			if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE){
				std::string error = str(boost::format("Receiver error: %s") % md.strerror());
				if (continue_on_bad_packet){
					std::cerr << error << std::endl;
					continue;
				}
				else throw std::runtime_error(error);
			}
			if (enable_size_map) {
				SizeMap::iterator it = mapSizes.find(num_rx_samps);
				if (it == mapSizes.end()) mapSizes[num_rx_samps] = 0;
				mapSizes[num_rx_samps] += 1;
			}

			num_total_samps += num_rx_samps;
		}

		// send rx packet to matlab through udp socket
		if(cslot != tslot) {
			int txSamps = num_total_samps;
			int count=0, txTotal=0, txUnit=1024, txLen=0, txResult=0;
			/*
for(size_t i=0; i<num_total_samps; i++) {
    printf("%d %d\n", std::real(buff[i]), std::imag(buff[i]));
}
			 */
			while(txTotal < txSamps) {
				if((txSamps - txTotal) > txUnit) txLen = txUnit;
				else txLen = txSamps - txTotal;
				txResult = sendto(sockfd, (&buff.front() + (txUnit * count)), txLen * sizeof(samp_type), 0, (struct sockaddr *)&si_other, sizeof(si_other));
				if(txResult == -1) break;
				count++;
				txTotal += (txResult/sizeof(samp_type));
				usleep(200); // intentional delay for proper context switching
			}

			// send zero size termination packet
			sendto(sockfd, &buff.front(), 0, 0, (struct sockaddr *)&si_other, sizeof(si_other));
			printf("rx : slot=%d, time=%f, len=%d(smpls)\n", cslot, rxTime.get_real_secs(), txTotal);
		}

		cslot = (cslot + 1) % MAX_SLOT;
	}
}

typedef boost::function<uhd::sensor_value_t (const std::string&)> get_sensor_fn_t;

bool check_locked_sensor(std::vector<std::string> sensor_names, const char* sensor_name, get_sensor_fn_t get_sensor_fn, double setup_time){
	if (std::find(sensor_names.begin(), sensor_names.end(), sensor_name) == sensor_names.end())
		return false;

	boost::system_time start = boost::get_system_time();
	boost::system_time first_lock_time;

	std::cout << boost::format("Waiting for \"%s\": ") % sensor_name;
	std::cout.flush();

	while (true) {
		if ((not first_lock_time.is_not_a_date_time()) and
				(boost::get_system_time() > (first_lock_time + boost::posix_time::seconds(setup_time))))
		{
			std::cout << " locked." << std::endl;
			break;
		}
		if (get_sensor_fn(sensor_name).to_bool()){
			if (first_lock_time.is_not_a_date_time())
				first_lock_time = boost::get_system_time();
			std::cout << "+";
			std::cout.flush();
		}
		else {
			first_lock_time = boost::system_time();	//reset to 'not a date time'

			if (boost::get_system_time() > (start + boost::posix_time::seconds(setup_time))){
				std::cout << std::endl;
				throw std::runtime_error(str(boost::format("timed out waiting for consecutive locks on sensor \"%s\"") % sensor_name));
			}
			std::cout << "_";
			std::cout.flush();
		}
		boost::this_thread::sleep(boost::posix_time::milliseconds(100));
	}
	std::cout << std::endl;
	return true;
}

uhd::time_spec_t get_system_time(void){
    pt::ptime time_now = pt::microsec_clock::universal_time();
    pt::time_duration time_dur = time_now - pt::from_time_t(0);
    return uhd::time_spec_t(
        time_t(time_dur.total_seconds()),
        long(time_dur.fractional_seconds()),
        double(pt::time_duration::ticks_per_second())
    );
}

int synch_to_gps(
		uhd::usrp::multi_usrp::sptr usrp
)
{
	int gps_unlocked = 0;

	// Lock mboard clocks
	usrp->set_clock_source("gpsdo");
	usrp->set_time_source("gpsdo");

	std::cout << "Synchronizing mboard " << 0 << ": " << usrp->get_mboard_name(0) << std::endl;
	std::vector<std::string> sensor_names = usrp->get_mboard_sensor_names(0);
	if(std::find(sensor_names.begin(), sensor_names.end(), "ref_locked") != sensor_names.end())
	{
		std::cout << "Waiting for reference lock..." << std::flush;
		bool ref_locked = false;
		for (int i = 0; i < 30 and not ref_locked; i++)
		{
			ref_locked = usrp->get_mboard_sensor("ref_locked", 0).to_bool();
			if (not ref_locked)
			{
				std::cout << "." << std::flush;
				boost::this_thread::sleep(boost::posix_time::seconds(1));
			}
		}
		if(ref_locked)
		{
			std::cout << "LOCKED" << std::endl;
		} else {
			std::cout << "FAILED" << std::endl;
			throw uhd::runtime_error("Failed to lock to GPSDO 10 MHz Reference");
		}
	}
	else
	{
		std::cout << boost::format("ref_locked sensor not present on this board.\n");
	}

	//Wait for GPS lock
	while(1) {
		bool gps_locked = usrp->get_mboard_sensor("gps_locked", 0).to_bool();
		if(gps_locked)
		{
			std::cout << boost::format("GPS Locked\n");
			break;
		}
		else
		{
			std::cerr << "WARNING:  GPS not locked - time will not be accurate until locked" << std::endl;
			//            std::cout << "WARNING:  GPS not locked - time will not be accurate until locked" << std::endl;
			if(gps_unlocked++ > 10) {
				//                std::cerr << "Terminate uControl because of GPS lock fail\n";
				//                exit(1);
				break;
			}
		}
	}

	//Set to GPS time
	uhd::time_spec_t gps_time = uhd::time_spec_t(time_t(usrp->get_mboard_sensor("gps_time", 0).to_int()));
	usrp->set_time_next_pps(gps_time+1.0, 0);

	//Wait for it to apply
	//The wait is 2 seconds because N-Series has a known issue where
	//the time at the last PPS does not properly update at the PPS edge
	//when the time is actually set.
	boost::this_thread::sleep(boost::posix_time::seconds(2));

	//Check times
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

void transmit_UMAC_worker(
		uhd::usrp::multi_usrp::sptr usrp,
		size_t total_num_samps,
		size_t tslot
) {

	std::cout << "================== TX UMAC worker =================\n";

	// variables for timer
	uhd::time_spec_t txTime, time_now, prev_txtime;
	double tMargin = 0.001; // 1msec

	prev_txtime = uhd::time_spec_t(0.0);

	// metadata for transmission
	uhd::tx_metadata_t md;

	// udp socket
	struct sockaddr_in si_me;
	int sockfd, slen;
	size_t rLen;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) exit(1);
	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(TX_PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sockfd, (struct sockaddr *)&si_me, sizeof(si_me))==-1) exit(1);

	//create a transmit streamer
	uhd::stream_args_t stream_args("fc32", "sc16");
	uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

	// buffer for transmission
	std::vector<std::complex<float> > buff(total_num_samps + 1000); // 1000 for control information

	char *udpBuf;
	size_t rResult;
	double *start_time;
	double timeout;
	size_t txUnit = tx_stream->get_max_num_samps();
	size_t txBufLoc = 0;
	size_t txLen;
	size_t rSamLen;

	while(1) {
		udpBuf = (char *)&buff[0];
		rLen = 0;
		rResult = 0;

		do {
			udpBuf += rResult;
			rLen += rResult;
			rResult = recvfrom(sockfd, udpBuf, buff.size(), 0, (struct sockaddr *)&si_me, (socklen_t *)&slen);
		} while (rResult > 0);
		rSamLen = rLen / sizeof(std::complex<float>);
		printf("TX: rx %d byte (%d samples) from MATLAB\n", rLen, rSamLen);

		time_now = usrp->get_time_now(0);
		start_time = (double *)(&buff[0] + rSamLen - 1);
		//printf("start_time = %f, prev_time = %f, time_now = %f\n", *start_time, prev_txtime.get_real_secs(), time_now.get_real_secs());

		// txtime management
		txTime = uhd::time_spec_t(*start_time);
		if(prev_txtime.get_real_secs() > time_now.get_real_secs()) {
			printf("scheduling error prev_time = %f, time_now = %f\n",
					prev_txtime.get_real_secs(), time_now.get_real_secs());
			exit(1);
		}
		prev_txtime = txTime;

		// md setup
		md.start_of_burst = true;
		md.end_of_burst = false;
		md.has_time_spec = true;
		md.time_spec = txTime;

		// send the contents of tx buffer
		txBufLoc = 0;
		timeout = txTime.get_real_secs() - time_now.get_real_secs() + 0.1;

//		while(txBufLoc < rSamLen) {
//			if((rSamLen - txBufLoc) > txUnit) txLen = txUnit;
//			else txLen = (rSamLen - txBufLoc);
//			tx_stream->send(&buff[txBufLoc], txLen, md, timeout);
//			printf("assigned_tx_time = %f, time_now = %f\n",
//					prev_txtime.get_real_secs(), time_now.get_real_secs());
//			//printf("txBufLoc = %d, txLen = %d\n", txBufLoc, txLen);
//			md.start_of_burst = false;
//			md.end_of_burst = false;
//			md.has_time_spec = false;
//			txBufLoc += txLen;
//		}

		txLen = tx_stream->send(&buff.front(), total_num_samps, md, timeout);
		if(txLen < total_num_samps) {
			printf("tx error actual_tx_len = %d, request_len = %d\n",
					txLen, total_num_samps);
			exit(1);
		}
			printf("assigned_tx_time = %f, time_now = %f\n",
					prev_txtime.get_real_secs(), time_now.get_real_secs());
			//printf("txBufLoc = %d, txLen = %d\n", txBufLoc, txLen);


		// send a mini EOB packet
		md.start_of_burst = false;
		md.has_time_spec = false;
		md.end_of_burst = true;
		tx_stream->send("", 0, md);
	}
}

void transmit_TDMA_worker(
		uhd::usrp::multi_usrp::sptr usrp,
		size_t total_num_samps,
		size_t tslot
) {

	printf("================== TX TDMA worker (slot:%d) =================\n", tslot);

	// variables for timer
	uhd::time_spec_t last_pps, time_now, txTime;
	double tMargin = 0.001; // 1msec

	// metadata for transmission
	uhd::tx_metadata_t md;

	// udp socket
	struct sockaddr_in si_me;
	int sockfd, slen;
	int rLen;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) exit(1);
	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(TX_PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sockfd, (struct sockaddr *)&si_me, sizeof(si_me))==-1) exit(1);

	//create a transmit streamer
	uhd::stream_args_t stream_args("fc32", "sc16");
	uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

	// buffer for transmission
	std::vector<std::complex<float> > buff(total_num_samps + 1000);

	char *udpBuf;
	int rResult;
	double timeout;
	int txUnit = tx_stream->get_max_num_samps();
	int txBufLoc = 0;
	int txLen;
	int rSamLen;
	int skipFlag = 0;

	while(1) {
		udpBuf = (char *)&buff[0];
		rLen = 0;
		rResult = 0;
		skipFlag = 0;
		do {
			udpBuf += rResult;
			rLen += rResult;
			/*
printf("rResult = %d, rLen = %d\n", rResult, rLen);
			 */
			rResult = recvfrom(sockfd, udpBuf, buff.size(), 0, (struct sockaddr *)&si_me, (socklen_t *)&slen);
		} while (rResult > 0);
		rSamLen = rLen / sizeof(std::complex<float>);
		printf("rx %d byte, (%d samps) from MATLAB\n", rLen, rSamLen);

		/*
for(size_t i=0; i<rSamLen; i++) {
    printf("%d  %f %f\n", i, std::real(buff[i]), std::imag(buff[i]));
}
		 */

		last_pps = usrp->get_time_last_pps(0);
		time_now = usrp->get_time_now(0);
		if((time_now - last_pps) < 0.998) {
			txTime = last_pps + uhd::time_spec_t(1.000 + tslot * sched_unit + tMargin);
		}
		else {
			skipFlag = 1;
			txTime = last_pps + uhd::time_spec_t(2.000 + tslot * sched_unit + tMargin);
		}
		printf("last_pps=%f, time_now=%f, txTime=%f\n", last_pps.get_real_secs(), time_now.get_real_secs(), txTime.get_real_secs());

		md.start_of_burst = true;
		md.end_of_burst = false;
		md.has_time_spec = true;
		md.time_spec = txTime;

		// send the contents of tx buffer
		txBufLoc = 0;
		timeout = txTime.get_real_secs() - time_now.get_real_secs() + 0.1;
		while(txBufLoc < rSamLen) {
			if((rSamLen - txBufLoc) > txUnit) txLen = txUnit;
			else txLen = (rSamLen - txBufLoc);
			tx_stream->send(&buff[txBufLoc], txLen, md, timeout);
			//printf("rSamLen = %d, txBufLoc = %d, txLen = %d\n", rSamLen, txBufLoc, txLen);
			md.has_time_spec = false;
			txBufLoc += txLen;
		}
		printf("rSamLen = %d, txBufLoc = %d, txLen = %d\n", rSamLen, txBufLoc, txLen);

		// send a mini EOB packet
		md.start_of_burst = false;
		md.has_time_spec = false;
		md.end_of_burst = true;
		tx_stream->send("", 0, md);

		if(skipFlag) printf("skip\n");
	}
}

/*int UHD_SAFE_MAIN(int argc, char *argv[]){

	uhd::set_thread_priority_safe();

	std::cerr << verStr << ", " << dateStr << std::endl;
	std::cout << verStr << ", " << dateStr << std::endl;

	//variables to be set by po
	std::string args, file, type, ant, subdev, ref, wirefmt;
	size_t total_num_samps, spb, tslot, chan = 0;
	double rate, freq, txgain, rxgain, bw, total_time, setup_time;
	std::string opmode, macmode;

	//setup the program options
	po::options_description desc("Allowed options");
	desc.add_options()
        		("help", "help message")
				("args", po::value<std::string>(&args)->default_value("type=b200"), "multi uhd device address args")
				("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"), "name of the file to write binary samples to")
				("type", po::value<std::string>(&type)->default_value("short"), "sample type: double, float, or short")
				("nsamps", po::value<size_t>(&total_num_samps)->default_value(5000), "total number of samples to receive")
				("duration", po::value<double>(&total_time)->default_value(0), "total number of seconds to receive")
				("time", po::value<double>(&total_time), "(DEPRECATED) will go away soon! Use --duration instead")
				("spb", po::value<size_t>(&spb)->default_value(10000), "samples per buffer")
				("rate", po::value<double>(&rate)->default_value(1e6), "rate of incoming samples")
				("freq", po::value<double>(&freq)->default_value(2.0E9), "RF center frequency in Hz")
				("txgain", po::value<double>(&txgain)->default_value(55), "txgain for the RF chain")
				("rxgain", po::value<double>(&rxgain)->default_value(55), "rxgain for the RF chain")
				("ant", po::value<std::string>(&ant)->default_value("TX/RX"), "antenna selection")
				("subdev", po::value<std::string>(&subdev)->default_value("A:A"), "subdevice specification")
				("bw", po::value<double>(&bw)->default_value(100E6), "analog frontend filter bandwidth in Hz")
				("ref", po::value<std::string>(&ref)->default_value("gpsdo"), "reference source (internal, external, mimo)")
				("wirefmt", po::value<std::string>(&wirefmt)->default_value("sc16"), "wire format (sc8 or sc16)")
				("setup", po::value<double>(&setup_time)->default_value(1.0), "seconds of setup time")
				("progress", "periodically display short-term bandwidth")
				("stats", "show average bandwidth on exit")
				("sizemap", "track packet size and display breakdown on exit")
				("null", "run without writing to file")
				("continue", "don't abort on a bad packet")
				("skip-lo", "skip checking LO lock status")
				("int-n", "tune USRP with integer-N tuning")
				("opmode", po::value<std::string>(&opmode)->default_value("TX/RX"), "node operation mode (TX/RX, TX, RX)")
				("macmode", po::value<std::string>(&macmode)->default_value("TDMA"), "node operation mode (UMAC, TDMA)")
				("tslot", po::value<size_t>(&tslot)->default_value(0), "TDMA tx timeslot");
	;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	//print the help message
	if (vm.count("help")) {
		std::cout << boost::format("UHD RX samples to file %s") % desc << std::endl;
		std::cout
		<< std::endl
		<< "This application streams data from a single channel of a USRP device to a file.\n"
		<< std::endl;
		return ~0;
	}

	bool bw_summary = vm.count("progress") > 0;
	bool stats = vm.count("stats") > 0;
	bool null = vm.count("null") > 0;
	bool enable_size_map = vm.count("sizemap") > 0;
	bool continue_on_bad_packet = vm.count("continue") > 0;

	if (enable_size_map)
		std::cout << "Packet size tracking enabled - will only recv one packet at a time!" << std::endl;

	//create a usrp device
	std::cout << std::endl;
	std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
	uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

	//    std::cout << std::endl;
	//    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
	//    uhd::usrp::multi_usrp::sptr tx_usrp = uhd::usrp::multi_usrp::make(args);

	//always select the subdevice first, the channel mapping affects the other settings
	if (vm.count("subdev"))
	{
		usrp->set_rx_subdev_spec(subdev);
		//    if (vm.count("subdev")) tx_usrp->set_tx_subdev_spec(subdev);
		usrp->set_tx_subdev_spec(subdev);
	}

	std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;
	//    std::cout << boost::format("Using Device: %s") % tx_usrp->get_pp_string() << std::endl;

	//set the sample rate
	if (rate <= 0.0){
		std::cerr << "Please specify a valid sample rate" << std::endl;
		return ~0;
	}
	std::cout << boost::format("Setting RX Rate: %f Msps...") % (rate/1e6) << std::endl;
	usrp->set_rx_rate(rate);
	std::cout << boost::format("Actual RX Rate: %f Msps...") % (usrp->get_rx_rate()/1e6) << std::endl << std::endl;
	//    tx_usrp->set_tx_rate(rate);
	//    std::cout << boost::format("Actual RX Rate: %f Msps...") % (tx_usrp->get_tx_rate()/1e6) << std::endl << std::endl;
	usrp->set_tx_rate(rate);
	std::cout << boost::format("Actual RX Rate: %f Msps...") % (usrp->get_tx_rate()/1e6) << std::endl << std::endl;

	//set the center frequency
	if (vm.count("freq"))
	{ //with default of 0.0 this will always be true
		std::cout << boost::format("Setting RX TX Freq: %f MHz...") % (freq/1e6) << std::endl;
		uhd::tune_request_t tune_request(freq);
		if(vm.count("int-n")) tune_request.args = uhd::device_addr_t("mode_n=integer");
		usrp->set_rx_freq(tune_request);
		std::cout << boost::format("Actual RX Freq: %f MHz...") % (usrp->get_rx_freq()/1e6) << std::endl << std::endl;
		usrp->set_tx_freq(tune_request);
		std::cout << boost::format("Actual TX Freq: %f MHz...") % (usrp->get_tx_freq()/1e6) << std::endl << std::endl;
	}

	//set the rf gain
	if (vm.count("rxgain")) {
		std::cout << boost::format("Setting RX Gain: %f dB...") % rxgain << std::endl;
		usrp->set_rx_gain(rxgain, chan);
		std::cout << boost::format("Actual RX Gain: %f dB...") % usrp->get_rx_gain() << std::endl << std::endl;
	}
	if (vm.count("txgain")) {
		std::cout << boost::format("Setting TX Gain: %f dB...") % txgain << std::endl;
		//        tx_usrp->set_tx_gain(txgain, chan);
		//        std::cout << boost::format("Actual TX Gain: %f dB...") % tx_usrp->get_tx_gain() << std::endl << std::endl;
		usrp->set_tx_gain(txgain,chan);
		std::cout << boost::format("Actual TX Gain: %f dB...") % usrp->get_tx_gain() << std::endl << std::endl;

	}

	//set the IF filter bandwidth
	if (vm.count("bw")) {
		std::cout << boost::format("Setting RX Bandwidth: %f MHz...") % (bw/1e6) << std::endl;
		usrp->set_rx_bandwidth(bw, chan);
		std::cout << boost::format("Actual RX Bandwidth: %f MHz...") % (usrp->get_rx_bandwidth()/1e6) << std::endl << std::endl;
		//        std::cout << boost::format("Setting TX Bandwidth: %f MHz...") % (bw/1e6) << std::endl;
		//        tx_usrp->set_tx_bandwidth(bw, chan);
		//        std::cout << boost::format("Actual TX Bandwidth: %f MHz...") % (tx_usrp->get_tx_bandwidth()/1e6) << std::endl << std::endl;

		std::cout << boost::format("Setting TX Bandwidth: %f MHz...") % (bw/1e6) << std::endl;
		usrp->set_tx_bandwidth(bw, chan);
		std::cout << boost::format("Actual TX Bandwidth: %f MHz...") % (usrp->get_tx_bandwidth()/1e6) << std::endl << std::endl;

	}

	//set the antenna
	if (vm.count("ant")) {
		usrp->set_rx_antenna(ant, chan);
		//        tx_usrp->set_tx_antenna(ant, chan);
		usrp->set_tx_antenna(ant,chan);
	}

	boost::this_thread::sleep(boost::posix_time::seconds(setup_time)); //allow for some setup time

	//check Ref and LO Lock detect
	if (not vm.count("skip-lo")){
		check_locked_sensor(usrp->get_rx_sensor_names(0), "lo_locked", boost::bind(&uhd::usrp::multi_usrp::get_rx_sensor, usrp, _1, 0), setup_time);
		if (ref == "mimo")
			check_locked_sensor(usrp->get_mboard_sensor_names(0), "mimo_locked", boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, usrp, _1, 0), setup_time);
		if (ref == "external")
			//            check_locked_sensor(usrp->get_mboard_sensor_names(0), "ref_locked", boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, usrp, _1, 0), setup_time);
			//        check_locked_sensor(tx_usrp->get_tx_sensor_names(0), "lo_locked", boost::bind(&uhd::usrp::multi_usrp::get_tx_sensor, tx_usrp, _1, 0), setup_time);
			//        if (ref == "mimo")
			//            check_locked_sensor(tx_usrp->get_mboard_sensor_names(0), "mimo_locked", boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, tx_usrp, _1, 0), setup_time);
			//        if (ref == "external")
			//            check_locked_sensor(tx_usrp->get_mboard_sensor_names(0), "ref_locked", boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, tx_usrp, _1, 0), setup_time);
		{
			check_locked_sensor(usrp->get_mboard_sensor_names(0), "ref_locked", boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, usrp, _1, 0), setup_time);
			uhd::time_spec_t last_pps_time = usrp->get_time_last_pps();
			while (last_pps_time.get_real_secs()
					== usrp->get_time_last_pps().get_real_secs()) {
				boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				//				usleep(100);
			}
			// get current local_time
			//			uhd::time_spec_t local_time = last_pps_time.get_system_time();
			uhd::time_spec_t local_time = get_system_time();
			// set the local_time+1 on the next pps edge
			usrp->set_time_next_pps(uhd::time_spec_t(double(local_time.get_real_secs() + 1)), 0);

			last_pps_time = usrp->get_time_last_pps();

			// wait for change to take effect
			while (last_pps_time.get_real_secs()
					== usrp->get_time_last_pps().get_real_secs()) {
				boost::this_thread::sleep(boost::posix_time::milliseconds(100));
				//				usleep(100);
			}
			std::cout<< boost::format("last_pps_time: %d sec...")% usrp->get_time_last_pps().get_real_secs() << std::endl;
			std::cout<< boost::format("current_time: %d sec...")% usrp->get_time_now().get_real_secs() << std::endl;
		}
		if (ref == "gpsdo"){
			synch_to_gps(usrp);
			check_locked_sensor(usrp->get_mboard_sensor_names(0), "gps_locked", boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, usrp, _1, 0), setup_time);
		}

	}

	if (total_num_samps == 0){
		std::signal(SIGINT, &sig_int_handler);
		std::cout << "Press Ctrl + C to stop streaming..." << std::endl;
	}

	// synchronization
	//    synch_to_gps(usrp);
	//    synch_to_gps(tx_usrp);

	// start thread
	if(opmode == "TX/RX" || opmode == "TX") {
		boost::thread_group transmit_thread;
		if(macmode == "TDMA") {
			//transmit_thread.create_thread(boost::bind(&transmit_TDMA_worker, tx_usrp, total_num_samps, tslot));
			transmit_thread.create_thread(boost::bind(&transmit_TDMA_worker, usrp, total_num_samps, tslot));

		}
		else if(macmode == "UMAC") {
			//transmit_thread.create_thread(boost::bind(&transmit_UMAC_worker, tx_usrp, total_num_samps, tslot));
			transmit_thread.create_thread(boost::bind(&transmit_UMAC_worker, usrp, total_num_samps, tslot));
		}
		else {
			printf("unknown macmode = %s\n", macmode);
			exit(1);
		}
	}
	if(opmode == "TX/RX" || opmode == "RX") {
#define recv_worker_args(format) \
		(usrp, format, wirefmt, file, spb, total_num_samps, total_time, bw_summary, stats, null, enable_size_map, continue_on_bad_packet, tslot)
		if(macmode == "TDMA") {
			if (type == "double") recv_TDMA_worker<std::complex<double> >recv_worker_args("fc64");
			else if (type == "float") recv_TDMA_worker<std::complex<float> >recv_worker_args("fc32");
			else if (type == "short") recv_TDMA_worker<std::complex<short> >recv_worker_args("sc16");
			else throw std::runtime_error("Unknown type " + type);
		}
		else if(macmode == "UMAC") {
			if (type == "double") recv_UMAC_worker<std::complex<double> >recv_worker_args("fc64");
			else if (type == "float") recv_UMAC_worker<std::complex<float> >recv_worker_args("fc32");
			else if (type == "short") recv_UMAC_worker<std::complex<short> >recv_worker_args("sc16");
			else throw std::runtime_error("Unknown type " + type);
		}
		else {
			printf("unknown macmode = %s\n", macmode);
			exit(1);
		}
	}
	else {
		while(1) sleep(1);
	}

	//finished
	std::cout << std::endl << "Done!" << std::endl << std::endl;

	return EXIT_SUCCESS;
}*/

int UHD_SAFE_MAIN(int argc, char *argv[]){

    uhd::set_thread_priority_safe();

    std::cerr << verStr << ", " << dateStr << std::endl;
    std::cout << verStr << ", " << dateStr << std::endl;

    //variables to be set by po
    std::string args, file, type, ant, subdev, ref, wirefmt;
    size_t total_num_samps, spb, tslot, chan = 0;
    double rate, freq, txgain, rxgain, bw, total_time, setup_time;
    std::string opmode, macmode;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value("type=b200"), "multi uhd device address args")
        ("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"), "name of the file to write binary samples to")
        ("type", po::value<std::string>(&type)->default_value("short"), "sample type: double, float, or short")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(5000), "total number of samples to receive")
        ("duration", po::value<double>(&total_time)->default_value(0), "total number of seconds to receive")
        ("time", po::value<double>(&total_time), "(DEPRECATED) will go away soon! Use --duration instead")
        ("spb", po::value<size_t>(&spb)->default_value(10000), "samples per buffer")
        ("rate", po::value<double>(&rate)->default_value(1e6), "rate of incoming samples")
        ("freq", po::value<double>(&freq)->default_value(2.0E9), "RF center frequency in Hz")
        ("txgain", po::value<double>(&txgain)->default_value(55), "txgain for the RF chain")
        ("rxgain", po::value<double>(&rxgain)->default_value(55), "rxgain for the RF chain")
        ("ant", po::value<std::string>(&ant)->default_value("TX/RX"), "antenna selection")
        ("subdev", po::value<std::string>(&subdev)->default_value("A:A"), "subdevice specification")
        ("bw", po::value<double>(&bw)->default_value(100E6), "analog frontend filter bandwidth in Hz")
        ("ref", po::value<std::string>(&ref)->default_value("gpsdo"), "reference source (internal, external, mimo)")
        ("wirefmt", po::value<std::string>(&wirefmt)->default_value("sc16"), "wire format (sc8 or sc16)")
        ("setup", po::value<double>(&setup_time)->default_value(1.0), "seconds of setup time")
        ("progress", "periodically display short-term bandwidth")
        ("stats", "show average bandwidth on exit")
        ("sizemap", "track packet size and display breakdown on exit")
        ("null", "run without writing to file")
        ("continue", "don't abort on a bad packet")
        ("skip-lo", "skip checking LO lock status")
        ("int-n", "tune USRP with integer-N tuning")
        ("opmode", po::value<std::string>(&opmode)->default_value("TX/RX"), "node operation mode (TX/RX, TX, RX)")
        ("macmode", po::value<std::string>(&macmode)->default_value("TDMA"), "node operation mode (UMAC, TDMA)")
        ("tslot", po::value<size_t>(&tslot)->default_value(0), "TDMA tx timeslot");
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")) {
        std::cout << boost::format("UHD RX samples to file %s") % desc << std::endl;
        std::cout
            << std::endl
            << "This application streams data from a single channel of a USRP device to a file.\n"
            << std::endl;
        return ~0;
    }

    bool bw_summary = vm.count("progress") > 0;
    bool stats = vm.count("stats") > 0;
    bool null = vm.count("null") > 0;
    bool enable_size_map = vm.count("sizemap") > 0;
    bool continue_on_bad_packet = vm.count("continue") > 0;

    if (enable_size_map)
        std::cout << "Packet size tracking enabled - will only recv one packet at a time!" << std::endl;

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr tx_usrp = uhd::usrp::multi_usrp::make(args);

    //always select the subdevice first, the channel mapping affects the other settings
    if (vm.count("subdev")) usrp->set_rx_subdev_spec(subdev);
    if (vm.count("subdev")) tx_usrp->set_tx_subdev_spec(subdev);

    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;
    std::cout << boost::format("Using Device: %s") % tx_usrp->get_pp_string() << std::endl;

    //set the sample rate
    if (rate <= 0.0){
        std::cerr << "Please specify a valid sample rate" << std::endl;
        return ~0;
    }
    std::cout << boost::format("Setting RX TX Rate: %f Msps...") % (rate/1e6) << std::endl;
    usrp->set_rx_rate(rate);
    std::cout << boost::format("Actual RX Rate: %f Msps...") % (usrp->get_rx_rate()/1e6) << std::endl << std::endl;
    tx_usrp->set_tx_rate(rate);
    std::cout << boost::format("Actual TX Rate: %f Msps...") % (tx_usrp->get_tx_rate()/1e6) << std::endl << std::endl;

    //set the center frequency
    if (vm.count("freq")) { //with default of 0.0 this will always be true
        std::cout << boost::format("Setting RX Freq: %f MHz...") % (freq/1e6) << std::endl;
        uhd::tune_request_t tune_request(freq);
        if(vm.count("int-n")) tune_request.args = uhd::device_addr_t("mode_n=integer");
        usrp->set_rx_freq(tune_request, chan);
        std::cout << boost::format("Actual RX Freq: %f MHz...") % (usrp->get_rx_freq()/1e6) << std::endl << std::endl;
        std::cout << boost::format("Setting TX Freq: %f MHz...") % (freq/1e6) << std::endl;
        uhd::tune_request_t tx_tune_request(freq);
        if(vm.count("int-n")) tx_tune_request.args = uhd::device_addr_t("mode_n=integer");
        tx_usrp->set_tx_freq(tune_request, chan);
        std::cout << boost::format("Actual TX Freq: %f MHz...") % (tx_usrp->get_tx_freq()/1e6) << std::endl << std::endl;
    }

    //set the rf gain
    if (vm.count("rxgain")) {
        std::cout << boost::format("Setting RX Gain: %f dB...") % rxgain << std::endl;
        usrp->set_rx_gain(rxgain, chan);
        std::cout << boost::format("Actual RX Gain: %f dB...") % usrp->get_rx_gain() << std::endl << std::endl;
    }
    if (vm.count("txgain")) {
        std::cout << boost::format("Setting TX Gain: %f dB...") % txgain << std::endl;
        tx_usrp->set_tx_gain(txgain, chan);
        std::cout << boost::format("Actual TX Gain: %f dB...") % tx_usrp->get_tx_gain() << std::endl << std::endl;
    }

    //set the IF filter bandwidth
    if (vm.count("bw")) {
        std::cout << boost::format("Setting RX Bandwidth: %f MHz...") % (bw/1e6) << std::endl;
        usrp->set_rx_bandwidth(bw, chan);
        std::cout << boost::format("Actual RX Bandwidth: %f MHz...") % (usrp->get_rx_bandwidth()/1e6) << std::endl << std::endl;
        std::cout << boost::format("Setting TX Bandwidth: %f MHz...") % (bw/1e6) << std::endl;
        tx_usrp->set_tx_bandwidth(bw, chan);
        std::cout << boost::format("Actual TX Bandwidth: %f MHz...") % (tx_usrp->get_tx_bandwidth()/1e6) << std::endl << std::endl;
    }

    //set the antenna
    if (vm.count("ant")) {
        usrp->set_rx_antenna(ant, chan);
        tx_usrp->set_tx_antenna(ant, chan);
    }

    boost::this_thread::sleep(boost::posix_time::seconds(setup_time)); //allow for some setup time

    //check Ref and LO Lock detect
    if (not vm.count("skip-lo")){
        check_locked_sensor(usrp->get_rx_sensor_names(0), "lo_locked", boost::bind(&uhd::usrp::multi_usrp::get_rx_sensor, usrp, _1, 0), setup_time);
        if (ref == "mimo")
            check_locked_sensor(usrp->get_mboard_sensor_names(0), "mimo_locked", boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, usrp, _1, 0), setup_time);
        if (ref == "external")
            check_locked_sensor(usrp->get_mboard_sensor_names(0), "ref_locked", boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, usrp, _1, 0), setup_time);
        check_locked_sensor(tx_usrp->get_tx_sensor_names(0), "lo_locked", boost::bind(&uhd::usrp::multi_usrp::get_tx_sensor, tx_usrp, _1, 0), setup_time);
        if (ref == "mimo")
            check_locked_sensor(tx_usrp->get_mboard_sensor_names(0), "mimo_locked", boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, tx_usrp, _1, 0), setup_time);
        if (ref == "external")
            check_locked_sensor(tx_usrp->get_mboard_sensor_names(0), "ref_locked", boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, tx_usrp, _1, 0), setup_time);
    }

    if (total_num_samps == 0){
        std::signal(SIGINT, &sig_int_handler);
        std::cout << "Press Ctrl + C to stop streaming..." << std::endl;
    }

    // synchronization
    synch_to_gps(usrp);
    synch_to_gps(tx_usrp);

    // start thread
    if(opmode == "TX/RX" || opmode == "TX") {
        boost::thread_group transmit_thread;
        if(macmode == "TDMA") {
            transmit_thread.create_thread(boost::bind(&transmit_TDMA_worker, tx_usrp, total_num_samps, tslot));
        }
        else if(macmode == "UMAC") {
            transmit_thread.create_thread(boost::bind(&transmit_UMAC_worker, tx_usrp, total_num_samps, tslot));
        }
        else {
            printf("unknown macmode = %s\n", macmode);
            exit(1);
        }
    }
    if(opmode == "TX/RX" || opmode == "RX") {
        #define recv_worker_args(format) \
        (usrp, format, wirefmt, file, spb, total_num_samps, total_time, bw_summary, stats, null, enable_size_map, continue_on_bad_packet, tslot)
        if(macmode == "TDMA") {
            if (type == "double") recv_TDMA_worker<std::complex<double> >recv_worker_args("fc64");
            else if (type == "float") recv_TDMA_worker<std::complex<float> >recv_worker_args("fc32");
            else if (type == "short") recv_TDMA_worker<std::complex<short> >recv_worker_args("sc16");
            else throw std::runtime_error("Unknown type " + type);
        }
        else if(macmode == "UMAC") {
            if (type == "double") recv_UMAC_worker<std::complex<double> >recv_worker_args("fc64");
            else if (type == "float") recv_UMAC_worker<std::complex<float> >recv_worker_args("fc32");
            else if (type == "short") recv_UMAC_worker<std::complex<short> >recv_worker_args("sc16");
            else throw std::runtime_error("Unknown type " + type);
        }
        else {
            printf("unknown macmode = %s\n", macmode);
            exit(1);
        }
    }
    else {
        while(1) sleep(1);
    }

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}
