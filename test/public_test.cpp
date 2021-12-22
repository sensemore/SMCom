#include "public_test.h"


public_node::public_node(uint16_t rx_buffer_size,
                         uint16_t tx_buffer_size, 
                         uint8_t id,
                         SMCom::rx_event_handler_callback rx, 
                         SMCom::tx_event_handler_callback tx,
                         std::string nodename) : SMCom(rx_buffer_size,tx_buffer_size, id,rx,tx)
{
    name = nodename;
    printf("Public node(%s) created, rx:%d,tx:%d | id:[%d]\n",name.c_str(),rx_buffer_size,tx_buffer_size,id);
}

SMCom_Status_t public_node::__write__(const uint8_t * buffer, uint16_t len){
    
    if(buffer == NULL) return SMCOM_STATUS_FAIL;
    for(uint16_t i = 0; i<len; ++i){
        
        if(1 || i != 3){
            write_queue.push(buffer[i]);
        }
        else{
            //CRC error must invoke!
            //write_queue.push(0xAB);
        }

        
    }

    return SMCOM_STATUS_SUCCESS;
}


void public_node::push_message_into_rx(const uint8_t * buffer, uint16_t len){
    for(uint16_t i = 0; i<len; ++i){
        read_queue.push(buffer[i]);
    }
}

SMCom_Status_t public_node::__read__(uint8_t * buffer, uint16_t len){

    if(len > read_queue.size()) return SMCOM_STATUS_FAIL;

    for(uint16_t i = 0; i<len; ++i){
        buffer[i] = read_queue.front();
        read_queue.pop();
    }
    return SMCOM_STATUS_SUCCESS;
}
size_t public_node::__available__(){
    return read_queue.size();
}

void public_node::print_rx(){
    std::queue<uint8_t> temp(read_queue);

    printf("Read queue(%s):",name.c_str());

    while(temp.empty() == 0){
        printf("%d,",temp.front());
        temp.pop();
    }
    printf("\n");
    
}

void public_node::print_tx(){
    std::queue<uint8_t> temp(write_queue);

    printf("Write queue(%s):",name.c_str());

    while(temp.empty() == 0){
        printf("%d,",temp.front());
        temp.pop();
    }
    printf("\n");
}


void public_node::copy_txqueue_into_another_rxqueue(public_node &node){
    while(write_queue.empty() == 0){
        node.read_queue.push(write_queue.front());
        write_queue.pop();
    }
}


namespace public_messages{

enum MESSAGES : uint8_t{
    GREETINGS = 0,
    PERSON,
    EMPTY,
};
enum STATUS : uint8_t{
	ERROR = 0,
	SUCCESS,
	NODEVICE,
	TIMEOUT,
	WAIT,
	DATA,	
	WRONG_MESSAGE,
};

typedef struct msg_greetings{
	char message[30];
}__attribute__((packed)) msg_greetins;

typedef struct msg_person{
	char name[20];
    char surname[20];
    uint8_t age;
    char city[20];
}__attribute__((packed)) msg_person;

};


static void public_rx_event_handler_callback(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PUBLIC * packet){
    (void)event;
    (void)status;
    printf("Packet length %d\n",packet->data_len);
    switch(packet->message_id){
        case public_messages::GREETINGS:{
            printf("Message GREETINGS invoked! Transmitter id[%d], Receiver id[%d]\n",packet->transmitter_id,packet->receiver_id);
            std::string msg((char*)packet->data);
            printf("Message length[%d]:%s\n",packet->data_len,msg.c_str());
            break;
        }
        case public_messages::PERSON:{
            printf("Message PERSON invoked! Transmitter id[%d], Receiver id[%d]\n",packet->transmitter_id,packet->receiver_id);
            break;
        }
        case public_messages::EMPTY:{
            printf("Message EMPTY invoked! Transmitter id[%d], Receiver id[%d]\n",packet->transmitter_id,packet->receiver_id);

            break;
        }
        default:{
            assert(0);
        }
    }
}

static void public_tx_event_handler_callback(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PUBLIC * packet){
    (void)event;
    (void)status;
    for(int i = 0; i < packet->data_len; i++){
        printf("%u ", packet->data[i]);
    }
    printf("\n");
    switch(packet->message_id){
        case public_messages::GREETINGS:{
            printf("Message 'GREETINGS' is sent from[%d] to [%d] | Status:%s\n",packet->transmitter_id,packet->receiver_id,SMCom<SMCOM_PUBLIC>::resolve_status(status));
            break;
        }
        case public_messages::PERSON:{
            printf("Message 'PERSON' is sent from[%d] to [%d]\n",packet->transmitter_id,packet->receiver_id);
            printf("Status:%d\n",status);
            break;
        }
        case public_messages::EMPTY:{
            printf("Message 'EMPTY' is sent from[%d] to [%d]\n",packet->transmitter_id,packet->receiver_id);
            printf("Status:%d\n",status);
            break;
        }
        default:{
            assert(0);
        }
    }
}

void public_test(){
    uint8_t idA = 10;
    uint8_t idB = 6;
    public_node nodeA(1024,0,idA,public_rx_event_handler_callback,public_tx_event_handler_callback,"nodeA");
    public_node nodeB(1024,0,idB,public_rx_event_handler_callback,public_tx_event_handler_callback,"nodeB");

    public_messages::msg_greetings greet_from_a = {0};
    strcpy(greet_from_a.message,"Hello my name is A");


    SMCom_Status_t listener_ret;
    
    printf("\n\n===============================================================\n\n");

    //Message from A to B
    nodeA.write(idB,public_messages::GREETINGS,(uint8_t*)&greet_from_a, sizeof(public_messages::msg_greetings));

    nodeA.print_tx();
    nodeA.copy_txqueue_into_another_rxqueue(nodeB);
    nodeB.print_rx();

    listener_ret = nodeB.listener();
    printf("Listener returned %s\n",nodeB.resolve_status(listener_ret));

    printf("\n\n===============================================================\n\n");

    nodeA.write(idB,public_messages::EMPTY,NULL,0);

    nodeA.print_tx();
    nodeA.copy_txqueue_into_another_rxqueue(nodeB);
    nodeB.print_rx();

    listener_ret = nodeB.listener();
    printf("Listener returned %s\n",nodeB.resolve_status(listener_ret));


    printf("\n\n===============================================================\n\n");    
}

// smcom_public_py::smcom_public_py(std::string name):SMCom(0,0,0,NULL,NULL)
// {
//     printf("Name :%s\n",name.c_str());
// }

// SMCom_Status_t smcom_public_py::__write__(const uint8_t * buffer, uint16_t len){
//     printf("Called write\n");
//     return SMCom_Status_t::SMCOM_STATUS_CRC_ERROR;
// }

// void inherit_ex(){
//     smcom_public_py spp("fbgencer");
//     spp.write(10,10,NULL,0);
// }