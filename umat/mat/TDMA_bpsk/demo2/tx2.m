%==========================================================================
% transmitter
%==========================================================================

% initial operations
clear all;
close all;
%rng(6);

addpath(genpath('../../lib'));
addpath(genpath('../bpsk_lib'));

% initialize interface to USRP
ulib.sys_init('txinit');
lId = ulib.logicalId();

% config
ts = 1 / 1.0E6; % 1MSPS
tsps = 4; % sample per symbol
rsps = tsps/2;
cw = [0 0 0 0 0 1 1 0 0 1 0 1 0];
pLen = 400; % multiples of 4

% banner
fprintf('\n\n==========================================================\n');
fprintf('= start tx2.m\n');
fprintf('==========================================================\n\n');

crcGen = comm.CRCGenerator('z^16 + z^15 + z^2 + 1', 'ChecksumsPerFrame', 1);

saveFlag = 0;
srcId = lId;
destId = srcId + 1;
count = 0;
while(1) 

    % input data
    pause(2);
    count = count + 1;
    msg = sprintf('%d-th hello world !!', count);
    fprintf('tx msg from %d to %d: %s\n', srcId, destId, msg);
    msg = [srcId destId msg zeros(1, pLen/8-2)];
    nmsg = uint8(msg(1:pLen/8-2))';
    bimsg = de2bi(nmsg, 8, 'left-msb');
    bvmsg = logical(reshape(bimsg', 1, []));

    % CRC gen
    pLoad = step(crcGen, bvmsg')';

    % scrambler
    scp = repmat([0 1], 1, length(pLoad)/2);
    pLoad = xor(pLoad, scp);

    % modulation and transmitssion
    [ upTxdata] = bpsk_tx(tsps, cw, pLen, pLoad);

    % tx log generation
    if(saveFlag == 0) 
        save('upTxdata.mat');
    end
    saveFlag = saveFlag + 1;

end

% cleanup
ulib.sys_close();
