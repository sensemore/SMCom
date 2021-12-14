#include "private_node.h"

#define RX_BUFFER_SIZE (300)
#define TX_BUFFER_SIZE (300)

private_node::private_node(std::string nodename) : SMCom(RX_BUFFER_SIZE, TX_BUFFER_SIZE,NULL,NULL)
{
    name = nodename;
    printf("Private node(%s) created, rx:%d,tx:%d \n",name.c_str(),RX_BUFFER_SIZE,TX_BUFFER_SIZE);
}

SMCom_Status_t private_node::__write__(const uint8_t * buffer, uint16_t len){
    
    if(buffer == NULL) return SMCOM_STATUS_FAIL;

    for(uint16_t i = 0; i<len; ++i){
        write_queue.push(buffer[i]);
    }

    return SMCOM_STATUS_SUCCESS;
}


SMCom_Status_t private_node::__read__(uint8_t * buffer, uint16_t len){

    if(len > read_queue.size()) return SMCOM_STATUS_FAIL;

    for(uint16_t i = 0; i<len; ++i){
        buffer[i] = read_queue.front();
        read_queue.pop();
    }
    return SMCOM_STATUS_SUCCESS;
}

size_t private_node::__available__(){
    return read_queue.size();
}


namespace public_messages{

	enum MESSAGES : uint8_t{
		GREETINGS = 0,
		PERSON,
		EMPTY,
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


void private_node::__rx_callback__(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PRIVATE * packet){
    printf("Packet length %d\n",packet->data_len);
    switch(packet->message_id){
        case public_messages::GREETINGS:{
            printf("Message GREETINGS invoked!\n");
            std::string msg((char*)packet->data);
            printf("Message length[%d]:%s\n",packet->data_len,msg.c_str());
            break;
        }
        case public_messages::PERSON:{
            printf("Message PERSON invoked!");
            break;
        }
        case public_messages::EMPTY:{
            printf("Message EMPTY invoked!");

            break;
        }
        default:{
            assert(0);
        }
    }
}

void private_node::__tx_callback__(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PRIVATE * packet){
    for(int i = 0; i < packet->data_len; i++){
        printf("%u ", packet->data[i]);
    }
    printf("\n");
    switch(packet->message_id){
        case public_messages::GREETINGS:{
            printf("Message 'GREETINGS' is sent status:%s\n",SMCom<SMCOM_PRIVATE>::resolve_status(status));
            break;
        }
        case public_messages::PERSON:{
            printf("Message 'PERSON' is sent");
            printf("Status:%d\n",status);
            break;
        }
        case public_messages::EMPTY:{
            printf("Message 'EMPTY' is sent");
            printf("Status:%d\n",status);
            break;
        }
        default:{
            assert(0);
        }
    }
}
