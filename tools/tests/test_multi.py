import sys
sys.path.append('../lib/')
from local_usrp import *
import time
import matplotlib.pyplot as plt

#start_time = float(sys.argv[1])
start_time = int(time.time()) + 5.0

num_samples = 50000
num_channels = 1

# initialize LocalUSRP object
usrp_radio = LocalUSRP(num_samples) # Specify Number of samples

#logic_id = usrp_radio.logical_id()

test_data = np.random.rand(num_samples, num_channels) + np.random.rand(num_samples, num_channels) * 1j

for i in range(1,10):
    usrp_radio.tx_usrp(start_time, test_data, num_channels,0)
    start_time = int(time.time()) + 5.0
#rx_buffer = usrp_radio.rx_usrp(start_time, num_channels)

usrp_radio.terminate_usrp()

#plt.plot(np.abs(rx_buffer))
#plt.show()
