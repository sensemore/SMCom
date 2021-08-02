#include "SMCom.h"
#include "public_test.h"
#include <pybind11/pybind11.h>
#include <string.h>

namespace py = pybind11;

// compile code :
// c++ -shared -fPIC $(python3 -m pybind11 --includes) config.cpp SMCom.cpp public_test.cpp -o SMCom$(python3-config --extension-suffix)

class PySMCOM : public SMCom<SMCOM_PUBLIC> {
public:
    using Class = SMCom<SMCOM_PUBLIC>;
    using Class::SMCom;
    /* Trampoline (need one for each virtual function) */
    SMCom_Status_t __write__(const uint8_t * buffer, uint16_t len) override {
        PYBIND11_OVERRIDE(
            SMCom_Status_t, /* Return type */
            Class,      /* Parent class */
            __write__,          /* Name of function in C++ (must match Python name) */
            buffer,
            len   /* Argument(s) */
        );
    }

    SMCom_Status_t __read__(uint8_t * buffer, uint16_t len) override {
        PYBIND11_OVERRIDE(
            SMCom_Status_t,   /* Return type */
            Class,      /* Parent class */
            __read__,          /* Name of function in C++ (must match Python name) */
            buffer,
            len                 /* Argument(s) */
        );
    }

    size_t __available__() override {
        PYBIND11_OVERRIDE(
            size_t,
            Class,
            __available__
        );
    }
};

template<typename T>
void declareSMCom(py::module &m, const std::string& typestr) {
    std::string pyclass_name = std::string("SMCom") + typestr;
    auto a = py::class_<SMCom<T>, PySMCOM>(m, pyclass_name.c_str())
    .def("rx_event_handler_callback", &SMCom<T>::rx_event_handler_callback_ptr)
    .def("tx_event_handler_callback", &SMCom<T>::tx_event_handler_callback_ptr)
    #ifdef SMCOM_CONFIG_REQUEST_RESPONSE
	.def("request_response_callback", SMCom<T>::request_response_callback)
	#endif
    .def("verify_message_header", &SMCom<T>::verify_message_header)
    .def("handle_message_data", &SMCom<T>::handle_message_data)
    #ifdef SMCOM_CONFIG_REQUEST_RESPONSE
    .def("request", static_cast<SMCom_Status_t (SMCom<T>::*)(uint8_t, const uint8_t*, uint8_t, uint32_t, SMCom<T>::request_response_callback)>(&SMCom<T>::request))
    .def("request", static_cast<SMCom_Status_t (SMCom<T>::*)(uint8_t, uint8_t, const uint8_t*, uint8_t, uint32_t, SMCom<T>::request_response_callback)>(&SMCom<T>::request))
    .def("ping", static_cast<SMCom_Status_t (SMCom<T>::*)(uint8_t, uint32_t, SMCom<T>::request_response_callback)>(&SMCom<T>::ping))
    .def("ping", static_cast<SMCom_Status_t (SMCom<T>::*)(uint32_t, SMCom<T>::request_response_callback)>(&SMCom<T>::ping))
    .def("get_version", static_cast<SMCom_Status_t (SMCom<T>::*)(uint8_t, uint32_t, SMCom<T>::request_response_callback)>(&SMCom<T>::get_version))
    .def("get_version", static_cast<SMCom_Status_t (SMCom<T>::*)(uint32_t, SMCom<T>::request_response_callback)>(&SMCom<T>::get_version))
    #endif
    .def("push_to_queue", &SMCom<T>::push_to_queue)
    .def("finalize_queue", &SMCom<T>::finalize_queue)
    .def("set_fixed_packet_size", &SMCom<T>::set_fixed_packet_size)
    .def("GET_SMCOM_VERSION", &SMCom<T>::GET_SMCOM_VERSION)
    .def("get_packet_data_length", &SMCom<T>::get_packet_data_length)
    .def("duplicate_message_packet", &SMCom<T>::duplicate_message_packet) /// ARE VIRTUALS SHOULD BE DEFINED IN THIS ?
    .def("listener", &SMCom<T>::listener)
    .def("is_packet_broken", &SMCom<T>::is_packet_broken)
    .def("is_crc_failed", &SMCom<T>::is_crc_failed)
    #ifdef SMCOM_CONFIG_REQUEST_RESPONSE
    .def("increase_ms_timer", &SMCom<T>::increase_ms_timer)
    .def("run_request_scheduler", &SMCom<T>::run_request_scheduler)
    #endif
    .def("get_rx_buffer_size", &SMCom<T>::get_rx_buffer_size)
    .def("__write__", &SMCom<T>::__write__)
    .def_static("resolve_status", &SMCom<T>::resolve_status)
    .def_property_readonly_static("HEADER_SIZE", [](py::object) { return SMCom<T>::HEADER_SIZE;})
    .def_property_readonly_static("MAX_MSG_LENGTH", [](py::object) { return SMCom<T>::MAX_MSG_LENGTH;});

    if(typeid(T) == typeid(SMCOM_PUBLIC)){
        a.def(py::init<uint16_t, uint16_t, uint8_t>())
        .def(py::init<uint8_t*, uint16_t, uint8_t*, uint16_t, uint8_t>())
        .def("write", static_cast<SMCom_Status_t (SMCom<T>::*)(uint8_t, uint8_t, const uint8_t*, uint8_t)>(&SMCom<T>::write))
        .def("respond", static_cast<SMCom_Status_t (SMCom<T>::*)(const T*, const uint8_t*, uint8_t)>(&SMCom<T>::respond))
        .def("start_write_queue", static_cast<SMCom_Status_t (SMCom<T>::*)(SMCom_message_types, uint8_t, uint8_t, uint8_t)>(&SMCom<T>::start_write_queue))
        .def("write_public", &SMCom<T>::write_public)
        .def("assign_new_id", &SMCom<T>::assign_new_id);
    }

    // if(typeid(T) == typeid(SMCOM_PRIVATE)){
    //     a.def(py::init<uint16_t, uint16_t>())
    //     .def(py::init<uint8_t*, uint16_t, uint8_t*, uint16_t>())
    //     .def("write", static_cast<SMCom_Status_t (SMCom<T>::*)(uint8_t, const uint8_t*, uint8_t)>(&SMCom<T>::write))
    //     .def("respond", static_cast<SMCom_Status_t (SMCom<T>::*)(uint8_t, const uint8_t*, uint8_t)>(&SMCom<T>::respond))
    //     .def("respond", static_cast<SMCom_Status_t (SMCom<T>::*)(const T*, const uint8_t*, uint8_t)>(&SMCom<T>::respond))
    //     .def("start_write_queue", static_cast<SMCom_Status_t (SMCom<T>::*)(SMCom_message_types, uint8_t, uint8_t)>(&SMCom<T>::start_write_queue));
    // }
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
        declareSMCom<SMCOM_PUBLIC>(m, "SMCOM_PUBLIC");
        // declareSMCom<SMCOM_PRIVATE>(m, "SMCOM_PRIVATE");
}