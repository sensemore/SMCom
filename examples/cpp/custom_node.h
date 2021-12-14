#ifndef CUSTOM_NODE
#define CUSTOM_NODE

#include "SMCom.h"
#include <queue>
#include <cassert>
#include <string>

#include <cstdio>      /* printf, scanf, puts, NULL */
#include <cstdlib>     /* srand, rand */
#include <ctime>       /* time */


PACK(struct custom_packet{
	uint8_t data_len;				
	uint8_t message_type:2;			
	uint8_t message_id:6;			
	uint16_t receiver_id;			
	uint16_t transmitter_id;
	uint32_t random_number;	
	uint8_t data[PACKET_DATA_LEN];
});
typedef struct custom_packet CUSTOM_PACKET;



class custom_node : public SMCom<CUSTOM_PACKET>{
public:
    custom_node(uint16_t node_id, std::string name);

	SMCom_Status_t write(uint16_t receiver_id, uint8_t message_id, const uint8_t * buffer, uint16_t len, uint8_t retry);


    SMCom_Status_t __write__(const uint8_t * buffer, uint16_t len);
	SMCom_Status_t __read__(uint8_t * buffer, uint16_t len);
	size_t __available__();
    void __tx_callback__(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PRIVATE * packet);
    void __rx_callback__(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PRIVATE * packet);

    //We will use these buffers to imitate communication line!
    std::queue<uint8_t> write_queue;
    std::queue<uint8_t> read_queue;
    std::string name;
	uint16_t my_id;
};


#endif