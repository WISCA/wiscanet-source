clear all;
close all;

if 0
    gradio_mex('txinit', '127.0.0.1', 9940);
    zData = zeros(1, 100);
    uData = 1:2000;
    txData = [zData uData];

    while(1)
        fprintf('send: \n');
        gradio_mex('write', txData);
        input('hahahha');
    end

    gradio_mex('close');
end


if 1
%    figure; hold on; grid on;
    gradio_mex('rxinit', '127.0.0.1', 9942);
    while(1)
        [len, rxdat] = gradio_mex('read', 2048);
        fprintf('recv: %d, %d\n', len, length(rxdat));
        m=1;
        for n=1:2:length(rxdat)
            rxcdat(m) = complex(rxdat(n), rxdat(n+1));
            m=m+1;
        end
%        plot(real(rxcdat), '-x');
%        plot(imag(rxcdat), '-o');
    end
    gradio_mex('close');
end

