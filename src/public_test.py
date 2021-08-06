import SMCom

class public_node(SMCom.SMCOM_PUBLIC):
    def __init__(self, rx_buffer_size, tx_buffer_size, id, nodename, rx = None):
        super().__init__(rx_buffer_size, tx_buffer_size, id)
        print(f"{nodename} is created rx:{rx_buffer_size} tx:{tx_buffer_size} id:{id}")
        self.write_queue = []
        self.read_queue = []
        self.name = nodename
    
    def __write__(self, buffer, length):
        if buffer == None:
            return SMCom.SMCOM_STATUS_FAIL
        for i in range(length):
            self.write_queue.append(buffer[i])
        print("__write__: " + str(buffer))
        return SMCom.SMCOM_STATUS_SUCCESS
    
    def push_message_into_rx(self, buffer, length):
        for i in range(length):
            self.read_queue.push(buffer[i])
        return SMCom.SMCOM_STATUS_SUCCESS

    def __read__(self, buffer, length):
        if length > len(self.read_queue):
            return SMCom.SMCOM_STATUS_FAIL
        for i in range(length):
            buffer[i] = self.read_queue[0]
            self.req_queue.pop(0)
        return SMCom.SMCOM_STATUS_SUCCESS
    
    def __available__(self):
        return len(self.read_queue)
    
    def print_rx(self):
        temp = self.read_queue.copy()
        print(f"Read queue: {self.name}")
        while len(temp) != 0:
            print(temp[0])
            temp.pop(0)

    def print_tx(self):
        temp = self.write_queue.copy()
        print(f"Write queue: {self.name}")
        while(len(temp) != 0):
            print(temp[0])
            temp.pop(0)
    
    def __rx_callback__(self, event, status, packet):
        print(f"Packet length: {packet.data_len}")
        print(packet.message_id)
        print(packet.data)

    def __tx_callback__(self, event, status, packet):
        print("id : " + str(packet.message_id))
        print("data : " + str(packet.data))

    def copy_txqueue_into_another_rxqueue(self, node):
        while(len(self.write_queue) == 0):
            node.read_queue.append(self.write_queue[0])
            self.write_queue.pop(0)


idA = 10
# idB = 6
nodeA = public_node(1024, 1024, idA, "NodeA")
nodeA.write_public(10, [0,1,2,3], 4)
# nodeB = public_node(1024, 1024, idB, "NodeB")
# x = [0,2,2,3,5]
# s = pySMCOM_PUBLIC()
# temp = nodeA.write(idA, 0, x, 5)

# duplicate_message_packet()

# nodeA.print_rx()
# nodeA.copy_txqueue_into_another_rxqueue(nodeB)
# nodeB.print_tx()

# listener_ret = nodeB.listener()
# print("Listener Returned : ", nodeB.resolve_status(listener_ret))

# print("="*40)

