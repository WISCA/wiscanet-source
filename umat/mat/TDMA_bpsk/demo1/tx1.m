%==========================================================================
% Transmitter MATLAB code
%==========================================================================

%==========================================================================
% initial operations
%==========================================================================
clear all;
close all;

addpath(genpath('../../lib'));
addpath(genpath('../bpsk_lib'));

% system initialization
ulib.sys_init('txinit');
lId = ulib.logicalId();

%==========================================================================
%config
%==========================================================================
ts = 1 / 1.0E6; % 1MSPS
tsps = 4; % sample per symbol
rsps = tsps/2;
cw = [0 0 0 0 0 1 1 0 0 1 0 1 0];
pLen = 400; % multiples of 4


fprintf('\n\n==========================================================\n');
fprintf('= start tx1.m\n');
fprintf('==========================================================\n\n');

% user initialization
crcGen = comm.CRCGenerator('z^16 + z^15 + z^2 + 1', 'ChecksumsPerFrame', 1);

saveFlag = 0;
while(1) 

    pause(1);

    %========== input data
    msg = sprintf('hello message from node-%d\n', lId);
    fprintf('tx msg = %s\n', msg);
    msg = [msg zeros(1, pLen/8-2)];

    nmsg = uint8(msg(1:pLen/8-2))';
    bimsg = de2bi(nmsg, 8, 'left-msb');
    bvmsg = logical(reshape(bimsg', 1, []));

    %========== CRC gen
    pLoad = step(crcGen, bvmsg')';

    %========== scrambler
    scp = repmat([0 1], 1, length(pLoad)/2);
    pLoad = xor(pLoad, scp);

    %========== Modulation and transmitssion
    [ upTxdata] = bpsk_tx(tsps, cw, pLen, pLoad);

    %========== Log generation
    if(saveFlag == 0) 
        save('upTxdata.mat');
        saveFlag = 1;
    end
end

% system close
ulib.sys_close();
