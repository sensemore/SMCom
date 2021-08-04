#include "SMCom.h"
#include "public_test.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <string.h>
#include <vector>
#include <iterator>

#define MAX_DATA_LENGTH 255

namespace py = pybind11;

// compile code :
// c++ -shared -fPIC $(python3 -m pybind11 --includes) config.cpp SMCom.cpp public_test.cpp -o SMCom$(python3-config --extension-suffix)

class pySMCOM_PUBLIC{
    public:
    uint8_t data_len;			//<! Data len in the packet, user can use this instead defining data length in the message max data length is 255
    uint8_t receiver_id;		//<! Receiver id in the communication can take value between 0-13 (14 and 15 is reserved), located at 0b0000xxxx
    uint8_t transmitter_id;	//<! Transmitter id in the communication can take value between 0-13 (14 and 15 is reserved), located at 0bxxxx0000
    uint8_t message_type;		//<! Message type , can be write/request/response/indicate etc. User does not change this max value is 3, located at 0b000000xx
    uint8_t message_id;		//<! Message id is defined by the user similar to uuid in BLE communication. Max value is 63, located at 0bxxxxxx00
    std::vector<uint8_t> data;			//<! Data pointer for derived classes
};

class py_smcom_tramp : public SMCom<SMCOM_PUBLIC>{
    public:
    py_smcom_tramp(uint16_t rx_buf_size, uint16_t tx_buf_size, uint8_t id) : SMCom::SMCom(rx_buf_size, tx_buf_size, id, NULL, NULL){;}
    virtual void __rx_callback__(SMCom_event_types event, SMCom_Status_t status, pySMCOM_PUBLIC packet) = 0;
};

void py_rx_event_handler_callback(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PUBLIC * packet);

class PySMCOM : py_smcom_tramp {
public:
    using tramp = py_smcom_tramp;
    PySMCOM(uint16_t rx_buf_size, uint16_t tx_buf_size, uint8_t id) : tramp(0, 0, 0)
    {
        this->rx_event_handler_callback_ptr = (void (*)(SMCom_event_types, SMCom_Status_t, const SMCOM_PUBLIC*)) &PySMCOM::py_rx_event_handler_callback;
    }

    static PySMCOM create(void){
        return PySMCOM(0,0,0);
    }

    // void __rx_callback__(SMCom_event_types event, SMCom_Status_t status, pySMCOM_PUBLIC packet);
    void py_rx_event_handler_callback(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PUBLIC* packet){
        pySMCOM_PUBLIC pypacket;
        pypacket.data = std::vector<uint8_t>(packet->data, packet->data + sizeof(packet->data) / sizeof(int));
        pypacket.data_len = packet->data_len;
        pypacket.message_id = packet->message_id;
        pypacket.message_type = packet->message_type;
        pypacket.receiver_id = packet->receiver_id;
        pypacket.transmitter_id = packet->transmitter_id;
        return __rx_callback__(event, status, pypacket);
    }

    // SMCom_Status_t __write__(const uint8_t * buffer, uint16_t len);
    /* Trampoline (need one for each virtual function) */
    SMCom_Status_t __write__(const uint8_t * buffer, uint16_t len) override {
        PYBIND11_OVERRIDE_PURE(
            SMCom_Status_t,     /* Return type */
            tramp,              /* Parent class */
            __write__,          /* Name of function in C++ (must match Python name) */
            buffer,
            len                 /* Argument(s) */
        );
    }

    SMCom_Status_t __read__(uint8_t * buffer, uint16_t len) override {
        PYBIND11_OVERRIDE(
            SMCom_Status_t,
            tramp,      
            __read__,          
            buffer,
            len           
        );
    }

    size_t __available__() override {
        PYBIND11_OVERRIDE(
            size_t,
            tramp,
            __available__
        );
    }

    void __rx_callback__(SMCom_event_types event, SMCom_Status_t status, pySMCOM_PUBLIC packet){
        PYBIND11_OVERRIDE_PURE(
            void,
            tramp,
            __rx_callback__,
            event,
            status,
            packet
        );
    }
};

template<typename T>
void declareSMCom(py::module &m, const std::string& typestr) {
    std::string pyclass_name = "SMCom";
    using Class = py_smcom_tramp;
    auto a = py::class_<Class, PySMCOM>(m, pyclass_name.c_str())
    #ifdef SMCOM_CONFIG_REQUEST_RESPONSE
	.def("request_response_callback", Class::request_response_callback)
	#endif
    .def("verify_message_header", &Class::verify_message_header)
    .def("handle_message_data", &Class::handle_message_data)
    // // .def("__rx_callback__", &PySMCOM::__rx_callback__)
    // // .def("rx_event_handler_callback", &Class::rx_event_handler_callback)
    // // .def("tx_event_handler_callback", &Class::tx_event_handler_callback)
    // // .def("py_rx_callback", &Class::py_rx_callback)
    #ifdef SMCOM_CONFIG_REQUEST_RESPONSE
    .def("request", static_cast<SMCom_Status_t (Class::*)(uint8_t, const uint8_t*, uint8_t, uint32_t, Class::request_response_callback)>(&Class::request))
    .def("request", static_cast<SMCom_Status_t (Class::*)(uint8_t, uint8_t, const uint8_t*, uint8_t, uint32_t, Class::request_response_callback)>(&Class::request))
    .def("ping", static_cast<SMCom_Status_t (Class::*)(uint8_t, uint32_t, Class::request_response_callback)>(&Class::ping))
    .def("ping", static_cast<SMCom_Status_t (Class::*)(uint32_t, Class::request_response_callback)>(&Class::ping))
    .def("get_version", static_cast<SMCom_Status_t (Class::*)(uint8_t, uint32_t, Class::request_response_callback)>(&Class::get_version))
    .def("get_version", static_cast<SMCom_Status_t (Class::*)(uint32_t, Class::request_response_callback)>(&Class::get_version))
    #endif
    .def("push_to_queue", &Class::push_to_queue)
    .def("finalize_queue", &Class::finalize_queue)
    .def("set_fixed_packet_size", &Class::set_fixed_packet_size)
    .def("GET_SMCOM_VERSION", &Class::GET_SMCOM_VERSION)
    .def("get_packet_data_length", &Class::get_packet_data_length)
    .def("duplicate_message_packet", &Class::duplicate_message_packet)
    .def("listener", &Class::listener)
    .def("is_packet_broken", &Class::is_packet_broken)
    .def("is_crc_failed", &Class::is_crc_failed)
    #ifdef SMCOM_CONFIG_REQUEST_RESPONSE
    .def("increase_ms_timer", &Class::increase_ms_timer)
    .def("run_request_scheduler", &Class::run_request_scheduler)
    #endif
    .def("get_rx_buffer_size", &Class::get_rx_buffer_size)
    .def("__write__", &Class::__write__)
    .def("__read__", &Class::__read__)
    .def("__available__", &Class::__available__)
    .def("listener", &Class::listener)
    .def_static("resolve_status", &Class::resolve_status)
    .def_property_readonly_static("HEADER_SIZE", [](py::object) { return Class::HEADER_SIZE;})
    .def_property_readonly_static("MAX_MSG_LENGTH", [](py::object) { return Class::MAX_MSG_LENGTH;});

    if(typeid(T) == typeid(SMCOM_PUBLIC)){
        a.def(py::init<uint16_t, uint16_t, uint8_t>())
        // .def(py::init<uint8_t*, uint16_t, uint8_t*, uint16_t, uint8_t, typename Class::rx_event_handler_callback, typename Class::tx_event_handler_callback>())
        .def("write", static_cast<SMCom_Status_t (Class::*)(uint8_t, uint8_t, const uint8_t*, uint8_t)>(&Class::write))
        .def("respond", static_cast<SMCom_Status_t (Class::*)(const T*, const uint8_t*, uint8_t)>(&Class::respond))
        .def("start_write_queue", static_cast<SMCom_Status_t (Class::*)(SMCom_message_types, uint8_t, uint8_t, uint8_t)>(&Class::start_write_queue))
        .def("write_public", &Class::write_public)
        .def("assign_new_id", &Class::assign_new_id);
    }
}

PYBIND11_MODULE(SMCom, m) {
    py::enum_<SMCom_Status_t>(m, "SMCom_Status_t")
        .value("SMCOM_STATUS_DEFAULT", SMCom_Status_t::SMCOM_STATUS_DEFAULT)
        .value("SMCOM_STATUS_SUCCESS", SMCom_Status_t::SMCOM_STATUS_SUCCESS)
        .value("SMCOM_STATUS_PORT_BUSY", SMCom_Status_t::SMCOM_STATUS_PORT_BUSY)
        .value("SMCOM_STATUS_TIMEOUT", SMCom_Status_t::SMCOM_STATUS_TIMEOUT)
        .value("SMCOM_STATUS_FAIL", SMCom_Status_t::SMCOM_STATUS_FAIL)
        .value("SMCOM_STATUS_RX_ERROR", SMCom_Status_t::SMCOM_STATUS_RX_ERROR)
        .value("SMCOM_STATUS_CRC_ERROR", SMCom_Status_t::SMCOM_STATUS_CRC_ERROR)
        .value("SMCOM_STATUS_START_BYTE_ERROR", SMCom_Status_t::SMCOM_STATUS_START_BYTE_ERROR)
        .value("SMCOM_STATUS_END_BYTE_ERROR", SMCom_Status_t::SMCOM_STATUS_END_BYTE_ERROR)
        .value("SMCOM_STATUS_HEADER_LENGTH_ERROR", SMCom_Status_t::SMCOM_STATUS_HEADER_LENGTH_ERROR)
        .value("SMCOM_STATUS_MESSAGE_ID_ERROR", SMCom_Status_t::SMCOM_STATUS_MESSAGE_ID_ERROR)
        .value("SMCOM_STATUS_NULL_MESSAGE", SMCom_Status_t::SMCOM_STATUS_NULL_MESSAGE)
        .export_values();

    py::enum_<SMCom_event_types>(m, "SMCom_event_types")
        .value("SM_WRITE_EVENT", SMCom_event_types::SM_WRITE_EVENT)
        .value("SM_REQUEST_EVENT", SMCom_event_types::SM_REQUEST_EVENT)
        .value("SM_RESPONSE_EVENT", SMCom_event_types::SM_RESPONSE_EVENT)
        .value("SM_INDICATE_EVENT", SMCom_event_types::SM_INDICATE_EVENT)
        .export_values();

    py::class_<pySMCOM_PUBLIC>(m, "pySMCOM_PUBLIC")
        .def(py::init<>())
        .def_readwrite("data_len", &pySMCOM_PUBLIC::data_len)
        .def_readwrite("receiver_id", &pySMCOM_PUBLIC::receiver_id)
        .def_readwrite("transmitter_id", &pySMCOM_PUBLIC::transmitter_id)
        .def_readwrite("message_type", &pySMCOM_PUBLIC::message_type)
        .def_readwrite("message_id", &pySMCOM_PUBLIC::message_id)
        .def_readwrite("data", &pySMCOM_PUBLIC::data);

    declareSMCom<SMCOM_PUBLIC>(m, "SMCOM_PUBLIC");

}


    // py::class_<public_node>(m, "public_node")
    //     .def(py::init<uint16_t, uint16_t, uint8_t, std::string>())
    //     .def("__read__", &public_node::__read__)
    //     .def("__write__", &public_node::__write__)
    //     .def("available", &public_node::__available__)
    //     .def("push_message_into_rx", &public_node::push_message_into_rx)
    //     .def("copy_txqueue_into_another_rxqueue", &public_node::copy_txqueue_into_another_rxqueue)
    //     .def("print_rx", &public_node::print_rx)
    //     .def("print_tx", &public_node::print_tx)
    //     .def_readwrite("write_queue", &public_node::write_queue)
    //     .def_readwrite("read_queue", &public_node::read_queue)
    //     .def_readwrite("name", &public_node::name);




    // if(typeid(T) == typeid(SMCOM_PRIVATE)){
    //     a.def(py::init<uint16_t, uint16_t>())
    //     .def(py::init<uint8_t*, uint16_t, uint8_t*, uint16_t>())
    //     .def("write", static_cast<SMCom_Status_t (SMCom<T>::*)(uint8_t, const uint8_t*, uint8_t)>(&SMCom<T>::write))
    //     .def("respond", static_cast<SMCom_Status_t (SMCom<T>::*)(uint8_t, const uint8_t*, uint8_t)>(&SMCom<T>::respond))
    //     .def("respond", static_cast<SMCom_Status_t (SMCom<T>::*)(const T*, const uint8_t*, uint8_t)>(&SMCom<T>::respond))
    //     .def("start_write_queue", static_cast<SMCom_Status_t (SMCom<T>::*)(SMCom_message_types, uint8_t, uint8_t)>(&SMCom<T>::start_write_queue));
    // }