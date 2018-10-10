%==========================================================================
% RECEIVER
%==========================================================================
clear all;
close all;
%rng(6);

addpath(genpath('../lib'));

% init system for USRP interface
ulib.sys_init('rxinit');

fprintf('\n\n==========================================================\n');
fprintf('= start rx_sin.m\n');
fprintf('==========================================================\n\n');

rxNum = 5;
slotNum = 4-1;

% rx data plot for debugging
if 1
    fprintf('msg log for debugging\n');
    rxlog = zeros(1,10);
    for n=1:(rxNum * slotNum)
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
