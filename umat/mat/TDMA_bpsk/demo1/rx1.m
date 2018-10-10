%==========================================================================
% RECEIVER
%==========================================================================
clear all;
close all;
%rng(6);

addpath(genpath('../../lib'));
addpath(genpath('../bpsk_lib'));

% init system for USRP interface
ulib.sys_init('rxinit');

%config
ts = 1 / 1.0E6; % 1MSPS
tsps = 4; % sample per symbol
rsps = tsps;
cw = [0 0 0 0 0 1 1 0 0 1 0 1 0];
pLen = 400; % multiples of 4


fprintf('\n\n==========================================================\n');
fprintf('= start rx1.m\n');
fprintf('==========================================================\n\n');

% main rx routine
if 1
    % CRC library
    crcDet = comm.CRCDetector('z^16 + z^15 + z^2 + 1', 'ChecksumsPerFrame', 1);
    % discard the first rx frame which is generated in transient period
    rxdata = ulib.usrp_read();

    % decoding
    while(1)
        rxdata = ulib.usrp_read();
        startIdx = 1;
        while(startIdx < length(rxdata));
            [rxmsg, stat, nIdx] = bpsk_rx(rxdata, startIdx, ts, rsps, cw, pLen, crcDet, 1);
            startIdx = nIdx;
            if(stat == 1) exit; end; % stop after 1 frame reception
        end
    end
end

% rx data plot for debugging
if 0
    fprintf('msg log for debugging\n');
    rxlog = zeros(1,10);
    for n=1:12
        rxdata = ulib.usrp_read();
        fprintf('R');
        rxlog = [rxlog rxdata];
    end
    fprintf('\n');
    save('rxlog');
end

% close system for USRP interface
ulib.sys_close();

%==========================================================================
