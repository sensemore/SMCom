#ifndef THREAD_TEST_H
#define THREAD_TEST_H

#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include "SMCom.h"
#include <queue>


std::vector<std::thread> thread_test();


#define MAX_DELAY (0xFFFFFFFF)

template<typename T>
class sync_queue{

	std::queue<T> que;
	std::timed_mutex mutex;
	public:
	bool receive(T & rec, uint32_t timeout_ms);
	bool send(const T & rec, uint32_t timeout_ms);
};


class public_thread_node : public SMCom<SMCOM_PUBLIC>{
public:
    public_thread_node(uint8_t id, SMCom::rx_event_handler_callback rx, SMCom::tx_event_handler_callback tx, std::string name);
    SMCom_Status_t __write__(const uint8_t * buffer, uint16_t len);
	SMCom_Status_t __read__(uint8_t * buffer, uint16_t len);
	size_t __available__();

    //We will these buffers and functions will call them
    std::string name;
};



#endif