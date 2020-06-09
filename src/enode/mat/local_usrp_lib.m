classdef local_usrp_lib
methods (Static)

%===========================================================
function sys_init(cmd);
    if(cmd == 'txinit')
        fprintf('Conneting to local host, txport 9940\n');
        local_usrp_mex(cmd, '127.0.0.1', 9940); 
    elseif (cmd == 'rxinit')
        fprintf('Conneting to local host, rxport 9942\n');
        local_usrp_mex(cmd, '127.0.0.1', 9942); 
    end
end

%===========================================================
function sys_close();
    fprintf('Close all resources for USRP control\n');
    local_usrp_mex('close');
end

%===========================================================
function usrp_write(tdat, start_time)
    % IQ data alignment
    for n=1:length(tdat)
        txdata(n*2-1) = single(real(tdat(n)));
        txdata(n*2)   = single(imag(tdat(n)));
    end
    % Transmission
fprintf('before TX\n');
    local_usrp_mex('write', txdata, start_time);
fprintf('after TX\n');

end

%===========================================================
function rxdata = usrp_read();

    % setup
    rx_comp_unit = 1024;
    rx_float_unit = rx_comp_unit * 2;
    bufSize = rx_float_unit * 10;
    rxbuf = single(zeros(1, bufSize + rx_float_unit));

    % data read
    totalRx = 0;
    while(totalRx < bufSize)
        sidx = (totalRx+1);
        eidx = (totalRx+rx_float_unit);
        [len, rxbuf(sidx:eidx)] = local_usrp_mex('read', rx_float_unit);
        if(len == 0) break; end;
        totalRx = totalRx + len;
    end
    
    % ....
%fprintf('\nread packet %d complex samples\n', totalRx/2);
%fprintf('R');

    % data align
    idx = 1;
    rxdata(1) = complex(0, 0);
    for n=1:2:totalRx
        rxdata(idx) = complex(rxbuf(n), rxbuf(n+1));
        idx = idx+1;
    end
    
end

end
end
