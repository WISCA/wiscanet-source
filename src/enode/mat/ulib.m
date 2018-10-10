classdef ulib
methods (Static)

%===========================================================
function sys_init(cmd);
    if(cmd == 'txinit')
        fprintf('Conneting to local host, txport 9940\n');
        gradio_mex(cmd, '127.0.0.1', 9940); 
    elseif (cmd == 'rxinit')
        fprintf('Conneting to local host, rxport 9942, 9943\n');
        gradio_mex(cmd, '127.0.0.1', 9942, 9943); 
        gradio_mex('rcon', 0); % immediate start
    end
end

%===========================================================
function lId = logicalId();
    fId = fopen('logicId', 'r');
    lId = fscanf(fId, '%d');
    fprintf('node logical ID = %d\n', lId);
    fclose(fId);
end

%===========================================================
function sys_close();
    fprintf('Close all resources for USRP control\n');
    gradio_mex('close');
end

%===========================================================
function usrp_write(tdat)
    % IQ data alignment
    for n=1:length(tdat)
        txdata(n*2-1) = single(real(tdat(n)));
        txdata(n*2)   = single(imag(tdat(n)));
    end
%fprintf('tdat-length = %d, txdata-length = %d\n', length(tdat), length(txdata));

    % Transmission
    gradio_mex('write', txdata);
end

%===========================================================
function rxdata = usrp_read();

    % setup
    rx_float_unit = 2048;
    maxBufSize = 200000;

    % data read
    totalRx = 0;
    while(totalRx < maxBufSize)
        sidx = (totalRx+1);
        eidx = (totalRx+rx_float_unit);
        [len, rxbuf(sidx:eidx)] = gradio_mex('read', rx_float_unit);
        if(len == 0) break; end;
        totalRx = totalRx + len;
    end
    
    % ....
%fprintf('\nrlen = %d complex samples\n', totalRx/2);
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
