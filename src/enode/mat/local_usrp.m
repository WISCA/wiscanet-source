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
        %===========================================================
        function this = set_usrp(this,type, ant, subdev, ref, wirefmt, num_samps, ...
                sample_rate, freq, rx_gain, tx_gain, bw, setup_time)
            fprintf('Connecting Transmit Controller (Port: 9940)\n');
            local_usrp_mex('txinit', '127.0.0.1', 9940);
            fprintf('Connecting Recieve Controller (Ports: 9944, 9945)\n');
            local_usrp_mex('rxinit', '127.0.0.1', 9944, 9945);
            this.request_num_samps = num_samps;
        end
        %===========================================================
        function tx_usrp(this,start_time, buff_in, num_channels)
            % This expects a num_samps x num_channels complex double matrix
            [inRows, inCols] = size(buff_in);
            assert(inRows == this.request_num_samps,'Wrong number of samples input!');
            assert(inCols == num_channels,'Wrong number of channels input!');
            flatBuff = reshape(buff_in,1,[]);
            total_samples = 2*this.request_num_samps*num_channels;
            txbuff = zeros(total_samples, 1);
            txbuff(1:2:total_samples) = real(flatBuff);
            txbuff(2:2:total_samples) = imag(flatBuff);
            tbuf = single(transpose(txbuff));
            fprintf('[Local USRP] Transmitting at %f, %d bytes (%d samples)\n', start_time, length(tbuf), total_samples);
            local_usrp_mex('write', tbuf, start_time, num_channels);
            fprintf('[Local USRP] Finished transmitting\n');
        end
        %===========================================================
        function rxWav = rx_usrp(this,start_time, num_channels)
            % This returns a num_samps x num_channels complex double matrix
            fprintf('[Local USRP] Receiving at %f for %d channels\n', start_time, num_channels);
            rx_comp_unit =  this.request_num_samps;
            rx_short_unit = rx_comp_unit * 2;

            [len, rdat] = local_usrp_mex('read',rx_short_unit,start_time,num_channels);
            rxdat = rdat;

            rxBuff = double(rxdat) / 2^15;
            rxWavComplex = complex(rxBuff(1:2:end),rxBuff(2:2:end));
            rxWav = reshape(rxWavComplex,this.request_num_samps,[]);
            fprintf('[Local USRP] Finished reciving %d complex samples at time %f\n', len/2, start_time);

        end
        %===========================================================
        function terminate_usrp(this)
            pause(1);
            local_usrp_mex('close');
            pause(1);
            fprintf('Connections have been shutdown.\n');
        end

    end
end
