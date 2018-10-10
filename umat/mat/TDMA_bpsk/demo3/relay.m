%==========================================================================
% transmitter
%==========================================================================
clear all;
close all;
%rng(6);

addpath(genpath('../../lib'));
addpath(genpath('../bpsk_lib'));

% init socket interface to UHD
ulib.sys_init('txinit');
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
fprintf('= start relay.m\n');
fprintf('= my nodeId = %d\n', nodeId);
fprintf('==========================================================\n\n');

% CRC
crcGen = comm.CRCGenerator('z^16 + z^15 + z^2 + 1', 'ChecksumsPerFrame', 1);
crcDet = comm.CRCDetector('z^16 + z^15 + z^2 + 1', 'ChecksumsPerFrame', 1);

% Actucal data TX/RX operations
if 1

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
                if(nodeId ~= destId) 
                    if(relayId == (nodeId - 1))
                        nmsg(3) = nodeId;
                        fprintf('fwd %d->%d: %s\n', nodeId, destId, nmsg(4:end));
                        upTxdata = bpsk_tx_chain(srcId, destId, nmsg(3:end)', pLen, crcGen, tsps, cw);
                    end
                else
                    fprintf('my message\n');
                end
                
            end
        end
    end
end

% close system
ulib.sys_close();

%==========================================================================
