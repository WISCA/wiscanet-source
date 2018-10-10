figure('position', [50, 600, 400, 400]); 
hold on; grid on;
load('upTxdata');
plot(real(upTxdata), '-x');
plot(imag(upTxdata), '-o');
title('BPSK TX signal');
