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

nodeId = lId;

fprintf('\n\n==========================================================\n');
fprintf('= start rx3.m !!\n');
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
            [rxmsg, stat, nIdx] = bpsk_rx(rxdata, startIdx, ts, rsps, cw, pLen, crcDet, 0);
            startIdx = nIdx;
            if(stat == 1) 
                nmsg = uint8(rxmsg(1:pLen/8-2))';
                srcId = nmsg(1);
                destId = nmsg(2);
                relayId = nmsg(3);
                payload = nmsg(4:end);
                if(relayId == (nodeId - 1) && nodeId == nmsg(2)) 
                    fprintf('rx %d->%d: %s\n', nmsg(1), nmsg(2), nmsg(4:end));
                end
            end
        end
    end
end

% close system for USRP interface
ulib.sys_close();
%==========================================================================
