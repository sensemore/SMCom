//COMMON

template<typename T>
SMCom_Status_t SMCom<T>::common_start_write_queue(T * out_packet, uint8_t retry){

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

	ret = __write__retry(&head,1,retry);
	if(ret != SMCOM_STATUS_SUCCESS){
		return ret;
	}

	txflag.start_byte_flag = 1;
	//Msg sequence
	last_crc = get_crc_ibm((uint8_t *)out_packet,sizeof(T),last_crc);
	
	ret = __write__retry((uint8_t *)out_packet,sizeof(T),retry);
	
	txflag.rx_tx_id_flag = 1;

	out_packet->data_len = 0;

	return ret;
}


template<typename T>
SMCom_Status_t SMCom<T>::common_push_to_queue(T * out_packet, const uint8_t * buffer, uint8_t len, uint8_t retry){

	if(buffer == NULL && len != 0) return SMCOM_STATUS_FAIL;

	SMCom_Status_t ret = SMCOM_STATUS_DEFAULT;

	if(txflag.start_byte_flag == 0) return SMCOM_STATUS_START_BYTE_ERROR;
	if(txflag.rx_tx_id_flag == 0) return SMCOM_STATUS_FAIL;

	out_packet->data_len += len;

	if(out_packet->data_len > MAX_MSG_LENGTH) return SMCOM_STATUS_FAIL;

	//Data sequence
	last_crc = get_crc_ibm(buffer,len,last_crc);

	ret = __write__retry(buffer,len,retry);
	if(ret != SMCOM_STATUS_SUCCESS){
		return ret;
	}

	txflag.data_flag = 1;

	return ret;
}

template<typename T>
SMCom_Status_t SMCom<T>::common_finalize_queue(T * out_packet, uint8_t retry){
	SMCom_Status_t ret = SMCOM_STATUS_DEFAULT;

	if(txflag.start_byte_flag == 0) return SMCOM_STATUS_START_BYTE_ERROR;
	if(txflag.rx_tx_id_flag == 0) return SMCOM_STATUS_FAIL;
	if(txflag.data_flag == 0 || out_packet->data_len == 0) return SMCOM_STATUS_FAIL;


	uint8_t last_bytes[3] = {(uint8_t)((last_crc>>8)&0x00FF), (uint8_t)(last_crc & 0x00FF), MESSAGE_END};
	
	ret = __write__(last_bytes,3,retry);
	if(ret != SMCOM_STATUS_SUCCESS){
		return ret;
	}
	
	txflag.crc_flag = 1;

	if(tx_event_handler_callback_ptr != NULL){
		tx_event_handler_callback_ptr((SMCom_event_types)out_packet->message_type,ret, out_packet);
	}

	clear_tx_flag();

	return ret;
}


template<typename T>
SMCom_Status_t SMCom<T>::common_write_txbuffer(T * out_packet, const uint8_t * buffer, uint8_t len, uint8_t retry){
	//before calling this function check that txbuffer is not null

	uint16_t crc = CRC_IBM_SEED;
	out_packet->data_len = len;

	//packet data + start byte + end byte + crc (2) = sizeof(packet) + 4
	uint16_t possible_packet_size = sizeof(T) + 4 + len;

	if(packet_size > 0){
		if(packet_size >= possible_packet_size){
			//Tell receiver that this packet has padding bytes
			out_packet->data_len += (packet_size - possible_packet_size);
		}
		else{
			//If user fixes the packet size and try to exceed we won't publish the message
			return SMCOM_STATUS_MESSAGE_LENGTH_ERROR;
		}
	}

	uint16_t txit = 0;
	//Start sequence
	tx_buffer[txit++] = MESSAGE_START;
	txflag.start_byte_flag = 1;

	memcpy(tx_buffer+txit,(uint8_t *)out_packet,sizeof(T));
	txit += sizeof(T);
	txflag.rx_tx_id_flag = 1;

	if(len > 0){
		memcpy(tx_buffer+txit,(uint8_t *)buffer,len);
		txit += len;
	}
	txflag.data_flag = 1;

	if(packet_size > possible_packet_size){
		uint16_t padding = packet_size-possible_packet_size;
		memset(tx_buffer+txit, 0x0, padding);
		txit += padding;
	}

	crc = get_crc_ibm(tx_buffer,txit,crc);
	//CRC sequence
	//In reverse order
	tx_buffer[txit++] = (uint8_t)((crc>>8)&0x00FF);
	tx_buffer[txit++] = (uint8_t)(crc & 0x00FF);
	tx_buffer[txit++] = MESSAGE_END;

	txflag.crc_flag = 1;

	SMCom_Status_t ret = SMCOM_STATUS_DEFAULT;

	ret = __write__retry(tx_buffer,txit,retry);
	
	return ret;
}


template<typename T>
SMCom_Status_t SMCom<T>::common_write_polling(T * out_packet, const uint8_t * buffer, uint8_t len, uint8_t retry){

	SMCom_Status_t ret = SMCOM_STATUS_DEFAULT;
	uint16_t crc = CRC_IBM_SEED;
	out_packet->data_len = len;

	//packet data + start byte + end byte + crc (2) = sizeof(packet) + 4
	uint16_t possible_packet_size = sizeof(T) + 4 + len;
	if(packet_size > 0){
		if(packet_size >= possible_packet_size){
			//Tell receiver that this packet has padding bytes
			out_packet->data_len += (packet_size - possible_packet_size);
		}
		else{
			//If user fixes the packet size and try to exceed we won't publish the message
			return SMCOM_STATUS_MESSAGE_LENGTH_ERROR;
		}
	}

	//Start sequence
	uint8_t head = MESSAGE_START;
	crc = get_crc_ibm(&head,1,crc);
	
	ret = __write__retry(&head,1,retry);
	if(ret != SMCOM_STATUS_SUCCESS){
		return ret;
	}

	txflag.start_byte_flag = 1;

	//Msg sequence
	crc = get_crc_ibm((uint8_t *)out_packet,sizeof(T),crc);
	
	
	ret = __write__retry((uint8_t *)out_packet,sizeof(T),retry);
	if(ret != SMCOM_STATUS_SUCCESS){
		return ret;
	}

	txflag.rx_tx_id_flag = 1;

	if(len > 0){
		//Data sequence
		crc = get_crc_ibm(buffer,len,crc);
		ret = __write__retry(buffer,len,retry);
		if(ret != SMCOM_STATUS_SUCCESS){
			return ret;
		}
	}
	
	if(packet_size > possible_packet_size){
		//Now add padding, user wants to fix message size
		uint16_t padding = (packet_size - possible_packet_size);
		while(padding--){
			uint8_t d = 0;
			ret = __write__retry(&d,1,retry);
			if(ret != SMCOM_STATUS_SUCCESS){
				return ret;
			}
		}
	}
	txflag.data_flag = 1;
	//CRC sequence
	//In reverse order
	uint8_t last_bytes[3] = {(uint8_t)((crc>>8)&0x00FF), (uint8_t)(crc & 0x00FF), MESSAGE_END};

	txflag.crc_flag = 1;
	
	ret = __write__retry(last_bytes,3,retry);
	if(ret != SMCOM_STATUS_SUCCESS){
		return ret;
	}

	return ret;
}


template<typename T>
SMCom_Status_t SMCom<T>::common_write(T * out_packet, const uint8_t * buffer, uint8_t len, uint8_t retry){
	if(rxflag.port_busy_flag){
		//If header is verified and port is busy, this flag is set to 1.
		//We should'not allow to send message while we didn't take the whole message, even though it is not for us. port_busy flag will be cleared after getting end byte
		return SMCOM_STATUS_PORT_BUSY;
	}

	if(buffer == NULL && len != 0){
		return SMCOM_STATUS_FAIL;
	}

	SMCom_Status_t ret = SMCOM_STATUS_DEFAULT;

	clear_tx_flag();

	if(tx_buffer && tx_buf_size){
		ret = common_write_txbuffer(out_packet,buffer,len,retry);
	}
	else{
		ret = common_write_polling(out_packet,buffer,len,retry);
	}
	//if it is a request we won't call general tx handler because each request has its own callback function
	if(tx_event_handler_callback_ptr != NULL && out_packet->message_type != REQUEST)
		tx_event_handler_callback_ptr((SMCom_event_types)out_packet->message_type,ret, out_packet);

	return ret;
}




template<typename T>
SMCom_Status_t SMCom<T>::common_verify_message_header(T * in_packet, const uint8_t * raw_bytes, uint16_t * len, bool copy_buffer){

	if(raw_bytes == NULL) return SMCOM_STATUS_NULL_MESSAGE;

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

	if(copy_buffer){
		//If we call this we will provide rx_buffer already copied the data so no need to copy it again
		memcpy(rx_buffer,raw_bytes,rx_iter);
	}

	rxflag.start_byte_flag = 1;

	//Mesajın bitmesi 2.byte ile belirleniyor ve ek olarak 2 byte crc ile 1 byte end byte mesajı da alınmalı
	//Len artık bir sonraki mesajın boyunu söylüyor
	*len = rx_buffer[1] + 3;
	in_packet->data_len = *len;

	//maybe this message is not for us, maybe port is busy
	if(additional_buffer_check(in_packet) != SMCOM_STATUS_SUCCESS){
		rxflag.rx_tx_id_flag = 0;
		rxflag.port_busy_flag = 1;
		
		return SMCOM_STATUS_PORT_BUSY;
	}
	//Set the flag we are responsible of this message!
	rxflag.rx_tx_id_flag = 1;

	return SMCOM_STATUS_SUCCESS;
}


template<typename T>
SMCom_Status_t SMCom<T>::common_handle_message_data(const uint8_t * raw_bytes, uint16_t len, bool copy_buffer){

	SMCom_Status_t ret = SMCOM_STATUS_DEFAULT;
	if(raw_bytes == NULL) return SMCOM_STATUS_NULL_MESSAGE;

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

	if(copy_buffer){
		memcpy(rx_buffer+rx_iter,raw_bytes,len-3);
	}

	rx_iter += (len-3);

	//Skip the start byte and cast it to our message packet
	T * packet = (T *) (rx_buffer+1);
	SMCom_event_types event = (SMCom_event_types) packet->message_type;

	rxflag.data_flag = 1;

	// #ifdef SMCOM_CONFIG_REQUEST_RESPONSE
	// if(evt == SM_RESPONSE_EVENT || evt == SM_WRITE_EVENT){
	// 	//Check that is this a response from a registered request before ?
	// 	request_list_iterator prev = check_incoming_response(packet);
	// 	if(prev != request_list.end()){
	// 		request_list_iterator it = std::next(prev);
	// 		if(it->fptr != NULL){
	// 			it->fptr(ret,packet);
	// 			//set packet to null so we are not gonna call rxevent handler since we have already invoked this function
	// 			packet = NULL;
	// 		}
	// 		//Remove the request
	// 		request_list.erase_after(prev);
	// 	}
	// }
	// #endif

	bool notify_rx_callback = true;

	if(packet != NULL && packet->message_type == SMCom_message_types::REQUEST_ACK){
		smcom_log("Got write ack event, notify we got the data\n");
		notify_rx_callback = false;
		response_ack(packet,NULL,0); //Returning response to request
	}

	if(packet != NULL && packet->message_type == SMCom_message_types::RESPONSE_ACK){
		notify_rx_callback = false;
		//Leave it as it is, write_ack will handle it
	}

		//Rx buffer contains the data, responsible ack handler will use it!
	// if(packet != NULL && event == SM_REQUEST_EVENT && packet->message_id >= SMCOM_SPECIAL_MESSAGE_START_ID){
	// 	if(ret != SMCOM_STATUS_CRC_ERROR){
	// 		respond_smcom_special_messages(packet);
	// 		//Quietly return default like we had no messages
	// 		ret = SMCOM_STATUS_DEFAULT;
	// 		//set packet to null so we are not gonna call rxevent handler since we have already invoked this function
	// 	}
	// 	notify_rx_callback = false;
	// }

	if(notify_rx_callback){
		if(rx_event_handler_callback_ptr == NULL){
			__rx_callback__(event,ret,packet);
		}
		else{
			rx_event_handler_callback_ptr(event,ret,packet);
		}
	}

	clear_rx_flag();
	return ret;
}

template<typename T>
SMCom_Status_t SMCom<T>::common_respond_smcom_special_messages(T * packet){

	switch(packet->message_id){
		case SMCOM_MSG_GET_VERSION__:{
			smcom_message_get_version_struct__ vs;
			vs.version = SMCOM_VERSION;
			return common_write((uint8_t*)&vs,sizeof(smcom_message_get_version_struct__));
		}
	}

	return common_write(NULL,0);
}



template<typename T>
void SMCom<T>::__rx_callback__(SMCom_event_types event, SMCom_Status_t status, const T * packet){
	;
}


template<typename T>
void SMCom<T>::__tx_callback__(SMCom_event_types event, SMCom_Status_t status, const T * packet){
	;
}

template<typename T>
SMCom_Status_t SMCom<T>::__read__(uint8_t * buffer, uint16_t len){
	return SMCOM_STATUS_FAIL;
}

template<typename T>
size_t SMCom<T>::__available__(){
	return 0;
}

template<typename T>
SMCom_Status_t SMCom<T>::listener(void){
	uint16_t len = HEADER_SIZE;
	
	T in_packet = const_com_packet;

	SMCom_Status_t status = SMCOM_STATUS_DEFAULT;
	size_t avlb = __available__();
	if(avlb >= HEADER_SIZE){
		uint8_t * ptr = rx_buffer;
		while(__available__() > 0 && __read__(ptr,1) == SMCOM_STATUS_SUCCESS){
			if(*ptr == MESSAGE_START){
				++ptr;
				break;
			}
		}
		status = __read__(ptr,HEADER_SIZE-1);
		ptr--;
		if(status == SMCOM_STATUS_SUCCESS){
			status = common_verify_message_header(&in_packet,ptr, &len,false);
		}
		if(is_packet_broken(status)) return status;
		ptr += HEADER_SIZE;
		status = __read__(ptr,len);
		if(status == SMCOM_STATUS_SUCCESS){
			status = common_handle_message_data(ptr,len,false);
		}
		else{
			clear_rx_flag();
		}
	}
	return status;
}

template<typename T>
SMCom_Status_t SMCom<T>::__write__retry(const uint8_t * buffer, uint8_t len, uint8_t retry){
	SMCom_Status_t ret = SMCOM_STATUS_DEFAULT;
	for(uint8_t i = 0; i<retry; ++i){
		if( (ret = __write__(buffer,len)) != SMCOM_STATUS_SUCCESS){
			return ret;
		}
	}
	return ret;
}


template<typename T>
SMCom_Status_t SMCom<T>::handle_message_data(const uint8_t * raw_bytes, uint16_t len){
		return common_handle_message_data(raw_bytes,len,true);
}
template<typename T>
SMCom_Status_t SMCom<T>::verify_message_header(const uint8_t * raw_bytes, uint16_t * len){
	return common_verify_message_header(raw_bytes,len,true);
}
template<typename T>
SMCom_Status_t SMCom<T>::push_to_queue(const uint8_t * buffer, uint8_t len,uint8_t retry){
	return common_push_to_queue(buffer,len,retry);
}
template<typename T>
SMCom_Status_t SMCom<T>::finalize_queue(uint8_t retry){
	return common_finalize_queue(retry);
}


template<typename T>
void SMCom<T>::set_fixed_packet_size(uint16_t packet_size){
	this->packet_size = packet_size;
}

template<typename CT>
CT * SMCom<CT>::duplicate_message_packet(const CT * packet){
	uint16_t sz = sizeof(CT) + packet->data_len;
	CT * p = (CT *) malloc(sz);
	if(p != NULL){
		memcpy(p,packet,sz);
	}
	return p;
}

template<typename T>
uint8_t SMCom<T>::get_packet_data_length(const T * packet){
	return packet->data_len;
}


template<typename T>
void SMCom<T>::clear_tx_flag(){
	memset(&txflag,0,sizeof(message_flags));
}

template<typename T>
void SMCom<T>::clear_rx_flag(){
	memset(&rxflag,0,sizeof(message_flags));
}

template<typename T>
uint64_t SMCom<T>::GET_SMCOM_VERSION(){
	return SMCOM_VERSION;
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

// ########################## Version 2 ##############################

template<typename T>
SMCom_Status_t SMCom<T>::__lock__(uint32_t timeout){
	return SMCOM_STATUS_SUCCESS;
}

template<typename T>
void SMCom<T>::__unlock__(){
	;
}
