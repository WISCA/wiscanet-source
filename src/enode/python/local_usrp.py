import socket
import numpy as np
import struct
import time

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
        print("Connecting Transmit Controller (Port: 9940)\n", flush=True)
        self.tx_udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        print("Connecting Receive Controller (Ports: 9944, 9945)\n", flush=True)
        self.rx_udp_con = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.rx_udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.rx_udp.bind((self.UCONTROL_IP, self.RX_PORT))

    def logical_id(self):
        f_id = open('logicId', 'r')
        logic_id = int(f_id.read())
        print("Node Logical ID: %d\n" % logic_id, flush=True)
        f_id.close()
        return logic_id

    def tx_usrp(self, start_time, input_buff, num_chans, ref_power):
        # input_buff is req_num_samps x num_chans complex matrix
        [in_rows, in_cols] = input_buff.shape # Get shape of input
        assert in_rows == self.req_num_samps
        assert in_cols == num_chans
        tx_buff = input_buff.reshape(in_rows*in_cols, 1)
        interleaved_tx_buff = np.zeros((2*in_rows*in_cols, 1))
        interleaved_tx_buff[::2] = tx_buff.real
        interleaved_tx_buff[1::2] = tx_buff.imag
        # in original right here we do a single(transpose(interleaved_tx_buff))
        print("[Local USRP] Transmitting at %f, %d bytes (%d samples)\n" % (start_time, len(interleaved_tx_buff), self.req_num_samps), flush=True)
        byte_buff = interleaved_tx_buff.astype(np.single).tobytes()
        total_tx = 0
        tx_len = 0
        tx_unit = 4095
        while (total_tx < len(byte_buff)):
            if ((len(byte_buff) - total_tx) > tx_unit ):
                tx_len = tx_unit
            else:
                tx_len = len(byte_buff) - total_tx

            self.tx_udp.sendto(byte_buff[total_tx:tx_len+total_tx], (self.UCONTROL_IP, self.TX_PORT))
            total_tx = total_tx + tx_len

        self.tx_udp.sendto(bytearray(struct.pack("d",start_time)), (self.UCONTROL_IP, self.TX_PORT))
        self.tx_udp.sendto(bytearray(struct.pack("Q",num_chans)), (self.UCONTROL_IP, self.TX_PORT))
        self.tx_udp.sendto(bytearray(struct.pack("h",ref_power)), (self.UCONTROL_IP, self.TX_PORT))
        self.tx_udp.sendto(b'', (self.UCONTROL_IP, self.TX_PORT))

        while time.time() < start_time:
            time.sleep(200/1000000.0)

        time.sleep(0.2)
        print("[Local USRP] Finished transmitting\n", flush=True)

    def rx_usrp(self, start_time, num_chans):
        print("[Local USRP] Receiving at %f for %d channels\n" % (start_time, num_chans), flush=True)
        # Control Receive (aka send the start_time)
        self.rx_udp_con.sendto(bytearray(struct.pack("dd",start_time,0)), (self.UCONTROL_IP, self.RX_PORTCON)) # This has to send that zeroed second buffer, because the uControl/MEX version is sloppy
        self.rx_udp_con.sendto(bytearray(struct.pack("H",num_chans)), (self.UCONTROL_IP, self.RX_PORTCON))
        rx_unit = 4000*2
        input_len = num_chans * self.req_num_samps * 4
        buf_pos = 0
        byte_buff = bytearray()
        while True:
            readlen = input_len - buf_pos
            if (readlen > 0):
                (temp_buff,_) = self.rx_udp.recvfrom(rx_unit)
                retval = len(temp_buff)
                byte_buff.extend(bytearray(temp_buff))
                if (retval == 0):
                    print("[Local USRP] Completed one receiving cycle\n", flush=True)

            readlen = retval if retval > 0 else 0
            buf_pos = buf_pos + readlen

            if (buf_pos >= input_len):
                break

        rx_buff = np.frombuffer(byte_buff,np.short)
        rx_buff_scaled = rx_buff / 2**15
        rx_buff_complex = rx_buff_scaled[::2] + rx_buff_scaled[1::2] * 1j
        rx_buff_chans = rx_buff_complex.reshape((self.req_num_samps, num_chans))
        print("[Local USRP] Finished receiving %d complex samples at time %f\n" % (len(rx_buff), start_time), flush=True)
        return rx_buff_chans

    def terminate_usrp(self):
        self.rx_udp.close()
        self.rx_udp_con.close()
        self.tx_udp.close()
        print("[Local USRP] Connections have been shutdown.\n", flush=True)
