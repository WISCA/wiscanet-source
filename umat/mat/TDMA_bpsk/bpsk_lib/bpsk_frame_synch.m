function [ frBoundary preBoundary dcor ] = bpsk_frame_synch(rxdata, sps, clen, cwmod )

dIdx = 1;
scanLen = clen * 3;

dcorMax = 0;
for n=dIdx:scanLen
    dcor(n) = abs(rxdata(n:sps:n+clen-1) * cwmod');
    if(dcor(n) > dcorMax) dcorMax = dcor(n);
    end
end

if(dcorMax < 2.5E4)
    preBoundary = -1;
    frBoundary = -1;
    return;
end

sIdx = 1;
fsThres = dcorMax * 0.7;
syncState = 1;
for n=dIdx:scanLen;
    if(syncState == 1) % bellow thres
        if(dcor(n) > fsThres) 
            fsmax(sIdx) = dcor(n);
            fsmaxIdx(sIdx) = n;
            syncState = 2;
        end
    else % above thres
        if(dcor(n) > fsThres) 
            if(dcor(n) > fsmax(sIdx))
                fsmax(sIdx) = dcor(n);
                fsmaxIdx(sIdx) = n;
            end
        else
            sIdx = sIdx+1;
            syncState = 1;
        end
    end
end

% frame boundary decision
frBoundary = -1;
if sIdx > 2 
    preBoundary = fsmaxIdx(sIdx - 2);
    frBoundary = fsmaxIdx(sIdx - 1) + clen;
%    fprintf('frame started at %d, cur(%d), pre(%d), clen(%d)\n', frBoundary, fsmaxIdx(sIdx-1), preBoundary, clen);
else
    preBoundary = -1;
    frBoudnary = -1;
%    fprintf('frame synch error\n');
    return;
end

return;
end

