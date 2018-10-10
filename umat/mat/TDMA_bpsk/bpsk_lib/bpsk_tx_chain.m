function [ upTxdata ] = bpsk_tx_chain(srcId, destId, payload, pLen, crcGen, tsps, cw)

    msg = [srcId  destId payload zeros(1, pLen/8-2)];
    nmsg = uint8(msg(1:pLen/8-2))';
    bimsg = de2bi(nmsg, 8, 'left-msb');
    bvmsg = logical(reshape(bimsg', 1, []));

    % CRC gen
    pLoad = step(crcGen, bvmsg')';

    % scrambler
    scp = repmat([0 1], 1, length(pLoad)/2);
    pLoad = xor(pLoad, scp);

    % modulation and transmitssion
    [ upTxdata ] = bpsk_tx(tsps, cw, pLen, pLoad);

end
