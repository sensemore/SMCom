#ifndef SMCOM_H
#define SMCOM_H

#ifdef SMCOM_DEBUG
#define smcom_log(s,...) (printf("[%d-%s]" s "\n",__LINE__,__func__,##__VA_ARGS__))
#else
#define smcom_log(s,...)
#endif


#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>


#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#define PACKET_DATA_LEN (0)
#define SIZEOF_PACKET(T) (sizeof(T))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#define PACKET_DATA_LEN (1)
#define SIZEOF_PACKET(T) (sizeof(T)-1)
#endif

//#ifndef SMCOM_CONFIG_DISABLE_REQUEST_RESPONSE
//#define SMCOM_CONFIG_REQUEST_RESPONSE
//#endif

#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
#include <ctime>
#include <forward_list>
#include <iterator>
#endif


//  SMCOM_VERSION % 100 : patch level
//  SMCOM_VERSION / 100 % 1000 : minor version
//  SMCOM_VERSION / 100000 : major version
#define SMCOM_MAJOR_VERSION__ 	1
#define SMCOM_MINOR_VERSION__ 	0
#define SMCOM_PATCH_LEVEL__ 	3
#define SMCOM_VERSION_STRING	"1.0.3"
#define SMCOM_VERSION ((SMCOM_MAJOR_VERSION__ * 100000) + (SMCOM_MINOR_VERSION__ * 100) + SMCOM_PATCH_LEVEL__)
#define MAX_RETRY_FOR_WRITE 5
/*
	Some configurations macros before including this class
	-SMCOM REQUEST-RESPONSE FLAG
		SMCOM_CONFIG_REQUEST_RESPONSE
		SMCom holds requests inside a linkedlist.
		There are two function which MUST be called increase_ms_timer and run_request_scheduler to check timedout requests
		This will include c++ forward_list and iterator libraries and try to check every message if there is a response
		If macro is not defined no need to include above libraries, SMCom can only be used to write messages

		default: is undefined macro
*/

//General Message type
// +---------+-----------+---------------+------------+----------+----+---------+
// |         |           |               |            |          |    |         |
// |         |           |               |            |          |    |         |
// |START BYT|DATA LENGTH|  RECEIVER ID  |MESSAGE TYPE+<-+DATA+->+CRC | END BYTE|
// |         |           | TRANSMITTER ID|            |          |    |         |
// |    1    |     1     |    1,2,4      |      1     |          | 2  |    1    |
// +---------+-----------+---------------+------------+----------+----+---------+

/*
Bit field ordering is the following : first LOW then HIGH part ie little endian
For example
struct some_struct{
	uint8_t a:2;	0b000000xx
	uint8_t b:2;	0b0000xx00
	uint8_t c:3;	0b0xxx0000
	uint8_t d:3;	0bx0000000
}__attribute__((packed));

*/

//like uart, rx-tx are connected to only two devices. Buffer does not contain receiver id or transmitter id
// adds 6-bytes to message packet including start byte, end byte crc etc.
PACK(struct smcom_private_t{
	uint8_t data_len;			//<! Data len in the packet, user can use this instead defining data length in the message max data length is 255
	uint8_t message_type:2;		//<! Message type , can be write/request/response/indicate etc. User does not change this max value is 3
	uint8_t message_id:6;		//<! Message id is defined by the user similar to uuid in BLE communication. Max value is 63
	uint8_t data[PACKET_DATA_LEN];			//<! Data pointer for derived classes
	
});
typedef struct smcom_private_t SMCOM_PRIVATE;


//like rs485, rx-tx are connected to multiple devices. Buffer contains rx id, tx id 4bits which can only support 15 devices, 0 for public
// adds 7-bytes to message packet including start byte, end byte crc etc.
PACK(struct smcom_public_4bit_adr{
	uint8_t data_len;			//<! Data len in the packet, user can use this instead defining data length in the message max data length is 255
	uint8_t receiver_id:4;		//<! Receiver id in the communication can take value between 0-13 (14 and 15 is reserved), located at 0b0000xxxx
	uint8_t transmitter_id:4;	//<! Transmitter id in the communication can take value between 0-13 (14 and 15 is reserved), located at 0bxxxx0000
	uint8_t message_type:2;		//<! Message type , can be write/request/response/indicate etc. User does not change this max value is 3, located at 0b000000xx
	uint8_t message_id:6;		//<! Message id is defined by the user similar to uuid in BLE communication. Max value is 63, located at 0bxxxxxx00
	uint8_t data[PACKET_DATA_LEN];			//<! Data pointer for derived classes
});
typedef struct smcom_public_4bit_adr SMCOM_PUBLIC;


//like rs485, rx-tx are connected to multiple devices. Buffer contains rx id, tx id 8bits, which can only support 255 devices, 255 for public, 254 is default id
// adds 8-bytes to message packet including start byte, end byte crc etc.
PACK(struct smcom_public_8bit_adr{
	uint8_t data_len;			//<! Data len in the packet, user can use this instead defining data length in the message max data length is 255
	uint8_t receiver_id;		//<! Receiver id in the communication can take value between 0-253 (254 and 255 is reserved)
	uint8_t transmitter_id;		//<! Transmitter id in the communication can take value between 0-253 (254 and 255 is reserved)
	uint8_t message_type:2;		//<! Message type , can be write/request/response/indicate etc. User does not change this max value is 3, located at 0b000000xx
	uint8_t message_id:6;		//<! Message id is defined by the user similar to uuid in BLE communication. Max value is 63, located at 0bxxxxxx00
	uint8_t data[PACKET_DATA_LEN];			//<! Data pointer for derived classes
});
typedef struct smcom_public_8bit_adr SMCOM_PUBLIC_8BIT_ADDRESS;

#ifdef SMCOM_CONFIG_CUSTOM_ADDRESS
//user can define any address byte
// adds 6-bytes + 2*custom_address_size to message packet including start byte, end byte crc etc.
PACK(struct smcom_public_custom_adr{
	uint8_t data_len;
	uint8_t receiver_id[SMCOM_CUSTOM_ADDRESS];
	uint8_t transmitter_id[SMCOM_CUSTOM_ADDRESS];
	uint8_t message_type:2;
	uint8_t message_id:6;
	uint8_t data[PACKET_DATA_LEN];
});
typedef struct smcom_public_custom_adr SMCOM_PUBLIC_CUSTOM_ADDRESS;
#endif

//Not implemented yet!
//similar to spi, slave cannot send a message. Only master may request a data
// struct smcom_only_master{
// 	uint8_t data_len;
// 	uint8_t receiver_id;		
// 	uint8_t message_type:2;
// 	uint8_t message_id:6;
// 	uint8_t data[0];
// }__attribute__((packed));
// typedef struct smcom_only_master SMCOM_ONLY_MASTER;


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
	SMCOM_STATUS_MESSAGE_ID_ERROR,
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


typedef struct configuration_flags{
	uint8_t static_buffer_provided:1;
} configuration_flags;


enum SMCom_event_types : uint8_t{
	SM_WRITE_EVENT = 0,
	SM_REQUEST_EVENT = 1,
	SM_RESPONSE_EVENT = 2,
	SM_INDICATE_EVENT = 3,
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

//Our message id values is represented by 6bits
//Which starts from 0 to 2**6-1 = 63, so we will use the last values
//Even though we have nothing, maybe next versions will use the following structures
#define SMCOM_SPECIAL_MESSAGE_START_ID 60
#define SMCOM_MAX_USER_MESSAGE_ID (SMCOM_SPECIAL_MESSAGE_START_ID - 1)
enum SMCom_special_messages : uint8_t{
	SMCOM_MSG_PING__ = 62,
	SMCOM_MSG_GET_VERSION__ = 63,
};


PACK(struct smcom_message_get_version_struct__{
	uint32_t version;
});


typedef smcom_message_get_version_struct__ smcom_message_get_version_struct__;



template <typename CT>
class SMCom{
public:
	typedef void (*rx_event_handler_callback)(SMCom_event_types event, SMCom_Status_t status, const CT * packet);
	rx_event_handler_callback rx_event_handler_callback_ptr = NULL;

	typedef void (*tx_event_handler_callback)(SMCom_event_types event, SMCom_Status_t status, const CT * packet);
	tx_event_handler_callback tx_event_handler_callback_ptr = NULL;

	virtual void __rx_callback__(SMCom_event_types event, SMCom_Status_t status, const CT * packet);
	virtual void __tx_callback__(SMCom_event_types event, SMCom_Status_t status, const CT * packet);


	#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
	typedef void(*request_response_callback)(SMCom_Status_t status, const CT * packet);
	#endif
	
	////Constructors with dynamic buffers, uses C++ new allocater
	SMCom(uint16_t rx_buf_size, uint16_t tx_buf_size, rx_event_handler_callback rx, tx_event_handler_callback tx);
	SMCom(uint16_t rx_buf_size, uint16_t tx_buf_size, uint8_t id, rx_event_handler_callback rx, tx_event_handler_callback tx);
	//Constructors with static buffers
	SMCom(uint8_t * rx_buffer, uint16_t rx_buf_size, uint8_t * tx_buffer, uint16_t tx_buf_size, rx_event_handler_callback rx, tx_event_handler_callback tx);
	SMCom(uint8_t * rx_buffer, uint16_t rx_buf_size, uint8_t * tx_buffer, uint16_t tx_buf_size, uint8_t id, rx_event_handler_callback rx, tx_event_handler_callback tx);
	SMCom(rx_event_handler_callback rx=NULL, tx_event_handler_callback tx=NULL);
	~SMCom();

	SMCom_Status_t verify_message_header(const uint8_t * raw_bytes, uint16_t * len);
	SMCom_Status_t handle_message_data(const uint8_t * raw_bytes, uint16_t len);

	SMCom_Status_t write(uint8_t message_id, const uint8_t * buffer, uint8_t len, uint8_t retry = 1);
	SMCom_Status_t write(uint8_t receiver_id, uint8_t message_id, const uint8_t * buffer, uint8_t len, uint8_t retry = 1);

	SMCom_Status_t write_public(uint8_t message_id, const uint8_t * buffer, uint8_t len, uint8_t retry = 1);

	#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
	SMCom_Status_t request(uint8_t message_id, const uint8_t * buffer, uint8_t len, uint32_t timeout, request_response_callback fptr = NULL);
	SMCom_Status_t request(uint8_t receiver_id, uint8_t message_id, const uint8_t * buffer, uint8_t len, uint32_t timeout, request_response_callback fptr = NULL);

	SMCom_Status_t ping(uint8_t receiver_id, uint32_t timeout, request_response_callback fptr);
	SMCom_Status_t ping(uint32_t timeout, request_response_callback fptr);

	SMCom_Status_t get_version(uint8_t receiver_id, uint32_t timeout, request_response_callback fptr);
	SMCom_Status_t get_version(uint32_t timeout, request_response_callback fptr);
	#endif
	SMCom_Status_t respond(uint8_t message_id, const uint8_t * buffer, uint8_t len, uint8_t retry = 1);
	SMCom_Status_t respond(uint8_t receiver_id, uint8_t message_id, const uint8_t * buffer, uint8_t len, uint8_t retry = 1);
	SMCom_Status_t respond(const CT * inc_packet, const uint8_t * buffer, uint8_t len, uint8_t retry = 1);
	


	SMCom_Status_t start_write_queue(SMCom_message_types t, uint8_t receiver_id,uint8_t message_id, uint8_t len, uint8_t retry = 1);
	SMCom_Status_t start_write_queue(SMCom_message_types t, uint8_t message_id, uint8_t len, uint8_t retry = 1);
	SMCom_Status_t push_to_queue(const uint8_t * buffer, uint8_t len, uint8_t retry = 1);
	SMCom_Status_t finalize_queue(uint8_t retry = 1);

	//If given message length is smaller than the packet size, we add padding bytes to fullfill the condition of packet size
	//If it is greater than the packet size we return an error
	void set_fixed_packet_size(uint16_t packet_size);
	uint64_t GET_SMCOM_VERSION();

	uint8_t get_packet_data_length(const CT * packet);
	CT * duplicate_message_packet(const CT * packet);

	virtual SMCom_Status_t __write__(const uint8_t * buffer, uint16_t len) = 0;
	virtual SMCom_Status_t __read__(uint8_t * buffer, uint16_t len) = 0;
	virtual size_t __available__() = 0;
	//In order to call listener, user must provide __read__ and __available__ functions
	SMCom_Status_t listener(void);

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
			"SMCOM_MESSAGE_ID_ERROR",
			"SMCOM_NULL_MESSAGE",
		};
		return str_SMCom_Status_t[t];
	}

	static const uint8_t HEADER_SIZE = SIZEOF_PACKET(CT)+1;
	static const uint8_t MAX_MSG_LENGTH = 0xFF;

protected:

	void clear_tx_flag();
	void clear_rx_flag();
	void clear_configuration_flags();

	CT com_packet;


	SMCom_Status_t __write__retry(const uint8_t * buffer, uint8_t len, uint8_t retry=1);


	SMCom_Status_t common_write(const uint8_t * buffer, uint8_t len, uint8_t retry=1);
	SMCom_Status_t common_write_polling(const uint8_t * buffer, uint8_t len, uint8_t retry=1);
	SMCom_Status_t common_write_txbuffer(const uint8_t * buffer, uint8_t len, uint8_t retry=1);

	#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
	SMCom_Status_t common_request(const uint8_t * buffer, uint8_t len, uint32_t timeout, request_response_callback fptr);
	#endif
	
	SMCom_Status_t common_read_internal(const uint8_t * raw_bytes, uint8_t len,uint8_t end_byte_start_offset);
	SMCom_Status_t additional_buffer_check();
	
	SMCom_Status_t common_verify_message_header(const uint8_t * raw_bytes, uint16_t * len, bool copy_buffer);
	SMCom_Status_t common_handle_message_data(const uint8_t * raw_bytes, uint16_t len, bool copy_buffer);

	SMCom_Status_t common_start_write_queue(uint8_t retry=1);
	SMCom_Status_t common_push_to_queue(const uint8_t * buffer, uint8_t len,uint8_t retry=1);	
	SMCom_Status_t common_finalize_queue(uint8_t retry=1);

	SMCom_Status_t respond_smcom_special_messages(CT * packet);
	SMCom_Status_t common_respond_smcom_special_messages(CT * packet);

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

	uint8_t * rx_buffer = NULL;
	uint16_t rx_iter = 0;
	uint16_t rx_buf_size = 0;
	
	uint8_t * tx_buffer = NULL;
	uint16_t tx_buf_size = 0;

	configuration_flags conflag;

	uint16_t message_end_index;

	message_flags rxflag;
	message_flags txflag;

	uint16_t packet_size = 0;

	uint16_t last_crc;

	#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
	//Timeout counter counts miliseconds
	uint32_t timeout_counter = 0;
	#endif

	//Polynomial x^16 + x^12 + x^5 + 1
	#define POLY_CCITT 0x1021 //'0b1000000100001'
	//Polynomial x^16 + x^15 + x^2 + 1
	#define POLY_IBM 0x8005 //'0b1000000000000101'

	#define CRC_IBM_SEED 0xffff
	#define CRC_CCITT_SEED 0x1d0f

	uint16_t compute_crc_ibm(uint16_t crc, uint8_t data);
	uint16_t get_crc_ibm(const uint8_t * buffer, uint8_t len, uint16_t crc = CRC_IBM_SEED);

};


#include "SMCom.tpp"

#endif