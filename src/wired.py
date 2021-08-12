import SMCom
import serial
import threading
import time
import queue

BAUD_RATE = 115200


from enum import Enum
class SMCOM_WIRED_MESSAGES(Enum):
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



class Wired(SMCom.SMCOM_PUBLIC):
    def __init__(self, rx_buffer_size = 1024, tx_buffer_size = 1024, device_id = 13):
        super().__init__(device_id)
        
        self.ser = serial.Serial('/dev/ttyUSB0', BAUD_RATE)
        
        self.data_queue = queue.Queue()
        self.mutex = threading.Lock()
        
        self.listener_thread = threading.Thread(target=self.__thread_func__, daemon=True)
        self.listener_thread.start()
        

    def __thread_func__(self):
        while True:
            self.listener()
        
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
        self.mutex.acquire()
        self.ser.write(buffer)
        self.mutex.release()
        return SMCom.SMCOM_STATUS_SUCCESS

    def __rx_callback__(self, event, status, packet):
        temp_packet = SMCom.pySMCOM_PUBLIC()
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
        self.mutex.acquire()
        avlb = self.ser.inWaiting()
        self.mutex.release()
        return avlb

    def __read__(self, length):
        buffer = []
        pair = SMCom.SMCom_Pair()
        
        self.mutex.acquire()
        temp = self.ser.read(length)
        self.mutex.release()

        for i in temp:
            buffer.append(i)
        pair.vec = buffer
        pair.status = SMCom.SMCOM_STATUS_SUCCESS
        return pair

    def get_version(self, id, timeout = 3):
        #Check the write return maybe we get error
        self.write(id, SMCOM_WIRED_MESSAGES.GET_VERSION.value, [], 0)
        #Check also queue data receiver id!, maybe not desired message
        #while(self.data_queue.qsize() == 0):
        #    self.listener()
        SMCom_version = self.data_queue.get(timeout = timeout).data
        version = f"{SMCom_version[2]}.{SMCom_version[1]}.{SMCom_version[0]}"
        return version
    
    def get_mac_adress(self, id, timeout = 3):
        #Check the write return maybe we get error
        self.write(id, SMCOM_WIRED_MESSAGES.AUTO_ADDRESSING_INIT.value, [0,0,0,0,0], 5)
        #Check also queue data receiver id!, maybe not desired message
        #while(self.data_queue.qsize() == 0):
        #    self.listener()
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
        self.write(id, SMCOM_WIRED_MESSAGES.START_BATCH_MEASUREMENT.value, data, len(data))
        if(notify_measurement_end):
            #calculate end amount and give also additional time
            expected_timeout = sample_size/freq + 10
            return self.data_queue.get(timeout = expected_timeout)
        
        
    def read_measurement(self, id, sample_size):
        #Wait for all packets
        byte_offset = 0 # Start from the beginning
        data_len = sample_size * 6 #Convert to bytes
        

    
    def measure(self,id,acc, freq, sample_size):
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
        #self.ser.close()


if __name__ == "__main__":
    nodeA = Wired(1024, 1024, 13)
    # nodeA.initialize_measurement(255, 3, 6, 10000, 1)
    print("SMCom version:", nodeA.read_measurement(255))
    print("...")
    nodeA.threadKilled = True
    nodeA.ser.close()


