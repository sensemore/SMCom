#ifndef PUBLIC_TEST
#define PUBLIC_TEST

#include "SMCom.h"
#include <queue>
#include <cassert>
#include <string>


class public_node : public SMCom<SMCOM_PUBLIC>{
public:
    public_node(uint8_t id, std::string name);
    SMCom_Status_t __write__(const uint8_t * buffer, uint16_t len);
	SMCom_Status_t __read__(uint8_t * buffer, uint16_t len);
	size_t __available__();
    void __tx_callback__(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PUBLIC * packet);
    void __rx_callback__(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PUBLIC * packet);

    void print_rx();
    void print_tx();

    //We will use these buffers and functions will call them
    std::queue<uint8_t> write_queue;
    std::queue<uint8_t> read_queue;
    std::string name;
};


#endif