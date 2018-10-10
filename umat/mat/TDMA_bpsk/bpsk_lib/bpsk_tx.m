function [ upTxdata ] = bpsk_tx(sps, cw, pLen, pload)

    %============= data frame
    txframe = [cw cw cw pload]'; % preabmle

    %============= modulation
    Hbm = comm.BPSKModulator('PhaseOffset', pi/4);
    txdata = [zeros(1, 10) transpose(step(Hbm, txframe)) zeros(1, 10)];

    %============= over sampling
%    upTxdata = single(upsample(txdata, sps));
upTxdata = [ complex(zeros(1,500), zeros(1,500)) interp(txdata, sps)];

    %============= transmit frames to USRP
    ulib.usrp_write(upTxdata);

end
