function [rxmsg, stat, nIdx] = bpsk_rx(rxdata, startIdx, ts, sps, cw, pLen, crcDet, logFlag)

    sfactor = 13; % normalized correlation scaling factor
    Hbm = comm.BPSKModulator('PhaseOffset', pi/4);
    cwmod = transpose(step(Hbm, transpose(cw)));
    Hbd = comm.BPSKDemodulator('PhaseOffset', pi/4);
    clen = length(cw) * sps;
    pLoad = repmat([0 0 0 1], 1, pLen/4);

    frameId = 1;
    detScanLen = 100;
    syncScanLen = clen * 3;
    frameLen = 1800;
    endIdx = startIdx + detScanLen - 1;       

    while(1)
        if((endIdx + (frameLen)) > length(rxdata)) 
            rxmsg = zeros(1,400);
            stat = 0;
            nIdx = length(rxdata);
            break; 
        end;
%        fprintf('%d-th frame decoding, sIdx=%d, eIdx=%d\n', frameId, startIdx, endIdx);
    
        %============= frame detection
        frameDetIdat = rxdata(startIdx:endIdx+(2*clen));
        [dcrt dIdx] = bpsk_frame_detection(frameDetIdat, sps, clen, sfactor);

        if(dIdx == -1)
            % frame detection fail
            startIdx = startIdx + detScanLen;
            endIdx =  startIdx + detScanLen - 1;
            continue; % skip remainng process
        else
%            fprintf('frame detected: %d to %d\n', startIdx+dIdx-1, startIdx+dIdx+frameLen-1);
            detData = rxdata(startIdx+dIdx-1:startIdx+dIdx+frameLen+200-1);
        end
    
        %============= frame synch
        [frBoundary preBoundary dcor] = bpsk_frame_synch(detData, sps, clen, cwmod);
    
        if(frBoundary == -1) 
            % frame synch fail
            startIdx = startIdx + dIdx + syncScanLen;
            endIdx =  startIdx + detScanLen - 1;
            continue; % skip remainng process
        else
%            fprintf('frame boundary synchronized\n');
            % succsfful frame synch
            startIdx = startIdx + dIdx + frameLen;
            endIdx = startIdx + detScanLen -1;
        end

        %============= intermediate result display
        if 0
            figure; hold on; grid on;
            plot(real(detData(1:200)), '-o');
            plot(imag(detData(1:200)), '-x');
            plot(dcor/13, '-');
        end

        %============= down sampling
%        fprintf('%f, %f, %f, %d\n', pLen, frBoundary, frBoundary+sps*pLen-1, length(detData));
        rxModPload = detData(frBoundary:sps:frBoundary+sps*pLen-1);

        %============= frequency offset compensation
        comRxModPload = bpsk_freq_offset_comp(detData, rxModPload, preBoundary, frBoundary, cw, cwmod, clen, sps, ts);

        %============= display compensation results
        if 0
            bpsk_constellation(rxModPload, comRxModPload);
        end
    
        %============= demodulation
        rxPload = step(Hbd, transpose(comRxModPload));

        %============= descrambling
        dscp = repmat([0 1], 1, length(rxPload)/2);
        dscPload = xor(rxPload', dscp)';
    
        %============= data integrity checking
        [rxmsg err] = bpsk_rx_err(dscPload, pLen, crcDet);

        if(err == 0)
             if(logFlag); 
                 save('detData.mat', 'detData');
                 save('rxModPload.mat', 'rxModPload');
                 save('comRxModPload.mat', 'comRxModPload');
                 firstLogFlag = 1;
             end
             stat = 1;
             nIdx = startIdx + frameLen;
             return;
        end
    end
end
