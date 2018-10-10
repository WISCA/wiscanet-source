function [ ] = bpsk_constellation(pload1, pload2);
    n=1; m=400;
    figure; hold on; grid on;
    plot([-1 1], [1, -1], '-');
    plot([1 -1], [1, -1], ':');
    plot(real(pload1(n:m)), imag(pload1(n:m)), 'o');
    plot(real(pload2(n:m)), imag(pload2(n:m)), '*');
end
