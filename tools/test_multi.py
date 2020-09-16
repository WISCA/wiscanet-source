from local_usrp import *
import time
import matplotlib.pyplot as plt

num_samples = 50000
num_channels = 1

# initialize LocalUSRP object
usrp_radio = LocalUSRP(num_samples) # Specify Number of samples

logic_id = usrp_radio.logical_id()

test_data = np.random.rand(num_samples, num_channels) + np.random.rand(num_samples, num_channels) * 1j

start_time = int(time.time()) + 3.0

#usrp_radio.tx_usrp(start_time, test_data, num_channels)

rx_buffer = usrp_radio.rx_usrp(start_time, num_channels)

usrp_radio.terminate_usrp()

plt.plot(np.abs(rx_buffer))
plt.show()
