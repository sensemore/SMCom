import SMCom
# import serial
# import serial.rs485 as rs485
# baud_rate = 

class Wired(SMCom.SMCOM_PUBLIC):
    def __init__(self, rx_buffer_size, tx_buffer_size, device_id):
        super().__init__(rx_buffer_size, tx_buffer_size, device_id)
        
    def __write__(self, buffer, length):
        if buffer == None:
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
        # ser = serial.Serial('/dev/tty---', baud_rate)
        # ser.rs485_mode = rs485.RS485Settings(rts_level_for_tx=True, rts_level_for_rx=False, loopback=False, delay_before_tx=None, delay_before_rx=None)
        # ser.write(buffer)
        return SMCom.SMCOM_STATUS_SUCCESS
        
    def __read__(self, buffer, length):
        # ser = serial.Serial('/dev/tty---', baud_rate)
        # ser.rs485_mode = rs485.RS485Settings(rts_level_for_tx=True, rts_level_for_rx=False, loopback=False, delay_before_tx=None, delay_before_rx=None)
        try:
            # buffer = ser.read(length)
            return SMCom.SMCOM_STATUS_SUCCESS
        except:
            return SMCom.SMCOM_STATUS_FAIL
        
    def __rx_callback__(self, event, status, packet):
        print(f"Packet length: {packet.data_len}")
        print(packet.message_id)
        print(packet.data)

    def __tx_callback__(self, event, status, packet):
        print("id : " + str(packet.message_id))
        print("data : " + str(packet.data))
        
    
nodeA = Wired(1024, 1024, 10)
nodeA.write_public(10, [], 0)
print("...")