function [TrDataAWGN] = bpsk_channel(SNR, L, TrData)

    % frequency selective channel
    channel_effect = 1;    
    if(channel_effect) 
        h(1:L,1) = random('Normal',0,1,L,1) + j * random('Normal',0,1,L,1);  
        h  = h./sum(abs(h));    % normalization
        TrDataChannel = filter(h, 1, TrData);    % channel effect
    else
       TrDataChannel = TrData;    
    end
    
    % adding awgn noise
    awgn_effect = 1;
    if(awgn_effect) 
        TrDataAWGN = awgn(TrDataChannel, SNR - db(std(TrDataChannel))); % normalization to signal power
    else 
        TrDataAWGN = TrDataChannel;    
    end
    
end