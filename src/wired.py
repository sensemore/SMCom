import SMCom
import serial
import threading
import time
import queue
import atexit
from enum import Enum

BAUD_RATE = 115200


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


class PySMComPacket:
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

class Wired(SMCom.SMCOM_PUBLIC):
    accelerometer_coefficients= [(2*2)/(1<<16), (2*2)/(1<<16), (4*2)/(1<<16), (8*2)/(1<<16), (16*2)/(1<<16)]

    def __init__(self, rx_buffer_size = 1024, tx_buffer_size = 1024, device_id = 13):
        super().__init__(device_id)
        
        self.ser = serial.Serial('/dev/ttyUSB0', BAUD_RATE)

        atexit.register(self.__cleanup)
        
        self.data_queue = queue.Queue()
        self.mutex = threading.Lock()
        self.mutex_timeout = 10

        self.continue_thread = True
        
        self.listener_thread = threading.Thread(target=self.__thread_func__, daemon=True)
        self.listener_thread.start()

        print("Constuctor called !")

    def __thread_func__(self):
        while self.continue_thread:
            self.listener()
            time.sleep(0.01)
            #time.sleep(0.5) ## bug fix için
        print("Thread closed!")
        
    def __write__(self, buffer, length):
        if buffer == None or buffer == [] or buffer == "" or buffer == b'':
            return SMCom.SMCOM_STATUS_FAIL
        elif type(buffer) == str:
            buffer = bytes(buffer, "utf-8")
        elif type(buffer) == int:
            buffer = bytes([buffer])
        elif type(buffer) == list or type(buffer) == tuple:
            if type(buffer[0]) == int:
                buffer = bytes(buffer)
            elif type(buffer[0]) == str:
                buffer = bytes("".join(buffer), "utf-8")

        if(self.mutex.acquire(blocking=True,timeout=self.mutex_timeout)):
            self.ser.write(buffer)
            self.mutex.release()
            return SMCom.SMCOM_STATUS_SUCCESS
        else:
            print("Got no mutex!")

        return SMCom.SMCOM_STATUS_TIMEOUT

    def __rx_callback__(self, event, status, packet):
        if(status != SMCom.SMCOM_STATUS_SUCCESS):
            print("Error occured!")
            return
        temp_packet = PySMComPacket()#SMCom.pySMCOM_PUBLIC()
        #print(id(temp_packet))
        temp_packet.data = packet.data
        temp_packet.message_id = packet.message_id			   
        temp_packet.receiver_id = packet.receiver_id		 
        temp_packet.transmitter_id = packet.transmitter_id
        temp_packet.message_type = packet.message_type  
        temp_packet.data_len = packet.data_len
        #print("__rx invoked, putting:",temp_packet.data,temp_packet.data_len)
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
        pair = SMCom.SMCom_Pair()
        temp = []
        if(self.mutex.acquire(blocking=True,timeout=self.mutex_timeout)):
            temp = self.ser.read(length)
            self.mutex.release()

        if(len(temp) > 0):
            for i in temp:
                buffer.append(i)
            pair.vec = buffer
            pair.status = SMCom.SMCOM_STATUS_SUCCESS
            return pair
        else:
            return None

    def dirty_listener(self,wait_packet_no):
        return
        while(self.data_queue.qsize() == 0):
            self.listener()
        
        while(wait_packet_no != 0):
            self.listener()
            wait_packet_no -= 1

    def get_version(self, id, timeout = 3):
        #Check the write return maybe we get error
        write_ret = self.write(id, SMCOM_WIRED_MESSAGES.GET_VERSION.value, [], 0)
        if(write_ret != SMCom.SMCOM_STATUS_SUCCESS):
            return write_ret
        #Check also queue data receiver id!, maybe not desired message
        SMCom_version = self.data_queue.get(timeout = timeout).data
    
        version = f"{SMCom_version[2]}.{SMCom_version[1]}.{SMCom_version[0]}"
        return version
    
    def get_mac_adress(self, id, timeout = 3):
        #Check the write return maybe we get error
        self.write(id, SMCOM_WIRED_MESSAGES.AUTO_ADDRESSING_INIT.value, [0,0,0,0,0], 5)
        #Check also queue data receiver id!, maybe not desired message

        data = self.data_queue.get(timeout = timeout).data
        data = tuple(data[:-3])
        return "%02X:%02X:%02X:%02X:%02X:%02X"%data
            
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
            #self.dirty_listener(1)
            data_packet = self.data_queue.get(timeout = expected_timeout)
            print("measurement ended:",data_packet.data[0],data_packet.data[0]==WIRED_MESSAGE_STATUS.SUCCESS.value,WIRED_MESSAGE_STATUS.SUCCESS.value)
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
        print("Exptected packets",expected_packets)
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

    
    def measure(self,id,acc, freq, sample_size, timeout=10):
        if(self.start_batch_measurement(id,acc,freq,sample_size,notify_measurement_end=True) == True):
            coef = self.accelerometer_coefficients[acc_range_dict[acc]]
            return self.read_measurement(id,sample_size,coefficient= coef,timeout = timeout)

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
            return struct.unpack('d',bytes(bl))
        # double clearance[3];
        # double crest[3];
        # double grms[3];
        # double kurtosis[3];
        # double skewness[3];
        # //Included in v1.0.9
        # double vrms[3];
        # double peak[3];
        # double sum[3];
        # //Included in v1.0.13
        # double peak_to_peak[3];

    
        clearance = [   convert_byte_list_to_double(telemetries[0:8]),
                        convert_byte_list_to_double(telemetries[8:16]),
                        convert_byte_list_to_double(telemetries[16:24])]
    

        telemetries = {
            "temperature":temperature/100,
            "calibrated_frequency":calibrated_frequency,
            "clearance":clearance,
        }

        return telemetries

    def firmware_update(self,id,mac,bin_file,timeout = 10):
        #Bin dosyası okunup bir list içine alınacak ve list içinde ayrı listelerde paket paket datalar olacak
        #Max data boyutu 240 byte olmalı
        
        packets = [] #fill
        bin_file_size = 0 #fill
        #Read file here


        if(isinstance(mac,str)):
            mac = [int(ff,16) for ff in mac.split(':')]    

        print("Device mac:",mac)
        enter_message = mac
        write_ret = self.write(id, SMCOM_WIRED_MESSAGES.ENTER_FIRMWARE_UPDATER_MODE.value, enter_message, len(enter_message))
        if(write_ret != SMCom.SMCOM_STATUS_SUCCESS):
            return write_ret
        #self.ser.baudrate = 1000000
        packet = self.data_queue.get(timeout = 10)
        #Verify packet!
        if(packet.verify_packet(message_id = SMCOM_WIRED_MESSAGES.ENTER_FIRMWARE_UPDATER_MODE.value, transmitter_id = id) == False):
            return "invalid device entered fu-mode"

        print("Device entered firmware updater mode")
        id = 12 # This is predefined id for bootloader
        no_packets = len(packets)

        #parse mac as list
        start_message = [*tuple(bin_file_size.to_bytes(4, "little")),*tuple(no_packets.to_bytes(4, "little")), *tuple(mac) ]

        write_ret = self.write(id, SMCOM_WIRED_MESSAGES.FIRMWARE_PACKET_START.value, start_message, len(start_message))
        write_ret = self.write(id, SMCOM_WIRED_MESSAGES.FIRMWARE_PACKET.value, [], 0)
        write_ret = self.write(id, SMCOM_WIRED_MESSAGES.FIRMWARE_PACKET_END.value, [], 0)




        pass
    
        
    MESSAGES_SENSEWAY_WIRED = \
    [
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        ("GET_VERSION",                     get_version),
        ("AUTO_ADDRESSING_INIT",            0),
        ("AUTO_ADDRESSING_SET_NEW_ID",      0),
        ("START_BATCH_MEASUREMENT",         0),
        ("GET_BATCH_MEASUREMENT",           0),
        ("GET_CLEARANCE",                   0),
        ("GET_CREST",                       0),
        ("GET_GRMS",                        0),
        ("GET_KURTOSIS",                    0),
        ("GET_SKEWNESS",                    0),
        ("GET_BATCH_MEASUREMENT_CHUNK",     0),
        ("AUTO_ADDRESSING_INTEGRITY_CHECK", 0),
        ("GET_ALL_TELEMETRY",               0),
        ("GET_VRMS",                        0),
        ("GET_PEAK",                        0),
        ("GET_SUM",                         0)
    ]

    def __del__(self):
        print("dest called")
        self.continue_thread = False
        self.ser.close()

    def __cleanup(self):
        print("Closing serial port!")
        self.continue_thread = False
        self.ser.close()




if __name__ == "__main__":
    pass



