import SMCom
import serial
from threading import Thread
import time
import queue

BAUD_RATE = 115200

class Wired(SMCom.SMCOM_PUBLIC):
    def __init__(self, rx_buffer_size, tx_buffer_size, device_id):
        super().__init__(rx_buffer_size, tx_buffer_size, device_id)
        self.ser = serial.Serial('/dev/ttyUSB0', BAUD_RATE)
        self.compacket = SMCom.pySMCOM_PUBLIC() 
        self.threadKilled = False
        self.q = queue.Queue()
        self.resume = True
        
    def __thread_func__(self):
        while not self.threadKilled:
            if self.resume:
                self.listener()
        
    def __write__(self, buffer, length):
        self.resume = False
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
        self.ser.write(buffer)
        self.resume = True
        return SMCom.SMCOM_STATUS_SUCCESS

    def __rx_callback__(self, event, status, packet):
        temp_packet = SMCom.pySMCOM_PUBLIC()
        temp_packet.data = packet.data
        temp_packet.message_id = packet.message_id			   
        temp_packet.receiver_id = packet.receiver_id		 
        temp_packet.transmitter_id = packet.transmitter_id
        temp_packet.message_type = packet.message_type  
        temp_packet.data_len = packet.data_len
        self.q.put(temp_packet)

    def __tx_callback__(self, event, status, packet):
        pass
    
    def __available__(self):
        return self.ser.inWaiting()

    def __read__(self, length):
        buffer = []
        pair = SMCom.SMCom_Pair()
        temp = self.ser.read(length)
        for i in temp:
            buffer.append(i)
        pair.vec = buffer
        pair.status = SMCom.SMCOM_STATUS_SUCCESS
        return pair

    def get_version(self, id):
        self.write(id, 10, [], 0)
        SMCom_version = self.q.get(timeout=5).data
        SMCom_version = f"{SMCom_version[2]}.{SMCom_version[1]}.{SMCom_version[0]}"
        return SMCom_version
    
    def get_mac_adress(self, id):
        self.write(id, 11, [0,0,0,0,0], 5)
        data = self.q.get(timeout = 5).data
        mac_adress = [0 for i in range(6)]
        for i in range(len(data)-6):
            mac_adress[i] = hex(data[i])
            temp = mac_adress[i][2:]
            mac_adress[i] = temp.upper() if len(temp) != 1 else "0"+temp
        mac_adress = ":".join(mac_adress)
        return mac_adress
            
    def initialize_measurement(self, id, acc_index, freq_index, sampling_size, measurement_end):
        data = list()
        data.append(int.from_bytes(acc_index.to_bytes(1, "little"), "little"))
        data.append(int.from_bytes(freq_index.to_bytes(1, "little"), "little"))
        data.extend(sampling_size.to_bytes(4, "little"))
        data.append(int.from_bytes(measurement_end.to_bytes(1, "little"), "little"))
        self.write(id, 13, data, 7)
        time.sleep(10)
        self.q.get(timeout = 5)
        
    def read_measurement(self, id):
        self.write(id, 14, [], 0)
        time.sleep(10)
        self.temp2 = self.q.get(timeout=5)
        temp = self.q.get(timeout = 5)
        print(temp.data)
        data = temp.data
        print(temp.data_len)
        return data

    def assign_address(self, id, mac_adress):
        data = [0 for i in range(7)]
        data[0] = id
        mac_adress = mac_adress.split(':')
        for i in range(len(mac_adress)):
            mac_adress[i] = int(mac_adress[i], base = 16)
        print(mac_adress)
        data[1:] = mac_adress
        self.write(255, 12, data, 7)
        return True
        
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

    # def __del__(self):
    #     self.ser.close()

nodeA = Wired(1024, 1024, 13)
t = Thread(target=nodeA.__thread_func__, daemon=True)
t.start()
# nodeA.initialize_measurement(255, 3, 6, 10000, 1)
print("SMCom version:", nodeA.get_version(255))
print("SMCom version:", nodeA.get_mac_adress(255))
print("SMCom version:", nodeA.get_version(255))
# print("SMCom version:", nodeA.get_mac_adress(255))
print("...")
# t.join(timeout=0)
nodeA.threadKilled = True
nodeA.ser.close()


