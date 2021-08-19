import serial
import threading
import time
import queue
import atexit
from enum import Enum
from sys import argv, stdout
from argparse import ArgumentParser

#Before importing SMComPy check the .so or .dll file!
import SMComPy

BAUD_RATE = 115200
PORT = "/dev/ttyUSB0"
WIRED_FIRMWARE_MAX_RETRY_FOR_ONE_PACKET = 5

class SMCOM_WIRED_MESSAGES(Enum):
    #------------- Bootloader Messages ---------------- 
    ENTER_FIRMWARE_UPDATER_MODE     = 0
    FIRMWARE_PACKET_START           = 1 #!< Application side should not get this message! only from the bootloader
    FIRMWARE_PACKET                 = 2	#!< Application side should not get this message! only from the bootloader
    FIRMWARE_PACKET_END             = 3
    #------------- Application Messages ---------------- 
    GET_VERSION 					= 10
    AUTO_ADDRESSING_INIT 			= 11
    AUTO_ADDRESSING_SET_NEW_ID 		= 12
    START_BATCH_MEASUREMENT 		= 13
    GET_BATCH_MEASUREMENT 			= 14
    GET_CLEARANCE					= 15
    GET_CREST 						= 16
    GET_GRMS 						= 17
    GET_KURTOSIS 					= 18
    GET_SKEWNESS 					= 19
    GET_BATCH_MEASUREMENT_CHUNK 	= 20
    AUTO_ADDRESSING_INTEGRITY_CHECK = 21
    GET_ALL_TELEMETRY 				= 22
    GET_VRMS						= 23
    GET_PEAK						= 24
    GET_SUM							= 25

class WIRED_MESSAGE_STATUS(Enum):
    ERROR               = 0 	#!< [0] If the corresponding msg_handler fails put 0 for result status as an error, maybe additional message explanation
    SUCCESS             = 1 	#!< [1] If everything is okay handler sends 1 to indicate message is handled succesfully
    TIMEOUT             = 2		#!< [3] If the message handler sees a timeout error send this
    DATA                = 3		#!< [4] If we send data we will put first this result
    WRONG_MESSAGE       = 4 	#!< [5] If incoming message is broken or data is missing
    BROKEN_PACKET       = 5

acc_range_dict = {
    "2G":1,
    "4G":2,
    "8G":3,
    "16G":4,
}

sampling_frequency_dict = {
    "800":5,
    "1600":6,
    "3200":7,
    "6400":8,
    "12800":9
}

class PySMComPyPacket:
    data_len = 0			    
    receiver_id = 0		    
    transmitter_id = 0
    message_type = 0		    
    message_id = 0	         
    data = 0

    def verify_packet(self,message_id=None,transmitter_id=None,receiver_id = None,**kwargs):
        ret = True
        if(ret and message_id != None):
            ret = ret and self.message_id == message_id

        if(ret and transmitter_id != None):
            ret = ret and self.transmitter_id == transmitter_id

        if(ret and receiver_id != None):
            ret = ret and self.receiver_id == receiver_id

        return ret

class Wired(SMComPy.SMCOM_PUBLIC):
    accelerometer_coefficients= [(2*2)/(1<<16), (2*2)/(1<<16), (4*2)/(1<<16), (8*2)/(1<<16), (16*2)/(1<<16)]

    def __init__(self, port = PORT, rx_buffer_size = 1024, tx_buffer_size = 1024, device_id = 13):
        """
        Takes rx_buffer_size, tx_buffer_size and transmitter_id as arguments; 
        creates and returns a wired(inherited from SMComPy<SMCOM_PUBLIC>)
        """
        super().__init__(device_id)
        self.transmitter_id = id
        
        self.ser = serial.Serial(port, BAUD_RATE)

        atexit.register(self.__del__)
        
        self.data_queue = queue.Queue()
        self.mutex = threading.Lock()
        self.mutex_timeout = 10
        self.continue_thread = True
        
        self.listener_thread = threading.Thread(target=self.__thread_func__, daemon=True)
        self.listener_thread.start()
        time.sleep(1)

        self.mac_address = self.get_mac_address(255)
        self.version = self.get_version(255)

    def __thread_func__(self):
        while self.continue_thread:
            self.listener()
            time.sleep(0.01)
        print("Thread closed!")
        
    def __write__(self, buffer, length):
        if buffer == None or buffer == [] or buffer == "" or buffer == b'':
            return SMComPy.SMCOM_STATUS_FAIL
        elif type(buffer) == str:
            buffer = bytes(buffer, "utf-8")
        elif type(buffer) == int:
            buffer = bytes([buffer])
        elif type(buffer) == list or type(buffer) == tuple:
            if type(buffer[0]) == int:
                buffer = bytes(buffer)
            elif type(buffer[0]) == str:
                buffer = bytes("".join(buffer), "utf-8")

        if(self.mutex.acquire(blocking=True, timeout=self.mutex_timeout)):
            self.ser.write(buffer)
            self.ser.flush()
            self.mutex.release()
            return SMComPy.SMCOM_STATUS_SUCCESS
        else:
            print("Got no mutex!")

        return SMComPy.SMCOM_STATUS_TIMEOUT

    def __rx_callback__(self, event, status, packet):
        if(status != SMComPy.SMCOM_STATUS_SUCCESS):
            print("Error occured!")
            return
        temp_packet = PySMComPyPacket()
        temp_packet.data = packet.data
        temp_packet.message_id = packet.message_id			   
        temp_packet.receiver_id = packet.receiver_id		 
        temp_packet.transmitter_id = packet.transmitter_id
        temp_packet.message_type = packet.message_type  
        temp_packet.data_len = packet.data_len
        self.data_queue.put(temp_packet)

    def __tx_callback__(self, event, status, packet):
        pass
    
    def __available__(self):
        avlb = 0
        if(self.mutex.acquire(blocking=True,timeout=self.mutex_timeout)):
            avlb = self.ser.inWaiting()
            self.mutex.release()
        else:
            print("Got no mutex avlb!")
        return avlb

    def __read__(self, length):
        buffer = []
        pair = SMComPy.SMComPy_Pair()
        temp = []
        if(self.mutex.acquire(blocking=True,timeout=self.mutex_timeout)):
            temp = self.ser.read(length)
            self.mutex.release()

        if(len(temp) > 0):
            for i in temp:
                buffer.append(i)
            pair.vec = buffer
            pair.status = SMComPy.SMCOM_STATUS_SUCCESS
            return pair
        else:
            return None

    def get_version(self, id, timeout = 3):
        """
        Takes receiver id and timeout(default is 3) as argument and returns version of the device
        in format (MAJOR.MINOR.PATCH)(Ex. 1.0.12) with given id (15 or 255 are reserved as public id)
        """
        write_ret = self.write(id, SMCOM_WIRED_MESSAGES.GET_VERSION.value, [], 0)
        if(write_ret != SMComPy.SMCOM_STATUS_SUCCESS):
            return write_ret
        packet = self.data_queue.get(timeout = timeout)
        SMComPy_version = packet.data
        rec_id = packet.receiver_id
        # print(rec_id)
        # if rec_id != id:
        #     return SMComPy.SMCOM_STATUS_FAIL
        version = f"{SMComPy_version[2]}.{SMComPy_version[1]}.{SMComPy_version[0]}"
        return version
    
    def get_mac_address(self, id, timeout = 3):
        """
        Takes receiver id as argument and returns mac address of the device with given id as string in format
        (XX:XX:XX:XX:XX:XX) (15 or 255 are reserved as public id).
        """
        write_ret = self.write(id, SMCOM_WIRED_MESSAGES.AUTO_ADDRESSING_INIT.value, [0,0,0,0,0], 5)
        if write_ret != SMComPy.SMCOM_STATUS_SUCCESS:
            return write_ret

        packet = self.data_queue.get(timeout = timeout)
        # rec_id = packet.receiver_id

        # if rec_id != self.transmitter_id:
        #     return SMComPy.SMCOM_STATUS_FAIL

        data = packet.data
        data = tuple(data[:-3])
        return "%02X:%02X:%02X:%02X:%02X:%02X"%data

    def assign_new_id(self, mac_address, id):
        """
        Takes mac adress and id to be assigned to the given mac address as arguments assigns the 
        receiver id to the device which has mac address same as given and return None.
        """
        data = []
        data.append(id)
        mac_address = mac_address.split(':')
        for i in range(len(mac_address)):
            mac_address[i] = int(mac_address[i], base = 16)
        data.extend(mac_address)
        self.write(255, SMCOM_WIRED_MESSAGES.AUTO_ADDRESSING_SET_NEW_ID, data, len(data))
    
    def start_batch_measurement(self, id, acc, freq, sample_size, notify_measurement_end = True):
        if(sample_size <= 0 or sample_size >= 1000000 or (str(freq) not in sampling_frequency_dict.keys()) or (str(acc) not in acc_range_dict.keys()) ):
            #Return arg error here
            return None
        
        #Check the dictionaries
        acc_index = acc_range_dict[acc]
        freq_index = sampling_frequency_dict[str(freq)]
        #Below are all in bytes except sampling size, so no need to convert we already checked it
        data = [acc_index,freq_index,*tuple(sample_size.to_bytes(4, "little")), notify_measurement_end]

        #Check write!
        self.write(id, SMCOM_WIRED_MESSAGES.START_BATCH_MEASUREMENT.value, data, len(data))

        if(notify_measurement_end):
            #calculate end amount and give also additional time
            expected_timeout = sample_size/freq + (sample_size*0.1)
            data_packet = self.data_queue.get(timeout = expected_timeout)
            return (data_packet.data[0] == WIRED_MESSAGE_STATUS.SUCCESS.value)
        
        return True
        
    def read_measurement(self, id, sample_size, coefficient = 0, timeout = 10):
        #Wait for all packets
        byte_offset = 0 # Start from the beginning
        data_len = sample_size * 6 #Convert to bytes

        data = [*tuple(byte_offset.to_bytes(4, "little")),*tuple(data_len.to_bytes(4, "little"))]

        #Check write!
        self.write(id, SMCOM_WIRED_MESSAGES.GET_BATCH_MEASUREMENT_CHUNK.value, data, len(data))

        measurement_data = [[0]*sample_size,[0]*sample_size,[0]*sample_size]
        raw_measurement_data = []

        from math import ceil
        expected_packets = ceil(data_len/240)
        while(expected_packets != 0):
            packet = self.data_queue.get(timeout = 10)
            expected_packets -= 1
            raw_data = packet.data
            measurement_status = raw_data[0]
            measurement_data_len = raw_data[1]
            raw_measurement_data.extend(raw_data[2:])
                
        it = 0
        iter = 0

        coef = coefficient
        if(coef == 0):
            coef = 1 #multiply by itself

        while(it < len(raw_measurement_data)):
            one_packet = raw_measurement_data[it:it+6]
            it += 6
            measurement_data[0][iter] = int.from_bytes(one_packet[0:2],byteorder='little',signed=True)*coef
            measurement_data[1][iter] = int.from_bytes(one_packet[2:4],byteorder='little',signed=True)*coef
            measurement_data[2][iter] = int.from_bytes(one_packet[4:6],byteorder='little',signed=True)*coef
            iter += 1

        return measurement_data

    def measure(self, id, acc, freq, sample_size, timeout=10):
        if(self.start_batch_measurement(id, acc, freq, sample_size, notify_measurement_end=True) == True):
            coef = self.accelerometer_coefficients[acc_range_dict[acc]]
            return self.read_measurement(id,sample_size,coefficient= coef,timeout = timeout)
        else:
            print("Measurement failed")
            return None
    
    def get_all_telemetry(self, id, timeout = 30):
        #Check write!
        self.write(id, SMCOM_WIRED_MESSAGES.GET_ALL_TELEMETRY.value, [], 0)
        #Check also queue data receiver id!, maybe not desired message
        data = self.data_queue.get(timeout = timeout).data
        status = data[0]
        temperature = int.from_bytes(data[1:3],"little",signed=False)
        calibrated_frequency = int.from_bytes(data[3:7],"little",signed=False)
        telemetries = data[7:]

        import struct
        def convert_byte_list_to_double(bl):
            return struct.unpack('d',bytes(bl))[0]

        start = 0
        def byte_list_to_double_list(bl):
            nonlocal start
            dl = [  convert_byte_list_to_double(bl[start:start+8]),
                    convert_byte_list_to_double(bl[start+8:start+16]),
                    convert_byte_list_to_double(bl[start+16:start+24])]
            start += 8
            return dl
        major, minor, patch = self.version.split('.')
        
        clearance = byte_list_to_double_list(telemetries)
        crest = byte_list_to_double_list(telemetries)
        grms = byte_list_to_double_list(telemetries)
        kurtosis = byte_list_to_double_list(telemetries)
        skewness = byte_list_to_double_list(telemetries)

        telems = {
            "temperature":temperature/100,
            "calibrated_frequency":calibrated_frequency,
            "clearance":clearance,
            "crest":crest,
            "grms":grms,
            "kurtosis":kurtosis,
            "skewness":skewness
        }

        if int(patch) >= 9:
            vrms = byte_list_to_double_list(telemetries)
            peak = byte_list_to_double_list(telemetries)
            sum = byte_list_to_double_list(telemetries)
            telems["vrms"] = vrms
            telems["peak"] = peak
            telems["sum"] = sum
            
        if int(patch) >= 13:
            peak_to_peak = byte_list_to_double_list(telemetries)
            telems["peak_to_peak"] = peak_to_peak

        return telems

    def firmware_update(self, mac, bin_file_address, receiver_id = 255, timeout = 10):
        """
        Takes receiver_id, mac_address, address of binary file, and timeout(default 10) and return SMComPy_Status_t 
        or WIRED_MESSAGE_STATUS type as int.
        """
        bin_file = open(bin_file_address, "rb")
        packets = []

        while True:
            temp = bin_file.read(240)
            if temp == b'':
                break
            packets.append(list(temp))
        packets[-1] += [0] * (240 - len(packets[-1]))
        
        bin_file_size = 240*(len(packets)-1) + len(packets[-1])
        if bin_file_size == 0:
            print("Empty File")
            return
        if(isinstance(mac,str)):
            mac = [int(ff,16) for ff in mac.split(':')]    

        print("Device mac:",mac)
        enter_message = mac
        write_ret = self.write(receiver_id, SMCOM_WIRED_MESSAGES.ENTER_FIRMWARE_UPDATER_MODE.value, enter_message, len(enter_message))
        
        self.ser.baudrate = 1000000
        enter_mac_return = self.data_queue.get(timeout = timeout).data
        if enter_mac_return != mac:
            return SMComPy.SMCOM_STATUS_FAIL
        try:
            if(write_ret != SMComPy.SMCOM_STATUS_SUCCESS):
                return write_ret

            print("Device entered firmware updater mode")
            bootloader_id = 12 # This is predefined id for bootloader
            no_packets = len(packets) - 1

            start_message = [*tuple(bin_file_size.to_bytes(4, "little")),*tuple(no_packets.to_bytes(4, "little")), *tuple(mac)]
            write_ret = self.write(bootloader_id, SMCOM_WIRED_MESSAGES.FIRMWARE_PACKET_START.value, start_message, len(start_message))
            
            if write_ret != SMComPy.SMCOM_STATUS_SUCCESS:
                return write_ret
            
            read_ret = self.data_queue.get(timeout = timeout).data
            start_mac_return = read_ret[1:]
            status = read_ret[0]

            if mac != start_mac_return or status != WIRED_MESSAGE_STATUS.SUCCESS.value:
                return SMComPy.SMCOM_STATUS_FAIL
            
            for packet_no in range(no_packets + 1):
                retry = 0
                wired_packet_no = packet_no + 1
                success = False
                data_packet = [*tuple(packets[packet_no]), *tuple(mac), *tuple(wired_packet_no.to_bytes(2, "little"))]
                while retry < WIRED_FIRMWARE_MAX_RETRY_FOR_ONE_PACKET:
                    retry += 1
                    write_ret = self.write(bootloader_id, SMCOM_WIRED_MESSAGES.FIRMWARE_PACKET.value, data_packet, len(data_packet))
                    resp_msg_data = self.data_queue.get(timeout = timeout).data
                    # print(f"retry: {retry} for packet_no {wired_packet_no}")
                    if write_ret != SMComPy.SMCOM_STATUS_SUCCESS:
                        # print("write error:",write_ret)
                        continue
                    read_ret = resp_msg_data[0]
                    packet_mac_return = resp_msg_data[1:7]     # resp data[0] is wired status and 1: is mac address returned wrt the msg
                    ret_packet_no = resp_msg_data[7:]
                    if ret_packet_no[0] != wired_packet_no:
                        return SMComPy.SMCOM_STATUS_FAIL
                    if mac != packet_mac_return or read_ret != WIRED_MESSAGE_STATUS.SUCCESS.value:
                        # print("data error:",read_ret)
                        continue

                    success = True
                    # print(f"packet {wired_packet_no} success")
                    break

                if not success:
                    return SMComPy.SMCOM_STATUS_FAIL

            #Data transmission ended successfully
                    
            write_ret = self.write(bootloader_id, SMCOM_WIRED_MESSAGES.FIRMWARE_PACKET_END.value, [*tuple(mac)], len(mac))
            if write_ret != SMComPy.SMCOM_STATUS_SUCCESS:
                return write_ret

            resp_end = self.data_queue.get(timeout = timeout).data
            read_ret = resp_end[0]
            end_mac_return = resp_end[1:]
            if mac != end_mac_return or read_ret != WIRED_MESSAGE_STATUS.SUCCESS.value:
                return SMComPy.SMCOM_STATUS_FAIL
            time.sleep(12)
            self.ser.baudrate = 115200
            print("Firmware Updated to version:", self.get_version(255))
        except:
            print("An error occurred while firmware update")
        finally:
            self.ser.baudrate = 115200
    
    MESSAGES_SENSEWAY_WIRED = [
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        ("GET_VERSION",                     get_version),
        ("AUTO_ADDRESSING_INIT",            get_mac_address),
        ("AUTO_ADDRESSING_SET_NEW_ID",      0),
        ("START_BATCH_MEASUREMENT",         start_batch_measurement),
        ("GET_BATCH_MEASUREMENT",           read_measurement),
        ("GET_CLEARANCE",                   0),
        ("GET_CREST",                       0),
        ("GET_GRMS",                        0),
        ("GET_KURTOSIS",                    0),
        ("GET_SKEWNESS",                    0),
        ("GET_BATCH_MEASUREMENT_CHUNK",     0),
        ("AUTO_ADDRESSING_INTEGRITY_CHECK", 0),
        ("GET_ALL_TELEMETRY",               get_all_telemetry),
        ("GET_VRMS",                        0),
        ("GET_PEAK",                        0),
        ("GET_SUM",                         0)
    ]

    def __del__(self):
        self.continue_thread = False
        self.ser.close()

def parse_arg():
    parser = ArgumentParser(description = "SMComPy wired library to get measurements and update firmware")
    sub_parsers = parser.add_subparsers(help = 'sub-command help')
    update_parser = sub_parsers.add_parser('update', help = 'Update firmware of the device connected to given port with given bin file')
    update_parser.add_argument('port', metavar = 'PORT', type = str, help = "port address of the device (linux /dev/ttyUSBX, win32 COMX, X is an integer)")
    update_parser.add_argument('binfile', metavar = 'FILE', type = str, help = "address of the binary file containing the firmware update")

    update_parser = sub_parsers.add_parser('measure', help = 'Update firmware of the device connected to given port with given bin file')
    update_parser.add_argument('port', metavar = 'PORT', action = 'store', type = str, help = "port address of the device (linux /dev/ttyUSBX, win32 COMX, X is an integer)")
    update_parser.add_argument('acc', metavar = 'ACC', action = 'store', type = str, help = "acceleration range: Possible args: 2G, 4G, 8G, 16G")
    update_parser.add_argument('freq', metavar = 'FREQ', action = 'store', type = int, help = "sampling frequency: Possible args: 800, 1600, 3200, 6400, 12800")
    update_parser.add_argument('smpsize', metavar = 'SMPSIZE', action = 'store', type = int, help = "sampling size: Number of samples")
    update_parser.add_argument('--fileadr', metavar = '', action = 'store', type = str, default = stdout, help = "output file address which measurement data will be written")
    update_parser.add_argument('--telem', action = 'store_true', help = "can be set or notset. if set, telemetries will be written at the beginning of the file")
    data = parser.parse_args()

    if(len(argv) <= 1):
        print("No argument is provided, exiting")
        exit()

    if argv[1] == 'update':
        dev = Wired(port = data.port)
        dev.firmware_update(dev.mac_address, data.binfile)
    elif argv[1] == 'measure':
        dev = Wired(port = data.port)
        meas = dev.measure(255, data.acc, data.freq, data.smpsize)
        ls = []
        for i in range(len(meas[0])*3):
            ls.append(meas[i%3][i//3])
        terminal = False
        if data.fileadr == stdout:
            terminal = True
            f = stdout

        if not terminal: 
            f = open(data.fileadr, "w")
        if data.telem:
            telems = dev.get_all_telemetry(0xFF)
            for i in telems.keys():
                print(f"{i} : {telems[i]}", file = f)
        for i in ls:
            print(i, file = f)
        if not terminal:
            f.close()

if __name__ == "__main__":
    parse_arg()