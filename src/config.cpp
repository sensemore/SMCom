#include "SMCom.h"
#include "public_test.h"
#include <pybind11/pybind11.h>
#include <string.h>

namespace py = pybind11;

// compile code :
// c++ -shared -fPIC $(python3 -m pybind11 --includes) config.cpp SMCom.cpp public_test.cpp -o SMCom$(python3-config --extension-suffix)


// Important fix required to template class : wrapping void is not working at all

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

// template<typename T>
// void declareSMCom(py::module &m, const std::string& typestr) {
//     using Class = SMCom<T>;
//     std::string pyclass_name = std::string("SMCom") + typestr;
//     py::class_<Class>(m, pyclass_name.c_str(), py::buffer_protocol(), py::dynamic_attr())
//     .def("rx_event_handler_callback", Class::rx_event_handler_callback)
//     .def("tx_event_handler_callback", Class::tx_event_handler_callback)
//     #ifdef SMCOM_CONFIG_REQUEST_RESPONSE
// 	.def("request_response_callback", Class::request_response_callback)
// 	#endif
//     .def(py::init<uint16_t, uint16_t, typename Class::rx_event_handler_callback, typename Class::tx_event_handler_callback>()) //maybe typename Class::rx_event_handler_callback not sure
//     .def(py::init<uint16_t, uint16_t, uint8_t, typename Class::rx_event_handler_callback, typename Class::tx_event_handler_callback>())
//     .def(py::init<uint8_t*, uint16_t, uint8_t*, uint16_t, typename Class::rx_event_handler_callback, typename Class::tx_event_handler_callback>())
//     .def(py::init<uint8_t*, uint16_t, uint8_t*, uint16_t, uint8_t, typename Class::rx_event_handler_callback, typename Class::tx_event_handler_callback>())
//     .def("verify_message_header", &Class::verify_message_header)
//     .def("handle_message_data", &Class::handle_message_data)
//     .def("write", static_cast<SMCom_Status_t (Class::*)(uint8_t, const uint8_t*, uint8_t)>(&Class::write))
//     .def("write", static_cast<SMCom_Status_t (Class::*)(uint8_t, uint8_t, const uint8_t*, uint8_t)>(&Class::write))
//     .def("write_public", &Class::write_public)
//     #ifdef SMCOM_CONFIG_REQUEST_RESPONSE
//     .def("request", static_cast<SMCom_Status_t (Class::*)(uint8_t, const uint8_t*, uint8_t, uint32_t, Class::request_response_callback)>(&Class::request))
//     .def("request", static_cast<SMCom_Status_t (Class::*)(uint8_t, uint8_t, const uint8_t*, uint8_t, uint32_t, Class::request_response_callback)>(&Class::request))
//     .def("ping", static_cast<SMCom_Status_t (Class::*)(uint8_t, uint32_t, Class::request_response_callback)>(&Class::ping))
//     .def("ping", static_cast<SMCom_Status_t (Class::*)(uint32_t, Class::request_response_callback)>(&Class::ping))
//     .def("get_version", static_cast<SMCom_Status_t (Class::*)(uint8_t, uint32_t, Class::request_response_callback)>(&Class::get_version))
//     .def("get_version", static_cast<SMCom_Status_t (Class::*)(uint32_t, Class::request_response_callback)>(&Class::get_version))
//     #endif
//     .def("respond", static_cast<SMCom_Status_t (Class::*)(uint8_t, const uint8_t*, uint8_t)>(&Class::respond))
//     .def("respond", static_cast<SMCom_Status_t (Class::*)(uint8_t, uint8_t, const uint8_t*, uint8_t)>(&Class::respond))
//     .def("respond", static_cast<SMCom_Status_t (Class::*)(const T*, const uint8_t*, uint8_t)>(&Class::respond))
//     .def("start_write_queue", static_cast<SMCom_Status_t (Class::*)(SMCom_message_types, uint8_t, uint8_t, uint8_t)>(&Class::start_write_queue))
//     .def("start_write_queue", static_cast<SMCom_Status_t (Class::*)(SMCom_message_types, uint8_t, uint8_t)>(&Class::start_write_queue))
//     .def("push_to_queue", &Class::push_to_queue)
//     .def("finalize_queue", &Class::finalize_queue)
//     .def("set_fixed_packet_size", &Class::set_fixed_packet_size)
//     .def("GET_SMCOM_VERSION", &Class::GET_SMCOM_VERSION)
//     .def("get_packet_data_length", &Class::get_packet_data_length)
//     .def("duplicate_message_packet", &Class::duplicate_message_packet) /// ARE VIRTUALS SHOULD BE DEFINED IN THIS ?
//     .def("listener", &Class::listener)
//     .def("is_packet_broken", &Class::is_packet_broken)
//     .def("is_crc_failed", &Class::is_crc_failed)
//     #ifdef SMCOM_CONFIG_REQUEST_RESPONSE
//     .def("increase_ms_timer", &Class::increase_ms_timer)
//     .def("run_request_scheduler", &Class::run_request_scheduler)
//     #endif
//     .def("get_rx_buffer_size", &Class::get_rx_buffer_size)
//     .def("assign_new_id", &Class::assign_new_id)
//     .def_static("resolve_status", &Class::resolve_status)
//     .def_readwrite("HEADER_SIZE", &Class::HEADER_SIZE)
//     .def_readwrite("MAX_MSG_LENGTH", &Class::MAX_MSG_LENGTH);
// }



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

    py::class_<public_node, PyNode>(m, "public_node")
        .def(py::init<uint16_t, uint16_t, uint8_t, public_node::rx_event_handler_callback, public_node::tx_event_handler_callback, std::string>())
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
        // declareSMCom<SMCOM_PUBLIC>(m, "SMCOM_PUBLIC");
        // declareSMCom<SMCOM_PRIVATE>(m, "SMCOM_PRIVATE");
}