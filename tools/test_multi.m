clear; clc;
test_usrp = local_usrp;
test_usrp = test_usrp.set_usrp(0, 0, 0, 0, 0, 50000,0,0, 0, 0, 0, 0);

sample_rate = 10e6;
num_samps = 50000;

fc = 0.1e6;
fc2 = 0.5e6;
t=0:1/sample_rate:(1/sample_rate*(num_samps - 1));
sine = exp(1i*2*pi*fc*t).';
sine2 = exp(1i*2*pi*fc2*t).';
numChans = 2;
sineChan = [sine sine2];

test_usrp.tx_usrp(double(uint64(posixtime(datetime('now','Timezone','UTC'))))+5,sineChan,numChans);

%rxChans = test_usrp.rx_usrp(double(uint64(posixtime(datetime('now','Timezone','UTC'))))+3,numChans);

test_usrp.terminate_usrp();
% 
% figure(1);
% title("Absolute Value of Each Channel");
% subplot(2,1,1);
% plot(abs(rxChans(10000:11024,1)));
% title("Channel 1");
% subplot(2,1,2);
% plot(abs(rxChans(10000:11024,2)));
% title("Channel 2");
% 
% figure(2);
% title("Phase of Each Channel");
% subplot(2,1,1);
% plot(phase(rxChans(10000:11024,1)));
% title("Channel 1");
% subplot(2,1,2);
% plot(phase(rxChans(10000:11024,2)));
% title("Channel 2");