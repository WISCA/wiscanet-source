clear; clc;
test_usrp = local_usrp;
test_usrp = test_usrp.set_usrp(0, 0, 0, 0, 0, 50000,0,0, 0, 0, 0, 0);
rxChans = test_usrp.rx_usrp(double(uint64(posixtime(datetime('now','Timezone','UTC'))))+3,2);

test_usrp.terminate_usrp();

figure(1);
title("Absolute Value of Each Channel");
subplot(2,1,1);
plot(abs(rxChans(:,1)));
title("Channel 1");
subplot(2,1,2);
plot(abs(rxChans(:,2)));
title("Channel 2");

figure(2);
title("Phase of Each Channel");
subplot(2,1,1);
plot(phase(rxChans(:,1)));
title("Channel 1");
subplot(2,1,2);
plot(phase(rxChans(:,2)));
title("Channel 2");