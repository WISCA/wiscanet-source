classdef local_usrp
    properties

        request_num_samps

    end



    methods (Static)
        function lId = logicalId()
            fId = fopen('logicId', 'r');
            lId = fscanf(fId, '%d');
            fprintf('node logical ID = %d\n', lId);
            fclose(fId);
        end

    end

    methods
        %=================================================================
        %===========================================================
        function this = set_usrp(this,type, ant, subdev, ref, wirefmt, num_samps, ...
                sample_rate, freq, rx_gain, tx_gain, bw, setup_time)
            fprintf('Connecting to local host, txport 9940\n');
            local_usrp_mex('txinit', '127.0.0.1', 9940);
            fprintf('Connecting to local host, rxport 9944, 9945\n');
            local_usrp_mex('rxinit', '127.0.0.1', 9944, 9945);
            this.request_num_samps = num_samps;
        end

        %===========================================================
        function tx_usrp(this,start_time, tx_buff)
            tbuf = single(transpose(tx_buff));
            fprintf('tx_usrp(), start_time: %f, len=%d\n', start_time, length(tbuf));
            local_usrp_mex('write', tbuf, start_time);
        end
        %===========================================================
        function rxWav = rx_usrp(this,start_time, num_channels)

            fprintf('local_usrp: rx_usrp(), start_time = %f\n', start_time);
            % setup
            rx_comp_unit =  this.request_num_samps;
            %rx_comp_unit = this.request_num_samps;

            rx_short_unit = rx_comp_unit * 2;

            % data read
            % local_usrp_mex('rcon', start_time);

            %fprintf('rx_usrp(), start_time: %f, len=%f\n', start_time, rx_comp_unit);
            [len, rdat] = local_usrp_mex('read',rx_short_unit,start_time,num_channels);
            rxdat = rdat;

            rxBuff = double(rxdat) / 2^15;
            
            rxWavComplex = complex(rxBuff(1:2:end),rxBuff(2:2:end));
            
            rxWav = reshape(rxWavComplex,this.request_num_samps,[]);

           fprintf('local_usrp: read packet %d complex samples at time %f\n', len, start_time);
            %fprintf('R');

        end

        %===========================================================
        function terminate_usrp(this)
            pause(1);
            fprintf('Close all resources for USRP control\n');
            local_usrp_mex('close');
        end

    end
end
