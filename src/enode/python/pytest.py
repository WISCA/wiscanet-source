from local_usrp import *

num_samples = 50000
num_channels = 1

# initialize LocalUSRP object
usrp_radio = LocalUSRP(num_samples) # Specify Number of samples

logic_id = usrp_radio.logical_id()

test_data = np.random.rand(num_samples, num_channels)

usrp_radio.tx_usrp(150000, test_data, num_channels)

usrp_radio.terminate_usrp()
