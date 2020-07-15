classdef local_usrp
    properties
        request_num_samps
        channel
        air_buffer
        num_tx
        num_rx
        txPos
        rxPos
        txPower
        sampleRate
        upsampFactor
        c
    end



    methods (Static)
        function lId = logicalId()
            lId = randi([0 3]);
            fprintf('node logical ID = %d\n', lId);
        end

    end

    methods
        %=================================================================
        %===========================================================
        function this = set_usrp(this,type, ant, subdev, ref, wirefmt, num_samps, ...
                sample_rate, freq, rx_gain, tx_gain, bw, setup_time)
            fprintf('Connecting Transmit Controller (Port: 9940)\n');
            fprintf('Connecting Recieve Controller (Ports: 9944, 9945)\n');
            this.request_num_samps = num_samps;
        end

        function this = set_channel(this, channel, posTx, posRx, txPower,sampleRate, upsampFactor)
            this.channel = channel;
            this.txPos = posTx;
            this.rxPos = posRx;
            this.txPower = txPower;
            this.sampleRate = sampleRate;
            this.upsampFactor = upsampeFactor;
            this.c = physconst('LightSpeed');
        end

        %===========================================================
        function tx_usrp(this,start_time, buff_in, num_channels)
            % This expects a num_samps x num_channels complex double matrix
            this.air_buffer = single(buff_in);
            this.num_tx = num_channels;
            fprintf('[Local USRP] Transmitting at %f, %d bytes (%d samples)\n', start_time, length(tbuf), total_samples);
            fprintf('[Local USRP] Finished transmitting\n');
        end
        %===========================================================
        function rxWav = rx_usrp(this,start_time, num_channels)
            % This returns a num_samps x num_channels complex double matrix
            fprintf('[Local USRP] Receiving at %f for %d channels\n', start_time, num_channels);
            %% Send data over the air
            tauTxRx=pdist2(this.rxPos*10,this.txPos*10+repmat([1e3,1e3,1e3],2,1))/this.c*(this.sampleRate*this.upsampFactor); %number of samples
            tauProp=floor(min(tauTxRx(:)));
            tauTxRx=tauTxRx-floor(min(tauTxRx(:)));
            sigxy=zeros(this.num_rx,this.request_num_samps);
            for ntx=1:this.num_tx
                for nrx=1:this.num_rx
                    xydel=ifft(ifftshift(fftshift(fft(this.air_buffer(:,ntx))).*exp(-1i*2*pi*tauTxRx(nrx,ntx)/(this.sampleRate*this.upsampFactor)*f.')));
                    sigxy((1+tauProp):end,nrx)=sigxy((1+tauProp):end,nrx)+this.channel(nrx,ntx)*sqrt(this.txPower)*xydel;
                end
            end

            %% Receive
            noise=(randn(Nr,2*Nbuff)+1i*randn(Nr,2*Nbuff))/sqrt(2);
            rxWav = sigxy+noise;
            fprintf('[Local USRP] Finished receiving %d complex samples at time %f\n', len/2, start_time);
        end

        %===========================================================
        function terminate_usrp(this)
            pause(1);
            fprintf('Connections have been shutdown.\n');
        end

    end
end
