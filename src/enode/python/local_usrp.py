import socket
import numpy as np

class LocalUSRP:
    req_num_samps = 50000 # Number of samples requested
    tx_udp = None
    rx_udp_con = None
    rx_udp = None
    UCONTROL_IP = "127.0.0.1"
    TX_PORT = 9940
    RX_PORT = 9944
    RX_PORTCON = 9945

    def __init__(self, numsamples):
        self.req_num_samps = numsamples
        print("Connecting Transmit Controller (Port: 9940)\n")
        self.tx_udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        print("Connecting Receive Controller (Ports: 9944, 9945)\n")
        self.rx_udp_con = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.rx_udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.rx_udp.bind((self.UCONTROL_IP, self.RX_PORT))

    def logical_id(self):
        f_id = open('logicId', 'r')
        logic_id = int(f_id.read())
        print("Node Logical ID: %d\n" % logic_id)
        f_id.close()
        return logic_id

    def tx_usrp(self, start_time, input_buff, num_chans):
        # input_buff is req_num_samps x num_chans complex matrix
        [in_rows, in_cols] = input_buff.shape # Get shape of input
        assert in_rows == self.req_num_samps
        assert in_cols == num_chans
        tx_buff = input_buff.reshape(in_rows*in_cols, 1)
        interleaved_tx_buff = np.zeros((2*in_rows*in_cols, 1))
        interleaved_tx_buff[::2] = tx_buff.real
        interleaved_tx_buff[1::2] = tx_buff.imag
        # in original right here we do a single(transpose(interleaved_tx_buff))
        print("[Local USRP] Transmitting at %f, %d bytes (%d samples)\n" % (start_time, len(interleaved_tx_buff), self.req_num_samps))
        byte_buff = interleaved_tx_buff.tobytes()
        total_tx = 0
        tx_len = 0
        tx_unit = 4095
        while (totalTx < len(byte_buff)):
            if ((len(byte_buff) - total_tx) > tx_unit ):
                tx_len = tx_unit
            else:
                tx_len = len(byte_buff) - total_tx

            self.tx_udp.sendto(byte_buff[total_tx:tx_en+total_tx], (self.UCONTROL_IP, self.TX_PORT))

        self.tx_udp.sendto(bytearray(start_time), (self.UCONTROL_IP, self.TX_PORT))
        self.tx_udp.sendto(bytearray(0), (self.UCONTROL_IP, self.TX_PORT))
        print("[Local USRP] Finished transmitting\n")

        def rx_usrp(self, start_time, num_chans):
            #        function rxWav = rx_usrp(this,start_time, num_channels)
#            % This returns a num_samps x num_channels complex double matrix
#            fprintf('[Local USRP] Receiving at %f for %d channels\n', start_time, num_channels);
#            rx_comp_unit =  this.request_num_samps;
#            rx_short_unit = rx_comp_unit * 2;
#
#            [len, rdat] = local_usrp_mex('read',rx_short_unit,start_time,num_channels);
#            rxdat = rdat;
#
#            rx_buff = double(rxdat) / 2^15;
#            rxWavComplex = complex(rx_buff(1:2:end),rx_buff(2:2:end));
#            rxWav = reshape(rxWavComplex,this.request_num_samps,[]);
#            fprintf('[Local USRP] Finished reciving %d complex samples at time %f\n', len/2, start_time);
#
#        end
# Return variable is req_num_samps x num_chans complex matrix
print("[Local USRP] Receiving at %f for %d channels\n" % (start_time, num_chans))
rx_buff = []
print("[Local USRP] Finished receiving %d complex samples at time %f\n" % (len(rx_buff), start_time))
return rx_buff

                             def terminate_usrp(self):
                                 self.rx_udp.close()
                                 self.rx_udp_con.close()
                                 self.tx_udp.close()
                                 print("[Local USRP] Connections have been shutdown.\n")

