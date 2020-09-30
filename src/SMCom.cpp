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
SMCom<SMCOM_PRIVATE>::SMCom(uint16_t _rx_buf_size, rx_event_handler_callback rx, tx_event_handler_callback tx ){
	//rx_buffer = (uint8_t*)malloc(_rx_buf_size);
	rx_buffer = new uint8_t[_rx_buf_size];
	rx_buf_size = _rx_buf_size;
	rx_event_handler_callback_ptr = rx;
	tx_event_handler_callback_ptr = tx;
}



template<>
SMCom_Status_t SMCom<SMCOM_PRIVATE>::write(SMCom_message_types t, uint8_t message_id, const uint8_t * buffer, uint8_t len){
	com_packet.message_type = t;
	com_packet.message_id = message_id;
	return common_write(buffer,len);
}

template<>
SMCom_Status_t SMCom<SMCOM_PRIVATE>::start_write_queue(SMCom_message_types t, uint8_t message_id, uint8_t len){
	com_packet.message_type = t;
	com_packet.message_id = message_id;
	com_packet.data_len = len;
	return common_start_write_queue();
}

template <>
SMCom_Status_t SMCom<SMCOM_PRIVATE>::request(uint8_t message_id, const uint8_t * buffer, uint8_t len, uint32_t timeout, request_response_callback fptr){
	com_packet.message_id = message_id;
	return common_request(buffer,len,timeout,fptr);
}

template<>
SMCom_Status_t SMCom<SMCOM_PRIVATE>::respond(uint8_t message_id, const uint8_t * buffer, uint8_t len){
	com_packet.message_type = SMCom_message_types::RESPONSE;
	com_packet.message_id = message_id;
	return common_write(buffer,len);
}

template<>
SMCom_Status_t SMCom<SMCOM_PRIVATE>::respond(const SMCOM_PRIVATE * inc_packet, const uint8_t * buffer, uint8_t len){
	com_packet.message_type = SMCom_message_types::RESPONSE;
	com_packet.message_id = inc_packet->message_id;
	return common_write(buffer,len);
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
	delete[] rx_buffer;
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
SMCom<SMCOM_PUBLIC>::SMCom(uint16_t _rx_buf_size, uint8_t id, rx_event_handler_callback rx, tx_event_handler_callback tx){
	rx_buffer = new uint8_t[_rx_buf_size];
	if(id > PUBLIC_ID_4BIT) id = PUBLIC_ID_4BIT;
	com_packet.transmitter_id = id;
	rx_buf_size = _rx_buf_size;
	rx_event_handler_callback_ptr = rx;
	tx_event_handler_callback_ptr = tx;
}

template<>
void SMCom<SMCOM_PUBLIC>::assign_new_id(uint8_t id){
	com_packet.transmitter_id = id;
}


template<>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::write(SMCom_message_types t, uint8_t receiver_id,uint8_t message_id, const uint8_t * buffer, uint8_t len){

	com_packet.message_type = t;
	com_packet.message_id = message_id;
	com_packet.receiver_id = receiver_id;
	
	return common_write(buffer,len);
}

template <>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::request(uint8_t receiver_id,uint8_t message_id, const uint8_t * buffer, uint8_t len, uint32_t timeout, request_response_callback fptr){
	
	com_packet.message_id = message_id;
	com_packet.receiver_id = receiver_id;
	return common_request(buffer,len,timeout,fptr);
}


template<>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::respond(const SMCOM_PUBLIC * inc_packet, const uint8_t * buffer, uint8_t len){

	com_packet.message_type = SMCom_message_types::RESPONSE;
	com_packet.message_id = inc_packet->message_id;
	com_packet.receiver_id = inc_packet->transmitter_id;	
	return common_write(buffer,len);
}

template<>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::respond(uint8_t receiver_id,uint8_t message_id, const uint8_t * buffer, uint8_t len){

	com_packet.message_type = SMCom_message_types::RESPONSE;
	com_packet.message_id = message_id;
	com_packet.receiver_id = receiver_id;
	
	return common_write(buffer,len);
}



template<>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::start_write_queue(SMCom_message_types t, uint8_t receiver_id,uint8_t message_id, uint8_t len){

	com_packet.message_type = t;
	com_packet.message_id = message_id;
	com_packet.receiver_id = receiver_id;
	com_packet.data_len = len;
	return common_start_write_queue();
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
	printf("Incoming packet transmitter_id %d | receiver_id %d\n",inc_packet->transmitter_id,inc_packet->receiver_id );
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
	delete[] rx_buffer;
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


//COMMON
#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
template<typename T>
void SMCom<T>::increase_ms_timer(){
	//Warn about this function, since we cannot use threads here, callbacks must return immeadiately
	++timeout_counter;
}
#endif

#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
template<typename T>
void SMCom<T>::run_request_scheduler(){
	//Check that do we have a timedout request?
	request_list_iterator prev;
	while( (prev = get_timedout_request() ) != request_list.end()){
		//Actual request is the next of this
		request_list_iterator it = std::next(prev);
		if(it == request_list.end()){
			printf("Error\n");
			return;
		}
		if(it->fptr != NULL){
			it->fptr(SMCOM_STATUS_TIMEOUT,&it->packet);
		}
		else{
			tx_event_handler_callback_ptr(SM_REQUEST_EVENT,SMCOM_STATUS_TIMEOUT,&it->packet);
		}
		request_list.erase_after(prev);
	}	
}
#endif

#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
template<typename T>
typename SMCom<T>::request_list_iterator SMCom<T>::get_request(T * packet){
	//We know that structs contain data_len/receiver_id/transmitter_id/message_id/message_type
	//In all these cases some types don't contain some variables(eg receiver id)
	//So skipping the data_len leaves the possible matches for requests
	//in private case: message_id is important
	//in public case: both message_id and receiver id is important etc.	
	uint8_t * ptr = ((uint8_t*)packet)+1; //Shift the data_len by shifting 1 byte
	if(!request_list.empty() ){
		auto it = request_list.before_begin();
		auto nextit = std::next(it);
		do{
			uint8_t * p2 = ((uint8_t*)(&nextit->packet)) + 1; //Again do not compare the data_len which could be different
			if(memcpy(ptr,p2,sizeof(T)-1) == 0){
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


#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
template<typename T>
bool SMCom<T>::remove_request_from_list(T * packet){
	request_list_iterator it = get_request(packet);
	if(it == request_list.end())
		return false;

	//So we have the iterator one item before the request
	request_list.erase_after(it);
	return true;
}
#endif


#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
template<typename T>
typename SMCom<T>::request_list_iterator SMCom<T>::get_timedout_request(){
	//Seek in the requests and return the previous iterator of the first timeout
	if(!request_list.empty() ){
		auto it = request_list.before_begin();
		auto nextit = std::next(it);
		do{
			if(timeout_counter >= nextit->timeout_end){
				//So timeout occured, return the iterator before next
				return it;
			}			
			it = nextit;
			nextit = std::next(it);
		}while(nextit != request_list.end());	
	}
	return 	request_list.end();
}
#endif


#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
template<typename T>
bool SMCom<T>::is_request_registered_before(){
	return get_request(&com_packet) != request_list.end();
}
#endif

#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
template<typename T>
SMCom_Status_t SMCom<T>::register_request(uint32_t timeout, request_response_callback fptr){
	if(is_request_registered_before()){
		return SMCOM_STATUS_FAIL;
	}

	//Do we need to stop timer ? Or just assume the below operation is fast
	request_packet rp;
	rp.timeout_end = timeout_counter+timeout; //There might be overflow here but timeout counter will also overflow at some point and leave us correct checking
	rp.packet = com_packet;
	rp.fptr = fptr;

	request_list.push_front(rp);	

	return SMCOM_STATUS_SUCCESS;
}
#endif


#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
template<typename T>
SMCom_Status_t SMCom<T>::common_request(const uint8_t * raw_bytes, uint8_t len, uint32_t timeout, request_response_callback fptr){

	if(raw_bytes == NULL && len != 0) return SMCOM_STATUS_FAIL;

	com_packet.message_type = SMCom_message_types::REQUEST;

	SMCom_Status_t ret = register_request(timeout,fptr);

	if(ret == SMCOM_STATUS_SUCCESS){
		ret = common_write(raw_bytes,len);
		if(ret != SMCOM_STATUS_SUCCESS){
			//Delete request, which is the first item in the list
			request_list.erase_after(request_list.before_begin());
		}
	}
	return ret;
}
#endif


template<typename T>
SMCom_Status_t SMCom<T>::common_start_write_queue(){

	if(rxflag.port_busy_flag){
		//If header is verified and port is busy, this flag is set to 1. 
		//We should'not allow to send message while we didn't take the whole message, even though it is not for us. port_busy flag will be cleared after getting end byte
		return SMCOM_STATUS_PORT_BUSY;
	}

	SMCom_Status_t ret = SMCOM_STATUS_DEFAULT;
	clear_tx_flag();
	last_crc = CRC_IBM_SEED;

	//Start sending start byte and head sequence
	uint8_t head = MESSAGE_START;
	last_crc = get_crc_ibm(&head,1,last_crc);
	ret = __write__(&head,1);									
		if(ret != SMCOM_STATUS_SUCCESS) return ret;

	txflag.start_byte_flag = 1;

	//Msg sequence
	last_crc = get_crc_ibm((uint8_t *)&com_packet,sizeof(com_packet),last_crc);
	ret = __write__((uint8_t *)&com_packet,sizeof(com_packet));
		if(ret != SMCOM_STATUS_SUCCESS) return ret;

	txflag.rx_tx_id_flag = 1;

	com_packet.data_len = 0;

	return ret;
}


template<typename T>
SMCom_Status_t SMCom<T>::common_push_to_queue(const uint8_t * buffer, uint8_t len){

	if(buffer == NULL && len != 0) return SMCOM_STATUS_FAIL;

	SMCom_Status_t ret = SMCOM_STATUS_DEFAULT;
	
	if(txflag.start_byte_flag == 0) return SMCOM_STATUS_START_BYTE_ERROR;
	if(txflag.rx_tx_id_flag == 0) return SMCOM_STATUS_FAIL;

	com_packet.data_len += len;

	if(com_packet.data_len > MAX_MSG_LENGTH) return SMCOM_STATUS_FAIL;

	//Data sequence
	last_crc = get_crc_ibm(buffer,len,last_crc);
	ret = __write__(buffer,len);
	if(ret != SMCOM_STATUS_SUCCESS) return ret;

	txflag.data_flag = 1;

	return ret;
}

template<typename T>
SMCom_Status_t SMCom<T>::common_finalize_queue(){
	SMCom_Status_t ret = SMCOM_STATUS_DEFAULT;
	
	if(txflag.start_byte_flag == 0) return SMCOM_STATUS_START_BYTE_ERROR;
	if(txflag.rx_tx_id_flag == 0) return SMCOM_STATUS_FAIL;
	if(txflag.data_flag == 0 || com_packet.data_len == 0) return SMCOM_STATUS_FAIL;


	uint8_t last_bytes[3] = {(uint8_t)((last_crc>>8)&0x00FF), (uint8_t)(last_crc & 0x00FF), MESSAGE_END};	
	ret = __write__(last_bytes,3);
	txflag.crc_flag = 1;


	if(tx_event_handler_callback_ptr!=NULL){
		tx_event_handler_callback_ptr((SMCom_event_types)com_packet.message_type,ret, &com_packet);
	}

	clear_tx_flag();

	return ret;
}



template<typename T>
SMCom_Status_t SMCom<T>::common_write(const uint8_t * buffer, uint8_t len){

	if(rxflag.port_busy_flag){
		//If header is verified and port is busy, this flag is set to 1. 
		//We should'not allow to send message while we didn't take the whole message, even though it is not for us. port_busy flag will be cleared after getting end byte
		return SMCOM_STATUS_PORT_BUSY;
	}

	if(buffer == NULL && len != 0)
		return SMCOM_STATUS_FAIL;

	clear_tx_flag();

	SMCom_Status_t ret = SMCOM_STATUS_DEFAULT;

	com_packet.data_len = len;

	uint16_t crc = CRC_IBM_SEED;
	
	//Start sequence
	uint8_t head = MESSAGE_START;
	crc = get_crc_ibm(&head,1,crc);
	ret = __write__(&head,1);									
		if(ret != SMCOM_STATUS_SUCCESS) return ret;

	txflag.start_byte_flag = 1;

	//Msg sequence
	crc = get_crc_ibm((uint8_t *)&com_packet,sizeof(com_packet),crc);
	ret = __write__((uint8_t *)&com_packet,sizeof(com_packet));
		if(ret != SMCOM_STATUS_SUCCESS) return ret;
	
	txflag.rx_tx_id_flag = 1;

	//Data sequence
	crc = get_crc_ibm(buffer,len,crc);
	ret = __write__(buffer,len);
	if(ret != SMCOM_STATUS_SUCCESS) return ret;

	txflag.data_flag = 1;
	
	//CRC sequence
	//In reverse order
	uint8_t last_bytes[3] = {(uint8_t)((crc>>8)&0x00FF), (uint8_t)(crc & 0x00FF), MESSAGE_END};

	txflag.crc_flag = 1;

	ret = __write__(last_bytes,3);


	if(tx_event_handler_callback_ptr!=NULL && com_packet.message_type != REQUEST)
		tx_event_handler_callback_ptr((SMCom_event_types)com_packet.message_type,ret, &com_packet);


	return ret;
}


template<typename T>
SMCom_Status_t SMCom<T>::common_verify_message_header(const uint8_t * raw_bytes, uint16_t * len){

	if(raw_bytes == NULL && *len != 0) return SMCOM_STATUS_NULL_MESSAGE;

	clear_rx_flag();
	rx_iter = 0;

	//Verifikasyon sizeof ile yapılabilir çünkü struct yapıları headerları barındırıyor. Ve start byte da içermeli
	if(*len != HEADER_SIZE ){
		return SMCOM_STATUS_HEADER_LENGTH_ERROR;
	}
	
	if(raw_bytes[0] != MESSAGE_START){
		return SMCOM_STATUS_START_BYTE_ERROR;
	}
	rx_iter = *len;
	memcpy(rx_buffer,raw_bytes,rx_iter);

	rxflag.start_byte_flag = 1;

	//Mesajın bitmesi 2.byte ile belirleniyor ve ek olarak 2 byte crc ile 1 byte end byte mesajı da alınmalı
	//Len artık bir sonraki mesajın boyunu söylüyor
	*len = rx_buffer[1] + 3;
	com_packet.data_len = *len;

	//maybe this message is not for us, maybe port is busy
	if(additional_buffer_check() != SMCOM_STATUS_SUCCESS){
		rxflag.rx_tx_id_flag = 0;
		rxflag.port_busy_flag = 1;
		return SMCOM_STATUS_PORT_BUSY;
	}
	//Set the flag we are responsible of this message!
	rxflag.rx_tx_id_flag = 1;

	return SMCOM_STATUS_SUCCESS;
}


template<typename T>
SMCom_Status_t SMCom<T>::common_handle_message_data(const uint8_t * raw_bytes, uint16_t len){

	SMCom_Status_t ret = SMCOM_STATUS_DEFAULT;
	if(raw_bytes == NULL && len != 0) return SMCOM_STATUS_NULL_MESSAGE;

	if(rxflag.rx_tx_id_flag == 0) ret = SMCOM_STATUS_PORT_BUSY;
	if(rxflag.start_byte_flag == 0) ret =  SMCOM_STATUS_START_BYTE_ERROR;
	if(rx_iter != HEADER_SIZE ) ret = SMCOM_STATUS_HEADER_LENGTH_ERROR;

	//Also include 3 bytes crc(2)+end(1)
	//The following error checkings should not invoke rx_callback function, they are strongly forbidden to user
	if(len != (rx_buffer[1]+3) ) ret = SMCOM_STATUS_MESSAGE_LENGTH_ERROR;
	if(raw_bytes[len-1] != MESSAGE_END) ret = SMCOM_STATUS_END_BYTE_ERROR;

	if(ret != SMCOM_STATUS_DEFAULT){
		clear_rx_flag();
		return ret;
	}
	rxflag.end_byte_flag = 1;

	uint8_t a1 = raw_bytes[len-2], a2 = raw_bytes[len-3];
	uint16_t crc_from_msg = a1 | (a2<<8);
	
	//crc can be applied two different chunks, while given the calculated crc to another crc. 
	//Here we calculated the first header bytes, then without COPYING the whole chunk(which could yield wrong crc)
	//we are directly calculating crc from the incoming bytes, then after verification we can copy
	uint16_t crc = get_crc_ibm(rx_buffer,rx_iter);
	//don't include crc + end bytes
	crc = get_crc_ibm(raw_bytes,len-3,crc);
	
	ret = SMCOM_STATUS_SUCCESS;

	//Even though crc check fails, we need to tell user only crc is broken, callback must be invoked to show user msg is received for itself
	if(crc != crc_from_msg) ret = SMCOM_STATUS_CRC_ERROR;
	rxflag.crc_flag = 1;

	memcpy(rx_buffer+rx_iter,raw_bytes,len-3);
	rx_iter += (len-3);

	//Skip the start byte and cast it to our message packet
	T * packet = (T *) (rx_buffer+1);
	SMCom_event_types evt = SM_WRITE_EVENT;

	rxflag.data_flag = 1;

	switch(packet->message_type){
		case SMCom_message_types::WRITE:
			evt = SM_WRITE_EVENT;
			break;
		case SMCom_message_types::REQUEST:
			evt = SM_REQUEST_EVENT;
			break;
		case SMCom_message_types::RESPONSE:
			evt = SM_RESPONSE_EVENT;
			break;						
		default:{
			break;
		}
	}

	#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
	if(evt == SM_RESPONSE_EVENT){
		//Check that is this a response from a registered request before ?
		request_list_iterator prev = check_incoming_response(packet);
		if(prev != request_list.end()){
			request_list_iterator it = std::next(prev);
			if(it->fptr != NULL){
				it->fptr(ret,packet);
				//set packet to null so we are not gonna call rxevent handler since we have already invoked this function
				packet = NULL;
			}
			//Remove the request
			request_list.erase_after(prev);
		}
	}
	#endif

	if(packet != NULL && rx_event_handler_callback_ptr != NULL){
		rx_event_handler_callback_ptr(evt,ret,packet);
	}

	clear_rx_flag();
	return ret;
}


template<typename T>
SMCom_Status_t SMCom<T>::handle_message_data(const uint8_t * raw_bytes, uint16_t len){
		return common_handle_message_data(raw_bytes,len);
}
template<typename T>
SMCom_Status_t SMCom<T>::verify_message_header(const uint8_t * raw_bytes, uint16_t * len){
	return common_verify_message_header(raw_bytes,len);
}
template<typename T>
SMCom_Status_t SMCom<T>::push_to_queue(const uint8_t * buffer, uint8_t len){
		return common_push_to_queue(buffer,len);
}
template<typename T>
SMCom_Status_t SMCom<T>::finalize_queue(){
	return common_finalize_queue();
}

template<typename CT>
CT * SMCom<CT>::duplicate_message_packet(const CT * packet){
	uint16_t sz = sizeof(packet) + packet->data_len;
	CT * p = (CT *) malloc(sz);
	memcpy(p,packet,sz);
	return p;
}

template<typename T>
uint8_t SMCom<T>::get_packet_data_length(const T * packet){
	return packet->data_len;
}


template<typename T>
void SMCom<T>::clear_tx_flag(){
	memset(&txflag,0,sizeof(message_flags));
	//txflag = {0};
}

template<typename T>
void SMCom<T>::clear_rx_flag(){
	memset(&rxflag,0,sizeof(message_flags));
	//rxflag = {0};
}

//CRC CODES
template<typename T>
uint16_t SMCom<T>::compute_crc_ibm(uint16_t crc, uint8_t data){
	for (uint8_t i = 0; i < 8; ++i){
		uint8_t b = ((crc & 0x8000) >> 8);
		crc <<= 1; // shift left once

		if( (b^(data & 0x80)) != 0){	
			crc ^= POLY_IBM; //xor with polynomial
		}

		data<<=1; // shift data to get next bit
	}
	return crc;
}

template<typename T>
uint16_t SMCom<T>::get_crc_ibm(const uint8_t * buffer, uint8_t len,uint16_t crc){
	
	for (uint8_t i = 0; i < len; ++i){
		crc = compute_crc_ibm(crc,buffer[i]);	
	}
	return crc;
}

template class SMCom<SMCOM_PUBLIC>;
template class SMCom<SMCOM_PRIVATE>;