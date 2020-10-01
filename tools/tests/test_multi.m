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
numChans = 1;
sineChan = [sine];

%test_usrp.tx_usrp(double(uint64(posixtime(datetime('now','Timezone','UTC'))))+5,sineChan,numChans);

rxChans = test_usrp.rx_usrp(double(uint64(posixtime(datetime('now','Timezone','UTC'))))+3,numChans);

test_usrp.terminate_usrp();
