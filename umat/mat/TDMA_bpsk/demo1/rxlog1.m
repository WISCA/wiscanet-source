load('detData');
load('rxModPload');
load('comRxModPload');

detData = detData / 24000;
rxModPload = rxModPload / 24000;
comRxModPload = comRxModPload / 24000;

figure('position', [470, 600, 900, 400]);

subplot(1,2,1); hold on; grid on; 
plot(real(detData), '-x');
plot(imag(detData), '-o');
title('BPSK RX signal');

n=1;
line1 = [complex(n, n) complex(-1*n, -1*n)];
line2 = [complex(-1*n, n) complex(n, -1*n)];

subplot(1,2,2); hold on; grid on;
plot(line1, '-');
plot(line2, '-');
plot(real(rxModPload), imag(rxModPload), 'o');
plot(real(comRxModPload), imag(comRxModPload), '*');
title('BPSK Constellation diagram (before and after)');
