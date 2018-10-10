function rxdata = buffer_read();

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
        [len, rxbuf(sidx:eidx)] = gradio_mex('read', rx_float_unit);
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
