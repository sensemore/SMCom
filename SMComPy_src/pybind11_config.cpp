#include "SMCom.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <string.h>
#include <vector>
#include <iterator>

#define MAX_DATA_LENGTH 255


namespace py = pybind11;

// compile code :
// c++ -shared -fPIC $(python3 -m pybind11 --includes) config.cpp SMCom.cpp -o SMCom$(python3-config --extension-suffix)

class pySMCOM_PUBLIC{
    public:
    uint8_t data_len = 0;			        //<! Data len in the packet, user can use this instead defining data length in the message max data length is 255
    uint8_t receiver_id = 0;		        //<! Receiver id in the communication can take value between 0-13 (14 and 15 is reserved), located at 0b0000xxxx
    uint8_t transmitter_id = 0;	            //<! Transmitter id in the communication can take value between 0-13 (14 and 15 is reserved), located at 0bxxxx0000
    uint8_t message_type = 0;		        //<! Message type , can be write/request/response/indicate etc. User does not change this max value is 3, located at 0b000000xx
    uint8_t message_id = 0;	            //<! Message id is defined by the user similar to uuid in BLE communication. Max value is 63, located at 0bxxxxxx00
    std::vector<uint8_t> data;			//<! Data pointer for derived classes
};

class SMCom_Pair{
    public:
    std::vector<uint8_t> vec;
    SMCom_Status_t status = SMCom_Status_t::SMCOM_STATUS_DEFAULT;
};


class py_smcom_tramp : public SMCom<SMCOM_PUBLIC>{
    
    uint8_t default_txbuffer[1024] = {0};
    uint8_t default_rxbuffer[1024] = {0};
    const uint8_t write_retry_amount = 5;

    public:

    // py_smcom_tramp(uint16_t rx_buf_size, uint16_t tx_buf_size, uint8_t id) : SMCom::SMCom(rx_buf_size, tx_buf_size, id, NULL, NULL){;}
	explicit py_smcom_tramp(uint8_t id) : SMCom::SMCom(default_rxbuffer, 1024, default_txbuffer, 1024, id, NULL, NULL){
    }
    ~py_smcom_tramp(){
        smcom_log("Desctructor worked!\n");
    };

    virtual void __rx_callback__(SMCom_event_types event, SMCom_Status_t status, pySMCOM_PUBLIC packet) = 0;
    virtual void __tx_callback__(SMCom_event_types event, SMCom_Status_t status, pySMCOM_PUBLIC packet) = 0;
    virtual SMCom_Status_t __write__(std::vector<uint8_t> buffer, uint16_t len) = 0;
    virtual SMCom_Pair __read__(uint16_t len) = 0;



    SMCom_Status_t write(uint8_t receiver_id, uint8_t message_id, std::vector<uint8_t> buffer, uint8_t len){
        return SMCom<SMCOM_PUBLIC>::write(receiver_id, message_id, buffer.data(), len, write_retry_amount);
    }

    SMCom_Status_t handle_message_data(std::vector<uint8_t> buffer, uint16_t len){
        return SMCom<SMCOM_PUBLIC>::handle_message_data(buffer.data(), len);
    }

    SMCom_Status_t push_to_queue(std::vector<uint8_t> buffer, uint8_t len){
        return SMCom<SMCOM_PUBLIC>::push_to_queue(buffer.data(), len, write_retry_amount);
    }

    SMCom_Status_t __write__(const uint8_t * buffer, uint16_t len) override {
        if(buffer != NULL){
            std::vector<uint8_t> vec(buffer,buffer+len);
            return __write__(vec,len);   
        }
        //So we already know data is NULL, give empty vector
        std::vector<uint8_t> vec;
        return __write__(vec, 0);
    }

    SMCom_Status_t __read__(uint8_t * buffer, uint16_t len) override {
        SMCom_Pair tup = __read__(len);
        memcpy(buffer,tup.vec.data(),len);
        return tup.status;
    }

    uint8_t get_packet_data_length(pySMCOM_PUBLIC pypacket){
        return pypacket.data_len;
    }

    SMCom_Status_t write_public(uint8_t message_id, std::vector<uint8_t> buffer, uint8_t len){
        return SMCom<SMCOM_PUBLIC>::write_public(message_id, buffer.data(), len, write_retry_amount);
    }

    void __rx_callback__(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PUBLIC* packet){
        pySMCOM_PUBLIC pypacket;
        
        pypacket.data = std::vector<uint8_t>(packet->data, packet->data + packet->data_len);
        pypacket.data_len = packet->data_len;
        pypacket.message_id = packet->message_id;
        pypacket.message_type = packet->message_type;
        pypacket.receiver_id = packet->receiver_id;
        pypacket.transmitter_id = packet->transmitter_id;
        __rx_callback__(event, status, pypacket);
    }

    void __tx_callback__(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PUBLIC* packet){
        pySMCOM_PUBLIC pypacket;
        pypacket.data = std::vector<uint8_t>(packet->data, packet->data + packet->data_len);
        pypacket.data_len = packet->data_len;
        pypacket.message_id = packet->message_id;
        pypacket.message_type = packet->message_type;
        pypacket.receiver_id = packet->receiver_id;
        pypacket.transmitter_id = packet->transmitter_id;
        __tx_callback__(event, status, pypacket);
    }
};

class PySMCOM : public py_smcom_tramp {
public:
    using tramp = py_smcom_tramp;
    // PySMCOM(uint16_t rx_buf_size, uint16_t tx_buf_size, uint8_t id) : tramp(rx_buf_size, tx_buf_size, id){;}
    explicit PySMCOM(uint8_t id) : tramp(id){;}
    ~PySMCOM(){
    };
    
    SMCom_Status_t __write__(std::vector<uint8_t> buffer, uint16_t len) override {
        PYBIND11_OVERRIDE_PURE(
            SMCom_Status_t,     /* Return type */
            tramp,              /* Parent class */
            __write__,          /* Name of function in C++ (must match Python name) */
            buffer,
            len                 /* Argument(s) */
        );
    }

    SMCom_Pair __read__(uint16_t len) override {
        PYBIND11_OVERRIDE_PURE(
            SMCom_Pair,
            tramp,
            __read__,
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

    void __rx_callback__(SMCom_event_types event, SMCom_Status_t status, pySMCOM_PUBLIC pypacket) override{
        PYBIND11_OVERRIDE_PURE(
            void,
            tramp,
            __rx_callback__,
            event,
            status,
            pypacket
        );
    }

    void __tx_callback__(SMCom_event_types event, SMCom_Status_t status, pySMCOM_PUBLIC pypacket) override{
        PYBIND11_OVERRIDE_PURE(
            void,
            tramp,
            __tx_callback__,
            event,
            status,
            pypacket
        );
    }
};

    
    

PYBIND11_MODULE(SMComPy, m) {
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

    py::enum_<SMCom_headers>(m, "SMCom_headers")
        .value("MESSAGE_START", SMCom_headers::MESSAGE_START)
        .value("MESSAGE_END", SMCom_headers::MESSAGE_END)
        .value("PUBLIC_ID_8BIT", SMCom_headers::PUBLIC_ID_8BIT)
        .value("DEFAULT_ID_8BIT", SMCom_headers::DEFAULT_ID_8BIT)
        .value("PUBLIC_ID_4BIT", SMCom_headers::PUBLIC_ID_4BIT)
        .value("DEFAULT_ID_4BIT", SMCom_headers::DEFAULT_ID_4BIT)
        .export_values();

    py::class_<pySMCOM_PUBLIC>(m, "pySMCOM_PUBLIC")
        .def(py::init<>())
        .def_readwrite("data_len", &pySMCOM_PUBLIC::data_len)
        .def_readwrite("receiver_id", &pySMCOM_PUBLIC::receiver_id)
        .def_readwrite("transmitter_id", &pySMCOM_PUBLIC::transmitter_id)
        .def_readwrite("message_type", &pySMCOM_PUBLIC::message_type)
        .def_readwrite("message_id", &pySMCOM_PUBLIC::message_id)
        .def_readwrite("data", &pySMCOM_PUBLIC::data);

    py::class_<SMCom_Pair>(m, "SMCom_Pair")
        .def(py::init<>())
        .def_readwrite("status", &SMCom_Pair::status)
        .def_readwrite("vec", &SMCom_Pair::vec);


    py::class_<py_smcom_tramp, PySMCOM>(m, "SMCOM_PUBLIC")
        .def(py::init<uint8_t>(),"(std::vector<uint8_t> rx_buffer, uint16_t rx_buf_size, std::vector<uint8_t> tx_buffer, uint16_t tx_buf_size, uint8_t id) -> SMCOM_PUBLIC")
        
        .def("__rx_callback__", static_cast<void (py_smcom_tramp::*)(SMCom_event_types, SMCom_Status_t, pySMCOM_PUBLIC)>(&py_smcom_tramp::__rx_callback__))
        .def("__tx_callback__", static_cast<void (py_smcom_tramp::*)(SMCom_event_types, SMCom_Status_t, pySMCOM_PUBLIC)>(&py_smcom_tramp::__tx_callback__))
        .def("__write__", static_cast<SMCom_Status_t (py_smcom_tramp::*)(std::vector<uint8_t>, uint16_t)>(&py_smcom_tramp::__write__))
        .def("__read__", static_cast<SMCom_Pair (py_smcom_tramp::*)(uint16_t)>(&py_smcom_tramp::__read__))
        .def("__available__", &py_smcom_tramp::__available__)
        
        .def("handle_message_data", static_cast<SMCom_Status_t (py_smcom_tramp::*)(std::vector<uint8_t>, uint16_t)>(&py_smcom_tramp::handle_message_data))
        .def("push_to_queue", static_cast<SMCom_Status_t (py_smcom_tramp::*)(std::vector<uint8_t>, uint8_t)>(&py_smcom_tramp::push_to_queue))
        .def("finalize_queue", &py_smcom_tramp::finalize_queue)
        .def("set_fixed_packet_size", &py_smcom_tramp::set_fixed_packet_size)
        .def("GET_SMCOM_VERSION", &py_smcom_tramp::GET_SMCOM_VERSION)
        .def("get_packet_data_length", static_cast<uint8_t (py_smcom_tramp::*)(pySMCOM_PUBLIC)>(&py_smcom_tramp::get_packet_data_length))
        .def("listener", &py_smcom_tramp::listener)
        .def("is_packet_broken", &py_smcom_tramp::is_packet_broken)
        .def("is_crc_failed", &py_smcom_tramp::is_crc_failed)
        .def("get_rx_buffer_size", &py_smcom_tramp::get_rx_buffer_size)
        .def_static("resolve_status", &py_smcom_tramp::resolve_status)
        .def_property_readonly_static("HEADER_SIZE", [](py::object) { return py_smcom_tramp::HEADER_SIZE;})
        .def_property_readonly_static("MAX_MSG_LENGTH", [](py::object) { return py_smcom_tramp::MAX_MSG_LENGTH;})
        .def("write", static_cast<SMCom_Status_t (py_smcom_tramp::*)(uint8_t, uint8_t, std::vector<uint8_t>, uint8_t)>(&py_smcom_tramp::write), "(uint8_t receiver_id, uint8_t message_id, std::vector<uint8_t> buffer, uint8_t len) -> SMCom_Status_t")
        //.def("respond", static_cast<SMCom_Status_t (py_smcom_tramp::*)(uint8_t, uint8_t, std::vector<uint8_t>, uint8_t)>(&py_smcom_tramp::respond),"(uint8_t receiver_id, uint8_t message_id, std::vector<uint8_t> buffer, uint8_t len) -> SMCom_Status_t")
        .def("start_write_queue", static_cast<SMCom_Status_t (py_smcom_tramp::*)(SMCom_message_types, uint8_t, uint8_t, uint8_t, uint8_t)>(&py_smcom_tramp::start_write_queue),"(SMCom_message_types t, uint8_t receiver_id, uint8_t message_id, uint8_t len) -> SMCom_Status_T")
        .def("write_public", static_cast<SMCom_Status_t (py_smcom_tramp::*)(uint8_t, std::vector<uint8_t>, uint8_t)>(&py_smcom_tramp::write_public),"(uint8_t message_id, std::vector<uint8_t> buffer, uint8_t len) -> SMCom_Status_t")
        .def("assign_new_id", &py_smcom_tramp::assign_new_id,"(uint8_t id) -> void");
}


    // if(typeid(T) == typeid(SMCOM_PRIVATE)){
    //     a.def(py::init<uint16_t, uint16_t>())
    //     .def(py::init<uint8_t*, uint16_t, uint8_t*, uint16_t>())
    //     .def("write", static_cast<SMCom_Status_t (SMCom<T>::*)(uint8_t, const uint8_t*, uint8_t)>(&SMCom<T>::write))
    //     .def("respond", static_cast<SMCom_Status_t (SMCom<T>::*)(uint8_t, const uint8_t*, uint8_t)>(&SMCom<T>::respond))
    //     .def("respond", static_cast<SMCom_Status_t (SMCom<T>::*)(const T*, const uint8_t*, uint8_t)>(&SMCom<T>::respond))
    //     .def("start_write_queue", static_cast<SMCom_Status_t (SMCom<T>::*)(SMCom_message_types, uint8_t, uint8_t)>(&SMCom<T>::start_write_queue));
    // }