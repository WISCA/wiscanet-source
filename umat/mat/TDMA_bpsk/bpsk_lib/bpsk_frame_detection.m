function [ dcrt, dIdx ] = bpsk_frame_detection( rxdata, sps, clen, sfactor ) 

    % correlation
    dcrt(1) = 0;
    avrg(1) = 0;

    for n=1:length(rxdata)-(2*clen)+1
        cor(n) = 0;
        e1(n) = 0;
        e2(n) = 0;
if 1
        if(n<4) 
            avrg(n) = abs(rxdata(n));
        else
            avrg(n) = avrg(n-1)/2 + abs(rxdata(n))/2;
        end
        if(avrg(n) < 3000) 
            dcrt(n) = 0;
            continue;
        end
end
        for m=1:sps:clen
            aa = rxdata(n+m-1);
            bb = rxdata(n+m+clen-1);
            cor(n) = cor(n) + aa * bb';
            e1(n) = e1(n) + aa * aa';
            e2(n) = e2(n) + bb * bb';
        end
        if (n<4)
            dcrt(n) = cor(n) * cor(n)' / e1(n) / e2(n) * sfactor; % detection criterion
        else
            dcrt(n) = (dcrt(n-3) + dcrt(n-2) + dcrt(n-1) + (cor(n) * cor(n)' / e1(n) / e2(n) * sfactor))/4;     
        end
    end
    
if 0
figure; hold on; grid on;
%plot(avrg*13, '-*');
plot(real(rxdata)*13, '-x');
%plot(imag(rxdata)*13, '-o');  
plot(real(dcrt), '-x');
end 

    % find correlction peak
    dIdx = -1;
    n = 1;
    dThres = sfactor * 0.7;
    while(n <= length(dcrt) && dIdx == -1)
        if(dcrt(n) > dThres && n > 20) 
            dIdx = n-20;
%            fprintf('frame detected at %d\n', dIdx);
        end
        n = n+1;
    end
    if dIdx == -1
%        fprintf('frame detection error\n');
        return;
    end
end

