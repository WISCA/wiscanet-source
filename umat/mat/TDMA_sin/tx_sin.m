close all;
clear all;

addpath(genpath('../lib'));
lId = ulib.logicalId();

sf = 0.25;
t = 1:30000;
T = 200;
ry = [single(cos(2 * pi * t / T)) * sf];
iy = [single(sin(2 * pi * t / T)) * sf];
cy = complex(ry, iy);
ydat = cy;

ulib.sys_init('txinit');
while(1)
    ulib.usrp_write(ydat);
    fprintf('T');
    pause(1);
end
ulib.sys_close();

figure; hold on; grid on;
plot(real(cy), '-x');
plot(imag(cy), '-o');


