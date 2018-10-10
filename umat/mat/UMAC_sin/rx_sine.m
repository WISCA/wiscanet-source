function rx_sine(sys_start_time)

close all;
addpath(genpath('../lib'));

usrp_address='type=b200';
type = 'double';            %type
ant  = 'TX/RX';             %ant
subdev = 'A:B';             %subdev
ref = 'gpsdo';              %ref
wirefmt = 'sc16';           %wirefmt
freq    = 5.8e9;            %freq
rx_gain = 60;               %rx_gain
tx_gain = 80;               %tx_gain
bw      = 20e6;             %bw
setup_time = 0.1;           %setup_time

num_samps = 200000;
sample_rate = 10e6; 
local_usrp.set_usrp(type, ant, subdev, ref, wirefmt, num_samps,...
                    sample_rate, freq, rx_gain, tx_gain, bw, setup_time);

fc = 0.1e6;
t=0:1/sample_rate:(1/sample_rate*(num_samps-1));
sine = exp(1i*2*pi*fc*t);
tx_buff = zeros(2*num_samps, 1);
tx_buff(1:2:2*num_samps) = real(sine);
tx_buff(2:2:2*num_samps) = imag(sine);

cycle_number = 5;
if 0
    p = posixtime(datetime('now','Timezone','UTC'));
    start_time = double(uint64(p)+2);  %start_time
else
    start_time = sys_start_time;
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%send out a carrier and then rx a carrier on next second
rxdat = zeros(1,100);
for i = 1:cycle_number
    %   
    rx_buff = local_usrp.rx_usrp(start_time);
    rxdat = [rxdat transpose(rx_buff)];
    start_time = start_time + 3;
end
cRxDat = complex(rxdat(1:2:end), rxdat(2:2:end));
fprintf('-- Dump log: cRxDat\n');
save('cRxDat');

local_usrp.terminate_usrp();

end
