#ifndef _SMCOM_H
#define _SMCOM_H

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>


#ifdef SMCOM_CONFIG_REQUEST_RESPONSE

#include <ctime>
#include <forward_list>
#include <iterator>

#endif

/*
	Some configurations macros before including this class
	-SMCOM BUFFER MEMORY
		SMCOM_CONFIG_STATIC_MEMORY : smcom statically defines its memory, does not use "new"
			expects SMCOM_CONFIG_STATIC_RX_SIZE and SMCOM_CONFIG_STATIC_TX_SIZE as macros
			default values are 1024 and 0

		SMCOM_CONFIG_DYNAMIC_MEMORY : smcom dynamically defines its memory, uses "new"
		default : SMCOM_CONFIG_STATIC_MEMORY
		Most of the time communication will be used and we do not need to define memory dynamically
	
	-SMCOM REQUEST-RESPONSE FLAG
		SMCOM_CONFIG_REQUEST_RESPONSE
		SMCom holds requests inside a linkedlist.
		There are two function which MUST be called increase_ms_timer and run_request_scheduler to check timedout requests
		This will include c++ forward_list and iterator libraries and try to check every message if there is a response
		If macro is not defined no need to include above libraries, SMCom can only be used to write messages

		default: is undefined macro


*/


#define SMCOM_CONFIG_STATIC_MEMORY

#ifndef SMCOM_RX_BUFFER_SIZE
#define SMCOM_RX_BUFFER_SIZE 1024
#endif

// static uint8_t __rx_buffer[SMCOM_RX_BUFFER_SIZE];

//General Message type
// +---------+-----------+---------------+------------+----------+----+---------+
// |         |           |               |            |          |    |         |
// |         |           |               |            |          |    |         |
// |START BYT|DATA LENGTH|  RECEIVER ID  |MESSAGE TYPE+<-+DATA+->+CRC | END BYTE|
// |         |           | TRANSMITTER ID|            |          |    |         |
// |    1    |     1     |    1,2,4      |      1     |          | 2  |    1    |
// +---------+-----------+---------------+------------+----------+----+---------+


//like uart, rx-tx are connected to only two devices. Buffer does not contain receiver id or transmitter id
struct smcom_private_t{
	uint8_t data_len;
	uint8_t message_type:2;
	uint8_t message_id:6;
	uint8_t data[0];
}__attribute__((packed));
typedef struct smcom_private_t SMCOM_PRIVATE;


//like rs485, rx-tx are connected to multiple devices. Buffer contains rx id, tx id 4bits which can only support 15 devices, 0 for public
struct smcom_public_4bit_adr{
	uint8_t data_len;
	uint8_t receiver_id:4;
	uint8_t transmitter_id:4;
	uint8_t message_type:2;
	uint8_t message_id:6;
	uint8_t data[0];
}__attribute__((packed));
typedef struct smcom_public_4bit_adr SMCOM_PUBLIC;

//like rs485, rx-tx are connected to multiple devices. Buffer contains rx id, tx id 8bits, which can only support 255 devices, 255 for public, 254 is default id
struct smcom_public_8bit_adr{
	uint8_t data_len;
	uint8_t receiver_id;
	uint8_t transmitter_id;
	uint8_t message_type:2;
	uint8_t message_id:6;
	uint8_t data[0];
}__attribute__((packed));
typedef struct smcom_public_8bit_adr SMCOM_PUBLIC_8BIT_ADDRESS;

#ifdef SMCOM_CONFIG_CUSTOM_ADDRESS
//user can define any address byte
struct smcom_public_custom_adr{
	uint8_t data_len;
	uint8_t receiver_id[SMCOM_CUSTOM_ADDRESS];
	uint8_t transmitter_id;
	uint8_t message_type:2;
	uint8_t message_id:6;
	uint8_t data[0];
}__attribute__((packed));
typedef struct smcom_public_custom_adr SMCOM_PUBLIC_CUSTOM_ADDRESS;
#endif

//similar to spi, slave cannot send a message. Only master may request a data
struct smcom_only_master{
	uint8_t data_len;
	uint8_t receiver_id;
	uint8_t message_type:2;
	uint8_t message_id:6;
	uint8_t data[0];
}__attribute__((packed));
typedef struct smcom_only_master SMCOM_ONLY_MASTER;


//broken packet errors must be bigger than SMCOM_STATUS_FAIL, before just notification to user
enum SMCom_Status_t : uint8_t{
	SMCOM_STATUS_DEFAULT = 0,
	SMCOM_STATUS_SUCCESS,
	SMCOM_STATUS_PORT_BUSY,
	SMCOM_STATUS_TIMEOUT,
	SMCOM_STATUS_FAIL,
	SMCOM_STATUS_RX_ERROR,
	SMCOM_STATUS_CRC_ERROR,
	SMCOM_STATUS_START_BYTE_ERROR,
	SMCOM_STATUS_END_BYTE_ERROR,
	SMCOM_STATUS_HEADER_LENGTH_ERROR,
	SMCOM_STATUS_MESSAGE_LENGTH_ERROR,
	SMCOM_STATUS_NULL_MESSAGE,
};


typedef struct message_flags{
	uint8_t start_byte_flag:1;
	uint8_t rx_tx_id_flag:1;
	uint8_t data_flag:1;
	uint8_t crc_flag:1;
	uint8_t end_byte_flag:1;
	uint8_t port_busy_flag:1;
} message_flags;


enum SMCom_event_types : uint8_t{
	SM_WRITE_EVENT = 0,
	SM_REQUEST_EVENT,
	SM_RESPONSE_EVENT,
	SM_INDICATE_EVENT
};

enum SMCom_message_types : uint8_t{
	WRITE = 0,
	REQUEST = 1,
	RESPONSE = 2,
	INDICATE = 3,
};

//predefined char values
enum SMCom_headers: uint8_t {
	MESSAGE_START = 0xFB,
	MESSAGE_END = 0xBF,
	PUBLIC_ID_8BIT = 255,
	DEFAULT_ID_8BIT = 254,
	PUBLIC_ID_4BIT = 15,
	DEFAULT_ID_4BIT = 14,
};



template <typename CT>
class SMCom{
public:
	typedef void (*rx_event_handler_callback)(SMCom_event_types event, SMCom_Status_t status, const CT * packet);
	rx_event_handler_callback rx_event_handler_callback_ptr = NULL;

	typedef void (*tx_event_handler_callback)(SMCom_event_types event, SMCom_Status_t status, const CT * packet);
	tx_event_handler_callback tx_event_handler_callback_ptr = NULL;

	typedef void(*request_response_callback)(SMCom_Status_t status, const CT * packet);
	
	SMCom(uint16_t rx_buf_size, rx_event_handler_callback rx, tx_event_handler_callback tx);
	SMCom(uint16_t _rx_buf_size, uint8_t id, rx_event_handler_callback rx, tx_event_handler_callback tx);
	~SMCom();

	SMCom_Status_t verify_message_header(const uint8_t * raw_bytes, uint16_t * len);
	SMCom_Status_t handle_message_data(const uint8_t * raw_bytes, uint16_t len);

	SMCom_Status_t write(SMCom_message_types t, uint8_t message_id, const uint8_t * buffer, uint8_t len);
	SMCom_Status_t write(SMCom_message_types t, uint8_t receiver_id, uint8_t message_id, const uint8_t * buffer, uint8_t len);

	#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
	SMCom_Status_t request(uint8_t message_id, const uint8_t * buffer, uint8_t len, uint32_t timeout, request_response_callback fptr = NULL);
	SMCom_Status_t request(uint8_t receiver_id, uint8_t message_id, const uint8_t * buffer, uint8_t len, uint32_t timeout, request_response_callback fptr = NULL);
	SMCom_Status_t respond(uint8_t message_id, const uint8_t * buffer, uint8_t len);
	SMCom_Status_t respond(uint8_t receiver_id, uint8_t message_id, const uint8_t * buffer, uint8_t len);
	SMCom_Status_t respond(const CT * inc_packet, const uint8_t * buffer, uint8_t len);
	#endif


	SMCom_Status_t start_write_queue(SMCom_message_types t, uint8_t receiver_id,uint8_t message_id, uint8_t len);
	SMCom_Status_t start_write_queue(SMCom_message_types t, uint8_t message_id, uint8_t len);

	SMCom_Status_t push_to_queue(const uint8_t * buffer, uint8_t len);
	SMCom_Status_t finalize_queue();

	uint8_t get_packet_data_length(const CT * packet);
	

	CT * duplicate_message_packet(const CT * packet);

	virtual SMCom_Status_t __write__(const uint8_t * buffer, uint8_t len) = 0;

	bool is_packet_broken(SMCom_Status_t stat){
		return stat >= SMCOM_STATUS_FAIL;
	}
	bool is_crc_failed(SMCom_Status_t stat){
		return stat == SMCOM_STATUS_CRC_ERROR;
	}


	#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
	void increase_ms_timer();
	void run_request_scheduler();
	#endif

	uint16_t get_rx_buffer_size(){return rx_buf_size;};
	void assign_new_id(uint8_t id);

	

	static const char * resolve_status(SMCom_Status_t t){
		static const char * str_SMCom_Status_t[] = {
			"SMCOM_DEFAULT",
			"SMCOM_SUCCESS",
			"SMCOM_PORT_BUSY",
			"SMCOM_TIMEOUT",
			"SMCOM_FAIL",
			"SMCOM_RX_ERROR",
			"SMCOM_CRC_ERROR",
			"SMCOM_START_BYTE_ERROR",
			"SMCOM_END_BYTE_ERROR",
			"SMCOM_HEADER_LENGTH_ERROR",
			"SMCOM_MESSAGE_LENGTH_ERROR",
			"SMCOM_NULL_MESSAGE",
		};
		return str_SMCom_Status_t[t];
	}

	static const uint8_t HEADER_SIZE = sizeof(CT)+1;
	static const uint8_t MAX_MSG_LENGTH = 0xFF;

private:


	SMCom_Status_t common_write(const uint8_t * buffer, uint8_t len);

	#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
	SMCom_Status_t common_request(const uint8_t * buffer, uint8_t len, uint32_t timeout, request_response_callback fptr);
	#endif
	
	SMCom_Status_t common_read_internal(const uint8_t * raw_bytes, uint8_t len,uint8_t end_byte_start_offset);
	SMCom_Status_t additional_buffer_check();
	
	SMCom_Status_t common_verify_message_header(const uint8_t * raw_bytes, uint16_t * len);
	SMCom_Status_t common_handle_message_data(const uint8_t * raw_bytes, uint16_t len);

	SMCom_Status_t common_start_write_queue();
	SMCom_Status_t common_push_to_queue(const uint8_t * buffer, uint8_t len);	
	SMCom_Status_t common_finalize_queue();


	#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
	//@TODO
	//Maybe we can add another request packet without fptr????
	typedef struct request_packet{
		CT packet;
		uint32_t timeout_end;
		request_response_callback fptr;
	}request_packet;
	
	std::forward_list<request_packet> request_list;
	typedef typename std::forward_list<request_packet>::iterator request_list_iterator; 

	bool is_request_registered_before();
	bool remove_request_from_list(CT * packet);
	request_list_iterator get_request(CT * packet);
	request_list_iterator get_timedout_request();
	request_list_iterator check_incoming_response(CT * inc_packet);
	SMCom_Status_t register_request(uint32_t timeout, request_response_callback fptr);
	#endif

	void clear_tx_flag();
	void clear_rx_flag();


	CT com_packet;

	uint8_t * rx_buffer = NULL;
	uint16_t rx_iter = 0;
	uint16_t rx_buf_size = 0;

	uint16_t message_end_index;

	message_flags rxflag, txflag;

	uint16_t last_crc;

	//Timeout counter counts miliseconds
	uint32_t timeout_counter = 0;

	//Polynomial x^16 + x^12 + x^5 + 1
	#define POLY_CCITT 0x1021 //'0b1000000100001'
	//Polynomial x^16 + x^15 + x^2 + 1
	#define POLY_IBM 0x8005 //'0b1000000000000101'

	#define CRC_IBM_SEED 0xffff
	#define CRC_CCITT_SEED 0x1d0f

	uint16_t compute_crc_ibm(uint16_t crc, uint8_t data);
	uint16_t get_crc_ibm(const uint8_t * buffer, uint8_t len,uint16_t crc = CRC_IBM_SEED);
};


#endif