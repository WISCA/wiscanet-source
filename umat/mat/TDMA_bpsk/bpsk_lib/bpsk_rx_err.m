function [s err] = bpsk_rx_err(rxPload, pLen, crcDet)

    [~,err] = step(crcDet, logical(rxPload));
    if(err) 
%        fprintf('CRC error\n');
    else
%        fprintf('CRC pass\n');
    end

    decData = rxPload(1:pLen-16)';
    payload = vec2mat(decData, 8);
    rxdata = bi2de(payload, 2, 'left-msb');
    s = char(rxdata');
%    fprintf('\n\npayload = %s\n\n', s);

end

