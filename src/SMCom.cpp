#include "SMCom.h"

//======================================================================================
//SMCOM_PRIVATE
// +----------+-----------+-------+----------+----+---------+
// |          |           |       |          |    |         |
// |          |           |       |          |    |         |
// |START BYTE|DATA LENGTH|MESSAGE+<-+DATA+->+CRC | END BYTE|
// |          |           | TYPE  |          |    |         |
// |    1     |     1     |   1   |          | 2  |    1    |
// +----------+-----------+-------+----------+----+---------+

template<>
SMCom<SMCOM_PRIVATE>::SMCom(uint16_t rx_buf_size, uint16_t tx_buf_size, rx_event_handler_callback rx, tx_event_handler_callback tx){

	rx_buffer = new uint8_t[rx_buf_size];
	tx_buffer = (tx_buf_size > 0) ? new uint8_t[tx_buf_size] : NULL;

	this->rx_buf_size = rx_buf_size;
	this->tx_buf_size = tx_buf_size;

	rx_event_handler_callback_ptr = rx;
	tx_event_handler_callback_ptr = tx;

	clear_rx_flag();
	clear_tx_flag();
	clear_configuration_flags();
}

template<>
SMCom<SMCOM_PRIVATE>::SMCom(uint8_t * rx_buffer, uint16_t rx_buf_size, uint8_t * tx_buffer, uint16_t tx_buf_size, rx_event_handler_callback rx, tx_event_handler_callback tx){
	this->rx_buffer = rx_buffer;
	this->tx_buffer = tx_buffer;

	this->rx_buf_size = rx_buf_size;
	this->tx_buf_size = tx_buf_size;

	rx_event_handler_callback_ptr = rx;
	tx_event_handler_callback_ptr = tx;

	conflag.static_buffer_provided = true;

	clear_rx_flag();
	clear_tx_flag();
	clear_configuration_flags();
}


template<>
SMCom_Status_t SMCom<SMCOM_PRIVATE>::write(uint8_t message_id, const uint8_t * buffer, uint8_t len, uint8_t retry){
	if(message_id > SMCOM_MAX_USER_MESSAGE_ID) return SMCOM_STATUS_MESSAGE_ID_ERROR;
	com_packet.message_type = SMCom_message_types::WRITE;
	com_packet.message_id = message_id;
	return common_write(buffer,len,retry);
}

template<>
SMCom_Status_t SMCom<SMCOM_PRIVATE>::start_write_queue(SMCom_message_types t, uint8_t message_id, uint8_t len, uint8_t retry){
	if(message_id > SMCOM_MAX_USER_MESSAGE_ID) return SMCOM_STATUS_MESSAGE_ID_ERROR;
	com_packet.message_type = t;
	com_packet.message_id = message_id;
	com_packet.data_len = len;
	return common_start_write_queue(retry);
}

#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
template <>
SMCom_Status_t SMCom<SMCOM_PRIVATE>::request(uint8_t message_id, const uint8_t * buffer, uint8_t len, uint32_t timeout, request_response_callback fptr){
	if(message_id > SMCOM_MAX_USER_MESSAGE_ID) return SMCOM_STATUS_MESSAGE_ID_ERROR;
	com_packet.message_id = message_id;
	return common_request(buffer,len,timeout,fptr);
}
#endif

#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
template <>
SMCom_Status_t SMCom<SMCOM_PRIVATE>::ping(uint32_t timeout, request_response_callback fptr){
	com_packet.message_id = SMCom_special_messages::SMCOM_MSG_PING__;
	return common_request(NULL,0,timeout,fptr);
}
#endif

#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
template <>
SMCom_Status_t SMCom<SMCOM_PRIVATE>::get_version(uint32_t timeout, request_response_callback fptr){
	com_packet.message_id = SMCom_special_messages::SMCOM_MSG_GET_VERSION__;
	return common_request(NULL,0,timeout,fptr);
}
#endif


template<>
SMCom_Status_t SMCom<SMCOM_PRIVATE>::respond(uint8_t message_id, const uint8_t * buffer, uint8_t len, uint8_t retry){
	if(message_id > SMCOM_MAX_USER_MESSAGE_ID) return SMCOM_STATUS_MESSAGE_ID_ERROR;
	com_packet.message_type = SMCom_message_types::RESPONSE;
	com_packet.message_id = message_id;
	return common_write(buffer,len,retry);
}

template<>
SMCom_Status_t SMCom<SMCOM_PRIVATE>::respond(const SMCOM_PRIVATE * inc_packet, const uint8_t * buffer, uint8_t len,uint8_t retry){
	if(inc_packet->message_id > SMCOM_MAX_USER_MESSAGE_ID) return SMCOM_STATUS_MESSAGE_ID_ERROR;
	com_packet.message_type = SMCom_message_types::RESPONSE;
	com_packet.message_id = inc_packet->message_id;
	return common_write(buffer,len,retry);
}

template<>
SMCom_Status_t SMCom<SMCOM_PRIVATE>::respond_smcom_special_messages(SMCOM_PRIVATE * packet){

	com_packet.message_type = SMCom_message_types::RESPONSE;
	com_packet.message_id = packet->message_id;
	return common_respond_smcom_special_messages(packet);
}



template<>
SMCom_Status_t SMCom<SMCOM_PRIVATE>::additional_buffer_check(){
	//Nothing to check, there is no receiver/transmitter id in this type of a communicaiton
	return SMCOM_STATUS_SUCCESS;
}

#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
template<>
typename SMCom<SMCOM_PRIVATE>::request_list_iterator SMCom<SMCOM_PRIVATE>::check_incoming_response(SMCOM_PRIVATE * inc_packet){
	//packet is (response) from another device, we'll check did we have a request for this response
	//For private communication only message_id matters
	//Iterate in the list and check their message id
	if(!request_list.empty()){
		auto it = request_list.before_begin();
		auto nextit = std::next(it);
		do{
			if(nextit->packet.message_id == inc_packet->message_id){
				//NOTICE THAT ITERATOR RETURNS ONE ITEM BEFORE THE FOUND OBJECT TO USE ERASE AFTER FUNCTION, USE IT WITH CARE
				return it;
			}
			it = nextit;
			nextit = std::next(it);
		}while(nextit != request_list.end());
	}
	return 	request_list.end();
}
#endif


template<>
SMCom<SMCOM_PRIVATE>::~SMCom(){
	if(!conflag.static_buffer_provided){
		delete[] rx_buffer;
		delete[] tx_buffer;
	}
}

//===================================================================================================================================================================================
//SMCOM_PUBLIC
// +----------+-----------+--------+-----------+-------+----------+----+---------+
// |          |           |        |           |       |          |    |         |
// |          |           |        |           |       |          |    |         |
// |START BYTE|DATA LENGTH|RECEIVER|TRANSMITTER|MESSAGE+<-+DATA+->+CRC | END BYTE|
// |          |           |   ID   |     ID    | TYPE  |          |    |         |
// |    1     |     1     |  4bit  |   4bit    |   1   |          | 2  |    1    |
// +----------+-----------+--------+-----------+-------+----------+----+---------+

template<>
SMCom<SMCOM_PUBLIC>::SMCom(uint16_t rx_buf_size, uint16_t tx_buf_size, uint8_t id, rx_event_handler_callback rx, tx_event_handler_callback tx){
	rx_buffer = new uint8_t[rx_buf_size];
	tx_buffer = (tx_buf_size > 0) ? new uint8_t[tx_buf_size] : NULL;

	this->rx_buf_size = rx_buf_size;
	this->tx_buf_size = tx_buf_size;

	rx_event_handler_callback_ptr = rx;
	tx_event_handler_callback_ptr = tx;

	if(id > PUBLIC_ID_4BIT) id = PUBLIC_ID_4BIT;
	com_packet.transmitter_id = id;

	clear_rx_flag();
	clear_tx_flag();
	clear_configuration_flags();
}

template<>
SMCom<SMCOM_PUBLIC>::SMCom(uint8_t * rx_buffer, uint16_t rx_buf_size, uint8_t * tx_buffer, uint16_t tx_buf_size, uint8_t id, rx_event_handler_callback rx, tx_event_handler_callback tx){
	this->rx_buffer = rx_buffer;
	this->tx_buffer = tx_buffer;

	this->rx_buf_size = rx_buf_size;
	this->tx_buf_size = tx_buf_size;

	rx_event_handler_callback_ptr = rx;
	tx_event_handler_callback_ptr = tx;

	if(id > PUBLIC_ID_4BIT) id = PUBLIC_ID_4BIT;
	com_packet.transmitter_id = id;

	conflag.static_buffer_provided = true;

	clear_rx_flag();
	clear_tx_flag();
	clear_configuration_flags();
}

template<>
void SMCom<SMCOM_PUBLIC>::assign_new_id(uint8_t id){
	com_packet.transmitter_id = id;
}


template<>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::write(uint8_t receiver_id,uint8_t message_id, const uint8_t * buffer, uint8_t len,uint8_t retry){

	if(message_id > SMCOM_MAX_USER_MESSAGE_ID) return SMCOM_STATUS_MESSAGE_ID_ERROR;

	com_packet.message_type = SMCom_message_types::WRITE;
	com_packet.message_id = message_id;
	com_packet.receiver_id = receiver_id;
	return common_write(buffer,len,retry);
}



template<>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::write_public(uint8_t message_id, const uint8_t * buffer, uint8_t len,uint8_t retry){
	return write(PUBLIC_ID_4BIT,message_id, buffer, len, retry);
}


#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
template <>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::request(uint8_t receiver_id,uint8_t message_id, const uint8_t * buffer, uint8_t len, uint32_t timeout, request_response_callback fptr){
	
	if(message_id > SMCOM_MAX_USER_MESSAGE_ID) return SMCOM_STATUS_MESSAGE_ID_ERROR;

	com_packet.message_id = message_id;
	com_packet.receiver_id = receiver_id;
	return common_request(buffer,len,timeout,fptr);
}
#endif

#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
template <>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::ping(uint8_t receiver_id, uint32_t timeout, request_response_callback fptr){
	com_packet.message_id = SMCom_special_messages::SMCOM_MSG_PING__;
	com_packet.receiver_id = receiver_id;
	return common_request(NULL,0,timeout,fptr);
}
#endif

#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
template <>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::get_version(uint8_t receiver_id, uint32_t timeout, request_response_callback fptr){
	com_packet.message_id = SMCom_special_messages::SMCOM_MSG_GET_VERSION__;
	com_packet.receiver_id = receiver_id;
	return common_request(NULL,0,timeout,fptr);
}
#endif


template<>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::respond(const SMCOM_PUBLIC * inc_packet, const uint8_t * buffer, uint8_t len,uint8_t retry){

	if(inc_packet->message_id > SMCOM_MAX_USER_MESSAGE_ID) return SMCOM_STATUS_MESSAGE_ID_ERROR;

	com_packet.message_type = SMCom_message_types::RESPONSE;
	com_packet.message_id = inc_packet->message_id;
	com_packet.receiver_id = inc_packet->transmitter_id;
	return common_write(buffer,len,retry);
}

template<>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::respond(uint8_t receiver_id,uint8_t message_id, const uint8_t * buffer, uint8_t len,uint8_t retry){

	if(message_id > SMCOM_MAX_USER_MESSAGE_ID) return SMCOM_STATUS_MESSAGE_ID_ERROR;

	com_packet.message_type = SMCom_message_types::RESPONSE;
	com_packet.message_id = message_id;
	com_packet.receiver_id = receiver_id;

	return common_write(buffer,len,retry);
}


template<>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::respond_smcom_special_messages(SMCOM_PUBLIC * packet){

	com_packet.message_type = SMCom_message_types::RESPONSE;
	com_packet.message_id = packet->message_id;
	com_packet.receiver_id = packet->transmitter_id;

	return common_respond_smcom_special_messages(packet);
}


template<>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::start_write_queue(SMCom_message_types t, uint8_t receiver_id, uint8_t message_id, uint8_t len, uint8_t retry){

	if(message_id > SMCOM_MAX_USER_MESSAGE_ID) return SMCOM_STATUS_MESSAGE_ID_ERROR;

	com_packet.message_type = t;
	com_packet.message_id = message_id;
	com_packet.receiver_id = receiver_id;
	com_packet.data_len = len;
	return common_start_write_queue(retry);
}


template<>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::additional_buffer_check(){
	//Check that is this message for us, we are just gonna check receiver id, which must be our ID

	SMCOM_PUBLIC * ptr = (SMCOM_PUBLIC *) (rx_buffer+1);

	if(ptr->receiver_id == com_packet.transmitter_id || ptr->receiver_id == SMCom_headers::PUBLIC_ID_4BIT){
		return SMCOM_STATUS_SUCCESS;
	}

	return SMCOM_STATUS_PORT_BUSY;

}

#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
template<>
typename SMCom<SMCOM_PUBLIC>::request_list_iterator SMCom<SMCOM_PUBLIC>::check_incoming_response(SMCOM_PUBLIC * inc_packet){
	//packet is (response) from another device, we'll check did we have a request for this response
	//Iterate in the list and check their message id + their transmitter id must be our request receiver id
	if(!request_list.empty()){
		auto it = request_list.before_begin();
		auto nextit = std::next(it);
		do{
			if(nextit->packet.message_id == inc_packet->message_id &&
				(nextit->packet.receiver_id == inc_packet->transmitter_id || nextit->packet.receiver_id == PUBLIC_ID_4BIT) ){
				//NOTICE THAT ITERATOR RETURNS ONE ITEM BEFORE THE FOUND OBJECT TO USE ERASE AFTER FUNCTION, USE IT WITH CARE
				return it;
			}
			it = nextit;
			nextit = std::next(it);
		}while(nextit != request_list.end());
	}
	return 	request_list.end();
}
#endif

template<>
SMCom<SMCOM_PUBLIC>::~SMCom(){
	if(!conflag.static_buffer_provided){
		delete[] rx_buffer;
		delete[] tx_buffer;
	}
}


//======================================================================================
//SMCOM_PUBLIC_8BIT_ADDRESS
// +----------+-----------+--------+-----------+-------+----------+----+---------+
// |          |           |        |           |       |          |    |         |
// |          |           |        |           |       |          |    |         |
// |START BYTE|DATA LENGTH|RECEIVER|TRANSMITTER|MESSAGE+<-+DATA+->+CRC | END BYTE|
// |          |           |   ID   |     ID    | TYPE  |          |    |         |
// |    1     |     1     |   1    |     1     |   1   |          | 2  |    1    |
// +----------+-----------+--------+-----------+-------+----------+----+---------+

/*
template<>
SMCom<SMCOM_PUBLIC_8BIT_ADDRESS>::SMCom(uint16_t _rx_buf_size, uint8_t id){
	rx_buffer = new uint8_t[_rx_buf_size];
	//rx_buffer = (uint8_t*)malloc(rx_buf_size);
	com_packet.transmitter_id = id;
}

template<>
SMCom<SMCOM_PUBLIC_8BIT_ADDRESS>::~SMCom(){
	delete[] rx_buffer;
}*/


//======================================================================================
//SMCOM_ONLY_MASTER
// +----------+-----------+--------+-------+----------+----+---------+
// |          |           |        |       |          |    |         |
// |          |           |        |       |          |    |         |
// |START BYTE|DATA LENGTH|RECEIVER|MESSAGE+<-+DATA+->+CRC | END BYTE|
// |          |           |   ID   | TYPE  |          |    |         |
// |    1     |     1     |   1    |   1   |          | 2  |    1    |
// +----------+-----------+--------+-------+----------+----+---------+

/*
template<>
SMCom<SMCOM_ONLY_MASTER>::SMCom(uint16_t _rx_buf_size, rx_event_handler_callback rx, tx_event_handler_callback tx ){
	rx_buffer = new uint8_t[_rx_buf_size];
}
template<>
SMCom<SMCOM_ONLY_MASTER>::~SMCom(){
	delete[] rx_buffer;
}

*/