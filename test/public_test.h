#ifndef PUBLIC_TEST
#define PUBLIC_TEST

#include "SMCom.h"
#include <queue>
#include <cassert>
#include <string>

void public_test();
void inherit_ex();

class public_node : public SMCom<SMCOM_PUBLIC>{
public:
    public_node(uint16_t rx_buffer_size,uint16_t tx_buffer_size, uint8_t id, SMCom::rx_event_handler_callback rx, SMCom::tx_event_handler_callback tx,std::string name);
    SMCom_Status_t __write__(const uint8_t * buffer, uint16_t len);
	SMCom_Status_t __read__(uint8_t * buffer, uint16_t len);
	size_t __available__();
    //void __tx_callback__(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PUBLIC * packet);
    //void __rx_callback__(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PUBLIC * packet);

    void push_message_into_rx(const uint8_t * buffer, uint16_t len);
    void copy_txqueue_into_another_rxqueue(public_node &node);

    void print_rx();
    void print_tx();

    //We will these buffers and functions will call them
    std::queue<uint8_t> write_queue;
    std::queue<uint8_t> read_queue;
    std::string name;
};


#endif