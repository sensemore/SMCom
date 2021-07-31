#include "SMCom.h"
#include "public_test.h"
#include <pybind11/pybind11.h>

namespace py = pybind11;


class PyNode : public public_node {
public:
    /* Inherit the constructors */
    using public_node::public_node;

    /* Trampoline (need one for each virtual function) */
    SMCom_Status_t __write__(const uint8_t * buffer, uint16_t len) override {
        PYBIND11_OVERRIDE(
            SMCom_Status_t, /* Return type */
            public_node,      /* Parent class */
            __write__,          /* Name of function in C++ (must match Python name) */
            buffer,
            len   /* Argument(s) */
        );
    }

    SMCom_Status_t __read__(uint8_t * buffer, uint16_t len) override {
        PYBIND11_OVERRIDE(
            SMCom_Status_t,   /* Return type */
            public_node,      /* Parent class */
            __read__,          /* Name of function in C++ (must match Python name) */
            buffer,
            len                 /* Argument(s) */
        );
    }

    size_t __available__() override {
        PYBIND11_OVERRIDE(
            size_t,
            public_node,
            __available__
        );
    }

    void push_message_into_rx(const uint8_t * buffer, uint16_t len) {
        PYBIND11_OVERRIDE(
            void,
            public_node,
            push_message_into_rx,
            buffer,
            len
        );
    }
};

PYBIND11_MODULE(SMCom, m) {
    py::class_<public_node, PyNode>(m, "public_node")
        .def(py::init<uint16_t,uint16_t,uint8_t, public_node::rx_event_handler_callback, public_node::tx_event_handler_callback,std::string>())
        .def("__read__", &public_node::__read__)
        .def("__write__", &public_node::__write__)
        .def("available", &public_node::__available__)
        .def("push_message_into_rx", &public_node::push_message_into_rx)
        .def("copy_txqueue_into_another_rxqueue", &public_node::copy_txqueue_into_another_rxqueue)
        .def("print_rx", &public_node::print_rx)
        .def("print_tx", &public_node::print_tx)
        .def_readwrite("write_queue", &public_node::write_queue)
        .def_readwrite("read_queue", &public_node::read_queue)
        .def_readwrite("name", &public_node::name);
}

template<typename T>
void declare_array(py::module &m, std::string &typestr) {
    using Class = SMCom<T>;
    std::string pyclass_name = std::string("SMCom") + typestr;
    py::class_<Class>(m, pyclass_name.c_str(), py::buffer_protocol(), py::dynamic_attr())
    .def(py::init<uint16_t, uint16_t, rx_event_handler_callback, tx_event_handler_callback>()) //maybe Class::rx_event_handler_callback not sure
    .def(py::init<uint16_t, uint16_t, uint8_t, rx_event_handler_callback, tx_event_handler_callback>())
    .def("size", &Class::size)
    .def("width", &Class::width)
    .def("height", &Class::height);
}

template <typename CT>
class SMCom{
public:
	typedef void (*rx_event_handler_callback)(SMCom_event_types event, SMCom_Status_t status, const CT * packet);
	rx_event_handler_callback rx_event_handler_callback_ptr = NULL;

	typedef void (*tx_event_handler_callback)(SMCom_event_types event, SMCom_Status_t status, const CT * packet);
	tx_event_handler_callback tx_event_handler_callback_ptr = NULL;

	#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
	typedef void(*request_response_callback)(SMCom_Status_t status, const CT * packet);
	#endif
	
	////Constructors with dynamic buffers, uses C++ new allocater
	SMCom(uint16_t rx_buf_size, uint16_t tx_buf_size, rx_event_handler_callback rx, tx_event_handler_callback tx);
	SMCom(uint16_t rx_buf_size, uint16_t tx_buf_size, uint8_t id, rx_event_handler_callback rx, tx_event_handler_callback tx);
	//Constructors with static buffers
	SMCom(uint8_t * rx_buffer, uint16_t rx_buf_size, uint8_t * tx_buffer, uint16_t tx_buf_size, rx_event_handler_callback rx, tx_event_handler_callback tx);
	SMCom(uint8_t * rx_buffer, uint16_t rx_buf_size, uint8_t * tx_buffer, uint16_t tx_buf_size, uint8_t id, rx_event_handler_callback rx, tx_event_handler_callback tx);
	~SMCom();

	SMCom_Status_t verify_message_header(const uint8_t * raw_bytes, uint16_t * len);
	SMCom_Status_t handle_message_data(const uint8_t * raw_bytes, uint16_t len);

	SMCom_Status_t write(uint8_t message_id, const uint8_t * buffer, uint8_t len);
	SMCom_Status_t write(uint8_t receiver_id, uint8_t message_id, const uint8_t * buffer, uint8_t len);

	SMCom_Status_t write_public(uint8_t message_id, const uint8_t * buffer, uint8_t len);

	#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
	SMCom_Status_t request(uint8_t message_id, const uint8_t * buffer, uint8_t len, uint32_t timeout, request_response_callback fptr = NULL);
	SMCom_Status_t request(uint8_t receiver_id, uint8_t message_id, const uint8_t * buffer, uint8_t len, uint32_t timeout, request_response_callback fptr = NULL);

	SMCom_Status_t ping(uint8_t receiver_id, uint32_t timeout, request_response_callback fptr);
	SMCom_Status_t ping(uint32_t timeout, request_response_callback fptr);

	SMCom_Status_t get_version(uint8_t receiver_id, uint32_t timeout, request_response_callback fptr);
	SMCom_Status_t get_version(uint32_t timeout, request_response_callback fptr);
	#endif
	SMCom_Status_t respond(uint8_t message_id, const uint8_t * buffer, uint8_t len);
	SMCom_Status_t respond(uint8_t receiver_id, uint8_t message_id, const uint8_t * buffer, uint8_t len);
	SMCom_Status_t respond(const CT * inc_packet, const uint8_t * buffer, uint8_t len);
	


	SMCom_Status_t start_write_queue(SMCom_message_types t, uint8_t receiver_id,uint8_t message_id, uint8_t len);
	SMCom_Status_t start_write_queue(SMCom_message_types t, uint8_t message_id, uint8_t len);
	SMCom_Status_t push_to_queue(const uint8_t * buffer, uint8_t len);
	SMCom_Status_t finalize_queue();

	//If given message length is smaller than the packet size, we add padding bytes to fullfill the condition of packet size
	//If it is greater than the packet size we return an error
	void set_fixed_packet_size(uint16_t packet_size);
	uint64_t GET_SMCOM_VERSION();

	uint8_t get_packet_data_length(const CT * packet);
	CT * duplicate_message_packet(const CT * packet);

	virtual SMCom_Status_t __write__(const uint8_t * buffer, uint16_t len) = 0;
	virtual SMCom_Status_t __read__(uint8_t * buffer, uint16_t len);
	virtual size_t __available__();
	//In order to call listener user must provide __read__ and __available__ functions
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

	static const uint8_t HEADER_SIZE = sizeof(CT)+1;
	static const uint8_t MAX_MSG_LENGTH = 0xFF;

protected:

	void clear_tx_flag();
	void clear_rx_flag();

private:

	SMCom_Status_t common_write(const uint8_t * buffer, uint8_t len);
	SMCom_Status_t common_write_polling(const uint8_t * buffer, uint8_t len);
	SMCom_Status_t common_write_txbuffer(const uint8_t * buffer, uint8_t len);

	#ifdef SMCOM_CONFIG_REQUEST_RESPONSE
	SMCom_Status_t common_request(const uint8_t * buffer, uint8_t len, uint32_t timeout, request_response_callback fptr);
	#endif
	
	SMCom_Status_t common_read_internal(const uint8_t * raw_bytes, uint8_t len,uint8_t end_byte_start_offset);
	SMCom_Status_t additional_buffer_check();
	
	SMCom_Status_t common_verify_message_header(const uint8_t * raw_bytes, uint16_t * len, bool copy_buffer);
	SMCom_Status_t common_handle_message_data(const uint8_t * raw_bytes, uint16_t len, bool copy_buffer);

	SMCom_Status_t common_start_write_queue();
	SMCom_Status_t common_push_to_queue(const uint8_t * buffer, uint8_t len);	
	SMCom_Status_t common_finalize_queue();

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


	CT com_packet;

	uint8_t * rx_buffer = NULL;
	uint16_t rx_iter = 0;
	uint16_t rx_buf_size = 0;
	
	uint8_t * tx_buffer = NULL;
	uint16_t tx_buf_size = 0;

	configuration_flags conflag = (configuration_flags){0};

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
	uint16_t get_crc_ibm(const uint8_t * buffer, uint8_t len,uint16_t crc = CRC_IBM_SEED);
};
