#ifndef PRIVATE_NODE
#define PRIVATE_NODE

#include "SMCom.h"
#include <queue>
#include <cassert>
#include <string>


class private_node : public SMCom<SMCOM_PRIVATE>{
public:
    private_node(std::string name);
    SMCom_Status_t __write__(const uint8_t * buffer, uint16_t len);
	SMCom_Status_t __read__(uint8_t * buffer, uint16_t len);
	size_t __available__();
    void __tx_callback__(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PRIVATE * packet);
    void __rx_callback__(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PRIVATE * packet);

    //We will use these buffers to imitate communication line!
    std::queue<uint8_t> write_queue;
    std::queue<uint8_t> read_queue;
    std::string name;
};


#endif