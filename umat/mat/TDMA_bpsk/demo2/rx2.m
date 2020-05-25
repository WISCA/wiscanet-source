%==========================================================================
% RECEIVER
%==========================================================================

% initial operations
clear all;
close all;
%rng(6);

addpath(genpath('../../lib'));
addpath(genpath('../bpsk_lib'));

% init socket interface to USRP
ulib.sys_init('rxinit');
lId = ulib.logicalId();

%config
ts = 1 / 1.0E6; % 1MSPS
tsps = 4; % sample per symbol
rsps = tsps;
cw = [0 0 0 0 0 1 1 0 0 1 0 1 0];
pLen = 400; % multiples of 4

logFlag = 1;
srcId = lId;

fprintf('\n\n==========================================================\n');
fprintf('= start rx2.m !!\n');
fprintf('==========================================================\n\n');

% Actucal data TX/RX operations
if 1
    % CRC
    crcDet = comm.CRCDetector('z^16 + z^15 + z^2 + 1', 'ChecksumsPerFrame', 1);
    % discard the first rx frame which is generated in transient period
    rxdata = ulib.usrp_read();

    % decoding
    while(1)
        rxdata = ulib.usrp_read();
        startIdx = 1;
        while(startIdx < length(rxdata));
%fprintf('startIdx = %d\n', startIdx);
            [rxmsg, stat, nIdx] = bpsk_rx(rxdata, startIdx, ts, rsps, cw, pLen, crcDet, logFlag);
            startIdx = nIdx;
            if(stat == 1) 
                nmsg = uint8(rxmsg(1:pLen/8-2))';
                if(nmsg(2) == srcId) 
                    fprintf('srcId = %d, destId = %d, palyad = %s\n', nmsg(1), nmsg(2), nmsg(3:end));
                end
                logFlag=0;
            end
        end
    end
end

% IQ data dump for debugging
if 0
    rxlog = zeros(1,10);
    for n=1:40
        rxdata = ulib.usrp_read();
        fprintf('R');
        rxlog = [rxlog rxdata];
    end
    fprintf('\n');

    save('rxlog.mat');
end

% close system for USRP interface
ulib.sys_close();
%==========================================================================
