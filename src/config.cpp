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

static uint8_t default_txbuffer[1024] = {0};
static uint8_t default_rxbuffer[1024] = {0};
class py_smcom_tramp : public SMCom<SMCOM_PUBLIC>{
    
    
    public:

    // py_smcom_tramp(uint16_t rx_buf_size, uint16_t tx_buf_size, uint8_t id) : SMCom::SMCom(rx_buf_size, tx_buf_size, id, NULL, NULL){;}
	explicit py_smcom_tramp(uint8_t id) : SMCom::SMCom(default_rxbuffer, 1024, default_txbuffer, 1024, id, NULL, NULL){
    }
    ~py_smcom_tramp(){};

    virtual void __rx_callback__(SMCom_event_types event, SMCom_Status_t status, pySMCOM_PUBLIC packet) = 0;
    virtual void __tx_callback__(SMCom_event_types event, SMCom_Status_t status, pySMCOM_PUBLIC packet) = 0;
    virtual SMCom_Status_t __write__(std::vector<uint8_t> buffer, uint16_t len) = 0;
    virtual SMCom_Pair __read__(uint16_t len) = 0;



    SMCom_Status_t write(uint8_t receiver_id, uint8_t message_id, std::vector<uint8_t> buffer, uint8_t len){
        // uint8_t buf[len];
        // for(int i = 0; i < len; i++){
        //     buf[i] = buffer.at(i);
        // }
        //Buffer.data yazarak kopyalamadan passlenebilir
        return SMCom<SMCOM_PUBLIC>::write(receiver_id, message_id, buffer.data(), len);
    }

    SMCom_Status_t handle_message_data(std::vector<uint8_t> buffer, uint16_t len){
        uint8_t buf[len];
        for(int i = 0; i < len; i++){
            buf[i] = buffer.at(i);
        }
        return SMCom<SMCOM_PUBLIC>::handle_message_data(buffer.data(), len);
    }

    SMCom_Status_t push_to_queue(std::vector<uint8_t> buffer, uint8_t len){
        uint8_t buf[len];
        for(int i = 0; i < len; i++){
            buf[i] = buffer.at(i);
        }
        return SMCom<SMCOM_PUBLIC>::push_to_queue(buffer.data(), len);
    }

    SMCom_Status_t respond(uint8_t receiver_id, uint8_t message_id, std::vector<uint8_t> buffer, uint8_t len){
        uint8_t buf[len];
        for(int i = 0; i < len; i++){
            buf[i] = buffer.at(i);
        }
        return SMCom<SMCOM_PUBLIC>::respond(receiver_id, message_id, buf, len);
    }

    SMCom_Status_t __write__(const uint8_t * buffer, uint16_t len) override {
        if(buffer != NULL){
            std::vector<uint8_t> vec(buffer,buffer+len); //Bu constructor yapısı daha güzel
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

    //Bunun yukarıya çıkmasına gerek yok
    // pySMCOM_PUBLIC duplicate_message_packet(pySMCOM_PUBLIC packet){
    //     uint16_t sz = sizeof(pySMCOM_PUBLIC) + packet.data_len;
    //     pySMCOM_PUBLIC * new_packet = (pySMCOM_PUBLIC *) malloc(sz);
    //     if(new_packet != NULL){
    //         memcpy(new_packet, &packet, sz);
    //     }
    //     return new_packet;
    // }

    uint8_t get_packet_data_length(pySMCOM_PUBLIC pypacket){
        return pypacket.data_len;
    }

    SMCom_Status_t write_public(uint8_t message_id, std::vector<uint8_t> buffer, uint8_t len){
        uint8_t buf[len];
        for(int i = 0; i < len; i++){
            buf[i] = buffer.at(i);
        }
        return SMCom<SMCOM_PUBLIC>::write_public(message_id, buf, len);
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

template<typename T>
void declareSMCom(py::module &m, const std::string& typestr) {
    std::string pyclass_name = typestr;
    using Class = py_smcom_tramp;
    auto a = py::class_<Class, PySMCOM>(m, pyclass_name.c_str())
    #ifdef SMCOM_CONFIG_REQUEST_RESPONSE
	.def("request_response_callback", Class::request_response_callback)
	#endif
    // .def("verify_message_header", static_cast<SMCom_Status_t (const uint8_t*, uint16_t*)>(&Class::verify_message_header)) //len parameter is uint16_t* is it true?
    .def("handle_message_data", static_cast<SMCom_Status_t (Class::*)(std::vector<uint8_t>, uint16_t)>(&Class::handle_message_data))
    .def("__rx_callback__", static_cast<void (Class::*)(SMCom_event_types, SMCom_Status_t, pySMCOM_PUBLIC)>(&Class::__rx_callback__))
    .def("__tx_callback__", static_cast<void (Class::*)(SMCom_event_types, SMCom_Status_t, pySMCOM_PUBLIC)>(&Class::__tx_callback__))
    #ifdef SMCOM_CONFIG_REQUEST_RESPONSE
    .def("request", static_cast<SMCom_Status_t (Class::*)(uint8_t, const uint8_t*, uint8_t, uint32_t, Class::request_response_callback)>(&Class::request))
    .def("request", static_cast<SMCom_Status_t (Class::*)(uint8_t, uint8_t, const uint8_t*, uint8_t, uint32_t, Class::request_response_callback)>(&Class::request))
    .def("ping", static_cast<SMCom_Status_t (Class::*)(uint8_t, uint32_t, Class::request_response_callback)>(&Class::ping))
    .def("ping", static_cast<SMCom_Status_t (Class::*)(uint32_t, Class::request_response_callback)>(&Class::ping))
    .def("get_version", static_cast<SMCom_Status_t (Class::*)(uint8_t, uint32_t, Class::request_response_callback)>(&Class::get_version))
    .def("get_version", static_cast<SMCom_Status_t (Class::*)(uint32_t, Class::request_response_callback)>(&Class::get_version))
    #endif
    .def("push_to_queue", static_cast<SMCom_Status_t (Class::*)(std::vector<uint8_t>, uint8_t)>(&Class::push_to_queue))
    .def("finalize_queue", &Class::finalize_queue)
    .def("set_fixed_packet_size", &Class::set_fixed_packet_size)
    .def("GET_SMCOM_VERSION", &Class::GET_SMCOM_VERSION)
    .def("get_packet_data_length", static_cast<uint8_t (Class::*)(pySMCOM_PUBLIC)>(&Class::get_packet_data_length))
    //.def("duplicate_message_packet", static_cast<pySMCOM_PUBLIC (Class::*)(pySMCOM_PUBLIC)>(&Class::duplicate_message_packet))
    .def("listener", &Class::listener)
    .def("is_packet_broken", &Class::is_packet_broken)
    .def("is_crc_failed", &Class::is_crc_failed)
    #ifdef SMCOM_CONFIG_REQUEST_RESPONSE
    .def("increase_ms_timer", &Class::increase_ms_timer)
    .def("run_request_scheduler", &Class::run_request_scheduler)
    #endif
    .def("get_rx_buffer_size", &Class::get_rx_buffer_size)
    .def("__write__", static_cast<SMCom_Status_t (Class::*)(std::vector<uint8_t>, uint16_t)>(&Class::__write__))
    .def("__read__", static_cast<SMCom_Pair (Class::*)(uint16_t)>(&Class::__read__))
    .def("__available__", &Class::__available__)
    .def_static("resolve_status", &Class::resolve_status)
    .def_property_readonly_static("HEADER_SIZE", [](py::object) { return Class::HEADER_SIZE;})
    .def_property_readonly_static("MAX_MSG_LENGTH", [](py::object) { return Class::MAX_MSG_LENGTH;});

    if(typeid(T) == typeid(SMCOM_PUBLIC)){
        // a.def(py::init<uint16_t, uint16_t, uint8_t>(),
        //                                 "(uint16_t rx_buf_size, uint16_t tx_buf_size, uint8_t id) -> SMCOM_PUBLIC")
        a.def(py::init<uint8_t>(), 
                                        "(std::vector<uint8_t> rx_buffer, uint16_t rx_buf_size, std::vector<uint8_t> tx_buffer, uint16_t tx_buf_size, uint8_t id) -> SMCOM_PUBLIC")
        .def("write", static_cast<SMCom_Status_t (Class::*)(uint8_t, uint8_t, std::vector<uint8_t>, uint8_t)>(&Class::write), 
                                        "(uint8_t receiver_id, uint8_t message_id, std::vector<uint8_t> buffer, uint8_t len) -> SMCom_Status_t")
        .def("respond", static_cast<SMCom_Status_t (Class::*)(uint8_t, uint8_t, std::vector<uint8_t>, uint8_t)>(&Class::respond),
                                        "(uint8_t receiver_id, uint8_t message_id, std::vector<uint8_t> buffer, uint8_t len) -> SMCom_Status_t")
        .def("start_write_queue", static_cast<SMCom_Status_t (Class::*)(SMCom_message_types, uint8_t, uint8_t, uint8_t)>(&Class::start_write_queue),
                                        "(SMCom_message_types t, uint8_t receiver_id, uint8_t message_id, uint8_t len) -> SMCom_Status_T")
        .def("write_public", static_cast<SMCom_Status_t (Class::*)(uint8_t, std::vector<uint8_t>, uint8_t)>(&Class::write_public),
                                        "(uint8_t message_id, std::vector<uint8_t> buffer, uint8_t len) -> SMCom_Status_t")
        .def("assign_new_id", &Class::assign_new_id,
                                        "(uint8_t id) -> void");
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

    declareSMCom<SMCOM_PUBLIC>(m, "SMCOM_PUBLIC");

}


    // if(typeid(T) == typeid(SMCOM_PRIVATE)){
    //     a.def(py::init<uint16_t, uint16_t>())
    //     .def(py::init<uint8_t*, uint16_t, uint8_t*, uint16_t>())
    //     .def("write", static_cast<SMCom_Status_t (SMCom<T>::*)(uint8_t, const uint8_t*, uint8_t)>(&SMCom<T>::write))
    //     .def("respond", static_cast<SMCom_Status_t (SMCom<T>::*)(uint8_t, const uint8_t*, uint8_t)>(&SMCom<T>::respond))
    //     .def("respond", static_cast<SMCom_Status_t (SMCom<T>::*)(const T*, const uint8_t*, uint8_t)>(&SMCom<T>::respond))
    //     .def("start_write_queue", static_cast<SMCom_Status_t (SMCom<T>::*)(SMCom_message_types, uint8_t, uint8_t)>(&SMCom<T>::start_write_queue));
    // }