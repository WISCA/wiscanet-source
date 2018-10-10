%==========================================================================
% transmitter
%==========================================================================

% initial operations
clear all;
close all;
%rng(6);

addpath(genpath('../../lib'));
addpath(genpath('../bpsk_lib'));

% init for USRP interface
ulib.sys_init('txinit');
lId = ulib.logicalId();

%config
ts = 1 / 1.0E6; % 1MSPS
tsps = 4; % sample per symbol
rsps = tsps/2;
cw = [0 0 0 0 0 1 1 0 0 1 0 1 0];
pLen = 400; % multiples of 4

% Banner
fprintf('\n\n==========================================================\n');
fprintf('= start tx3.m\n');
fprintf('==========================================================\n\n');

crcGen = comm.CRCGenerator('z^16 + z^15 + z^2 + 1', 'ChecksumsPerFrame', 1);
saveFlag = 0;

srcId = lId;
destId = 7;
relayId = srcId;
count = 0;

txString = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';

while(1) 

    % input data
    pause(7);
    payload = txString(1:count+1);
    payload = [relayId payload];
    fprintf('tx %d->%d: %s\n', srcId, destId, payload);
    count = mod(count+2, 26);

    % tx chain
    upTxdata = bpsk_tx_chain(srcId, destId, payload, pLen, crcGen, tsps, cw);

    % tx log generation
    if(saveFlag == 0) 
        save('upTxdata');
    end
    saveFlag = saveFlag + 1;
end

% cleanup
ulib.sys_close();

%===============================================================
