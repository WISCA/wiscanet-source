function [ comRxModPload ] = bpsk_freq_offset_comp( rxdata, rxModPload, preBoundary, frBoundary, cw, cwmod, clen, sps, ts)

    aa = rxdata(preBoundary);
    bb = rxdata(preBoundary+clen);

    angleAa = wrapTo2Pi(angle(aa));
    angleBb = wrapTo2Pi(angle(bb));
    angleRef = pi/4; % 0
    avgAngle = (angleAa + angleBb - 2 * angleRef) / 2;
    avgAngle = rem(avgAngle, 2 * pi);
    cf = exp(-1 * sqrt(-1)*avgAngle);

%fprintf('phase error in degree : %f\n', avgAngle / 2 / pi * 360);

    % compenstation
    for n=1:length(rxModPload)
        comRxModPload(n) = rxModPload(n) * cf;
    end

if 0
    figure; hold on; grid on;
    plot([-1 1], [1 -1], '-');
    plot([1 -1], [1 -1], '-');
    for n=1:400
        plot(real(rxModPload(n)), imag(rxModPload(n)), '-*');
        plot(real(comRxModPload(n)), imag(comRxModPload(n)), '-s');    
    end
end

end

