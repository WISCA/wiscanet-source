classdef local_usrp
methods (Static)

%===========================================================
function set_usrp(type, ant, subdev, ref, wirefmt, num_samps, ...
                  sample_rate, freq, rx_gain, tx_gain, bw, setup_time) 
    fprintf('Conneting to local host, txport 9940\n');
    local_usrp_mex('txinit', '127.0.0.1', 9940); 
    fprintf('Conneting to local host, rxport 9942, 9943\n');
    local_usrp_mex('rxinit', '127.0.0.1', 9942, 9943); 
end

%===========================================================
function tx_usrp(start_time, tx_buff)
    tbuf = single(transpose(tx_buff));
    fprintf('tx_usrp(), start_time: %f, len=%d\n', start_time, length(tbuf));
    local_usrp_mex('write', tbuf, start_time);
end

%===========================================================
function rx_buff = rx_usrp(start_time)

%fprintf('local_usrp: rx_usrp(), start_time = %f\n', start_time);

    % setup
    rx_comp_unit = 1000;
    rx_short_unit = rx_comp_unit * 2;

    % data read
    local_usrp_mex('rcon', start_time);

    [len, rdat] = local_usrp_mex('read', rx_short_unit);
    rxdat = transpose(rdat);
    totalRx = len;

    if(len > 0)
        while(1)
            [len, rdat] = local_usrp_mex('read', rx_short_unit);
            if(len == 0) break; end
            totalRx = totalRx + len;
            rxdat = [rxdat transpose(rdat)];
        end
    end
    rx_buff = double(transpose(rxdat)) / 2^11;

fprintf('local_usrp: read packet %d complex samples at time %f\n', length(rx_buff)/2, start_time);
%fprintf('R');

end

%===========================================================
function terminate_usrp()
    local_usrp_lib.sys_close();
end

end
end
