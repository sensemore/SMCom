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
}


template<>
SMCom_Status_t SMCom<SMCOM_PRIVATE>::write(uint8_t message_id, const uint8_t * buffer, uint8_t len, uint8_t retry){
	if(message_id > SMCOM_MAX_USER_MESSAGE_ID) return SMCOM_STATUS_MESSAGE_ID_ERROR;

	SMCOM_PRIVATE out_packet;
	out_packet.message_type = SMCom_message_types::WRITE;
	out_packet.message_id = message_id;
	return common_write(&out_packet,buffer,len,retry);
}

template<>
SMCom_Status_t SMCom<SMCOM_PRIVATE>::start_write_queue(SMCom_message_types t, uint8_t message_id, uint8_t len, uint8_t retry){
	if(message_id > SMCOM_MAX_USER_MESSAGE_ID) return SMCOM_STATUS_MESSAGE_ID_ERROR;
	SMCOM_PRIVATE out_packet;
	out_packet.message_type = t;
	out_packet.message_id = message_id;
	out_packet.data_len = len;
	return common_start_write_queue(&out_packet,retry);
}


// template<>
// SMCom_Status_t SMCom<SMCOM_PRIVATE>::respond(uint8_t message_id, const uint8_t * buffer, uint8_t len, uint8_t retry){
// 	if(message_id > SMCOM_MAX_USER_MESSAGE_ID) return SMCOM_STATUS_MESSAGE_ID_ERROR;

// 	com_packet.message_type = SMCom_message_types::RESPONSE;
// 	com_packet.message_id = message_id;
// 	return common_write(buffer,len,retry);
// }

// template<>
// SMCom_Status_t SMCom<SMCOM_PRIVATE>::respond(const SMCOM_PRIVATE * inc_packet, const uint8_t * buffer, uint8_t len,uint8_t retry){
// 	if(inc_packet->message_id > SMCOM_MAX_USER_MESSAGE_ID) return SMCOM_STATUS_MESSAGE_ID_ERROR;
// 	com_packet.message_type = SMCom_message_types::RESPONSE;
// 	com_packet.message_id = inc_packet->message_id;
// 	return common_write(buffer,len,retry);
// }

// template<>
// SMCom_Status_t SMCom<SMCOM_PRIVATE>::respond_smcom_special_messages(SMCOM_PRIVATE * packet){

// 	com_packet.message_type = SMCom_message_types::RESPONSE;
// 	com_packet.message_id = packet->message_id;
// 	return common_respond_smcom_special_messages(packet);
// }



template<>
SMCom_Status_t SMCom<SMCOM_PRIVATE>::additional_buffer_check(SMCOM_PRIVATE * in_packet){
	//Nothing to check, there is no receiver/transmitter id in this type of a communicaiton
	return SMCOM_STATUS_SUCCESS;
}



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
	const_com_packet.transmitter_id = id;

	clear_rx_flag();
	clear_tx_flag();
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
	const_com_packet.transmitter_id = id;

	conflag.static_buffer_provided = true;

	clear_rx_flag();
	clear_tx_flag();
}

template<>
void SMCom<SMCOM_PUBLIC>::assign_new_id(uint8_t id){
	const_com_packet.transmitter_id = id;
}


template<>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::write(uint8_t receiver_id,uint8_t message_id, const uint8_t * buffer, uint8_t len,uint8_t retry){

	if(message_id > SMCOM_MAX_USER_MESSAGE_ID) return SMCOM_STATUS_MESSAGE_ID_ERROR;

	SMCOM_PUBLIC out_packet = const_com_packet;
	out_packet.message_type = SMCom_message_types::WRITE;
	out_packet.message_id = message_id;
	out_packet.receiver_id = receiver_id;
	return common_write(&out_packet,buffer,len,retry);
}

template<>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::write_ack(uint8_t receiver_id,uint8_t message_id, const uint8_t * buffer, uint8_t len,uint16_t timeout,uint8_t retry){

	if(message_id > SMCOM_MAX_USER_MESSAGE_ID) return SMCOM_STATUS_MESSAGE_ID_ERROR;

	SMCOM_PUBLIC out_packet = const_com_packet;

	out_packet.message_type = SMCom_message_types::REQUEST_ACK;
	out_packet.message_id = message_id;
	out_packet.receiver_id = receiver_id;

	SMCom_Status_t ret = SMCOM_STATUS_DEFAULT;
	uint8_t internal_retry = retry;

	while(internal_retry > 0){
		
		//Got the mutex, now we can call listener and write
		if( (ret = common_write(&out_packet,buffer,len,retry)) != SMCOM_STATUS_SUCCESS){
			return ret;
		}

		SMCom_Status_t lock_ret = __lock__(timeout);
		if(lock_ret != SMCOM_STATUS_SUCCESS){
			return SMCOM_STATUS_MUTEX_TIMEOUT;
		}

		ret = listener();
		if(ret != SMCOM_STATUS_SUCCESS){
			__unlock__();
			return ret;
		}
		
		//Rx buffer contains the required fields
		SMCOM_PUBLIC * inc_ack_packet = (SMCOM_PUBLIC *)(rx_buffer+1);
		smcom_log("incoming packet ");

		//Now have the mutex

		__unlock__();
		internal_retry--;
	}
	
	ret = SMCOM_STATUS_FAIL;

	return ret;
}


template<>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::response_ack(const SMCOM_PUBLIC * inc_packet, const uint8_t * buffer, uint8_t len, uint8_t retry){

	if(inc_packet->message_id > SMCOM_MAX_USER_MESSAGE_ID) return SMCOM_STATUS_MESSAGE_ID_ERROR;
	
	SMCOM_PUBLIC out_packet = const_com_packet;
	out_packet.message_type = SMCom_message_types::RESPONSE_ACK;
	out_packet.message_id = inc_packet->message_id;
	out_packet.receiver_id = inc_packet->transmitter_id;
	return common_write(&out_packet,buffer,len,retry);
}


template<>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::write_public(uint8_t message_id, const uint8_t * buffer, uint8_t len,uint8_t retry){
	return write(PUBLIC_ID_4BIT,message_id, buffer, len, retry);
}


template<>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::start_write_queue(SMCom_message_types t, uint8_t receiver_id, uint8_t message_id, uint8_t len, uint8_t retry){

	if(message_id > SMCOM_MAX_USER_MESSAGE_ID) return SMCOM_STATUS_MESSAGE_ID_ERROR;

	SMCOM_PUBLIC out_packet = const_com_packet;

	out_packet.message_type = t;
	out_packet.message_id = message_id;
	out_packet.receiver_id = receiver_id;
	out_packet.data_len = len;
	return common_start_write_queue(&out_packet,retry);
}


template<>
SMCom_Status_t SMCom<SMCOM_PUBLIC>::additional_buffer_check(SMCOM_PUBLIC * in_packet){
	//Check that is this message for us, we are just gonna check receiver id, which must be our ID

	SMCOM_PUBLIC * ptr = (SMCOM_PUBLIC *) (rx_buffer+1);

	if(ptr->receiver_id == in_packet->transmitter_id || ptr->receiver_id == SMCom_headers::PUBLIC_ID_4BIT){
		return SMCOM_STATUS_SUCCESS;
	}

	return SMCOM_STATUS_PORT_BUSY;
}


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