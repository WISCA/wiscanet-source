load('detData');
load('rxModPload');
load('comRxModPload');

figure('position', [470, 600, 900, 400]);

% time signal
subplot(1,2,1); hold on; grid on; 
plot(real(detData), '-x');
plot(imag(detData), '-o');
title('BPSK RX signal');


% fftplot
subplot(1,2,2); hold on; grid on;
idata = detData;
fs = 1E6;
N = length(idata);
f = [-N/2:N/2-1]*fs/N;
fx = fftshift(fft(idata));
plot(f,abs(fx), '-o');
title('FFT plot of RX signal');
 
