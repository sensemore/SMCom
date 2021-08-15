#ifndef SM_PROTOCOL_SENSEWAY_TO_WIRED_H
#define SM_PROTOCOL_SENSEWAY_TO_WIRED_H
#include <stdint.h>


namespace MP_Senseway_to_Wired{

enum predefined_id:uint8_t{
	BOOTLOADER_AND_FIRMWARE_UPDATER_ID = 12,
	SENSEWAY_ID = 13,
};



enum STATUS_SENSEWAY_WIRED : uint8_t{
	ERROR = 0, 		//!< [0] If the corresponding msg_handler fails put 0 for result status as an error, maybe additional message explanation
	SUCCESS, 		//!< [1] If everything is okay handler sends 1 to indicate message is handled succesfully
	TIMEOUT,		//!< [3] If the message handler sees a timeout error send this
	DATA,			//!< [4] If we send data we will put first this result
	WRONG_MESSAGE, 	//!< [5] If incoming message is broken or data is missing
	BROKEN_PACKET,
};

enum CONSTANT_MESSAGES_SENSEWAY_WIRED : uint8_t{
	ENTER_FIRMWARE_UPDATER_MODE = 0,
	FIRMWARE_PACKET_START = 1,			//!< Application side should not get this message! only from the bootloader
	FIRMWARE_PACKET = 2,			//!< Application side should not get this message! only from the bootloader
	FIRMWARE_PACKET_END = 3,			//!< Application side should not get this message! only from the bootloader
};

//###############################[BOOTLOADER]#######################################
//If we get this then wired is entered in bootloader mode
typedef struct msg_enter_firmware_updater{
	uint8_t mac[6];
}__attribute__((packed)) msg_enter_firmware_updater;

typedef struct resp_enter_firmware_updater{
	uint8_t mac[6];
}__attribute__((packed)) resp_enter_firmware_updater;

//!< Firmware Packet messages
typedef struct msg_firmware_packet_start{
	uint32_t size;
	uint32_t no_packets;
	uint8_t mac[6];
}__attribute__((packed)) msg_firmware_packet_start;

typedef struct resp_firmware_packet_start{
	STATUS_SENSEWAY_WIRED status;
	uint8_t mac[6];
}__attribute__((packed)) resp_firmware_packet_start;

//----------------------------------------------------------------------
typedef struct msg_firmware_packet{
	uint8_t data[240];
	uint8_t mac[6];
	uint16_t packet_no;
}__attribute__((packed)) msg_firmware_packet;

//If writing packet into flash is succesful then wired returns the written packet_no to notify senseway
typedef struct resp_firmware_packet{
	STATUS_SENSEWAY_WIRED status;
	uint8_t mac[6];
	uint16_t packet_no;
}__attribute__((packed)) resp_firmware_packet;

//----------------------------------------------------------------------
typedef struct msg_firmware_packet_end{
	uint8_t mac[6];
}__attribute__((packed)) msg_firmware_packet_end;

typedef struct resp_firmware_packet_end{
	STATUS_SENSEWAY_WIRED status;
	uint8_t mac[6];
}__attribute__((packed)) resp_firmware_packet_end;
//######################################################################

//###############################[APPLICATION]#######################################
enum MESSAGES_SENSEWAY_WIRED : uint8_t{
	GET_VERSION 					= 10,
	AUTO_ADDRESSING_INIT 			= 11,
	AUTO_ADDRESSING_SET_NEW_ID 		= 12,
	START_BATCH_MEASUREMENT 		= 13,
	GET_BATCH_MEASUREMENT 			= 14,
	GET_CLEARANCE					= 15,
	GET_CREST 						= 16,
	GET_GRMS 						= 17,
	GET_KURTOSIS 					= 18,
	GET_SKEWNESS 					= 19,
	GET_BATCH_MEASUREMENT_CHUNK 	= 20,
	AUTO_ADDRESSING_INTEGRITY_CHECK = 21,
	GET_ALL_TELEMETRY 				= 22,
	GET_VRMS						= 23,
	GET_PEAK						= 24,
	GET_SUM							= 25,
};

typedef struct resp_get_version{
	uint8_t patch;
	uint8_t minor;
	uint8_t major;
}__attribute__((packed)) resp_get_version;


typedef struct msg_auto_addressing_init{
	uint8_t device_count;
	uint16_t delay_offset;
	uint16_t channel_delay;
} __attribute__((packed)) msg_auto_addressing_init;


typedef struct resp_auto_addressing_init{
	uint8_t mac[6];
	uint8_t patch;
	uint8_t minor;
	uint8_t major;
} __attribute__((packed)) resp_auto_addressing_init;

//Force this mac address to hold given id
typedef struct msg_auto_addressing_set_new_id{
	uint8_t id;
	uint8_t mac[6];
} __attribute__((packed)) msg_auto_addressing_set_new_id;


//####

typedef struct measurement_config{
	uint8_t accelerometer_range;
	uint8_t frequency;
	uint32_t sample_size;
}__attribute__((packed)) measurement_config;

typedef struct msg_start_batch_measurement{
	measurement_config mc;
	uint8_t acknowledge_senseway;
} __attribute__((packed)) msg_start_batch_measurement;

typedef struct resp_start_batch_measurement{
	STATUS_SENSEWAY_WIRED status;
} __attribute__((packed)) resp_start_batch_measurement;


enum MEASUREMENT_ERROR_CODES : uint8_t{
	NO_MEASUREMENT,
	BROKEN_MEASUREMENT,
	MEASUREMENT_TIMEOUT,
};


typedef struct resp_get_batch_measurement{
	STATUS_SENSEWAY_WIRED status;
	union content{
		struct measurement_data{
			uint8_t data_len;
			uint8_t data[0];
		}__attribute__((packed))md;
		
		struct measurement_error{
			MEASUREMENT_ERROR_CODES error;
		}__attribute__((packed))me;

		struct measurement_finished_data{
			uint32_t frequency_calibration;
			int16_t temperature;
		}__attribute__((packed)) mfd;

	}__attribute__((packed))content;
}__attribute__((packed)) resp_get_batch_measurement;


typedef struct msg_get_batch_measurement_chunk{
	uint32_t byte_offset;
	uint32_t data_len;
} __attribute__((packed)) msg_get_batch_measurement_chunk;

typedef struct resp_get_batch_measurement_chunk{
	STATUS_SENSEWAY_WIRED status;
	union content{
		struct measurement_data{
			uint8_t data_len;
			uint8_t data[0];
		}__attribute__((packed))md;

		struct measurement_error{
			MEASUREMENT_ERROR_CODES error;
		}__attribute__((packed))me;

		struct measurement_finished_data{
			uint32_t frequency_calibration;
			int16_t temperature;
		}__attribute__((packed)) mfd;

	}__attribute__((packed))content;
}__attribute__((packed)) resp_get_batch_measurement_chunk;


typedef struct resp_get_clearance{
	STATUS_SENSEWAY_WIRED status;
	double x;
	double y;
	double z;
}__attribute__((packed)) resp_get_clearance;

typedef struct resp_get_crest{
	STATUS_SENSEWAY_WIRED status;
	double x;
	double y;
	double z;
}__attribute__((packed)) resp_get_crest;

typedef struct resp_get_grms{
	STATUS_SENSEWAY_WIRED status;
	double x;
	double y;
	double z;
}__attribute__((packed)) resp_get_grms;

typedef struct resp_get_kurtosis{
	STATUS_SENSEWAY_WIRED status;
	double x;
	double y;
	double z;
}__attribute__((packed)) resp_get_kurtosis;

typedef struct resp_get_skewness{
	STATUS_SENSEWAY_WIRED status;
	double x;
	double y;
	double z;
}__attribute__((packed)) resp_get_skewness;


typedef struct msg_auto_addressing_integrity_check{
	uint8_t mac[6];
} __attribute__((packed)) msg_auto_addressing_integrity_check;


typedef struct resp_auto_addressing_integrity_check{
	uint8_t mac[6];
} __attribute__((packed)) resp_auto_addressing_integrity_check;


//There is no message for this response, just send empty

typedef struct resp_get_all_telemetry{
	STATUS_SENSEWAY_WIRED status;
	uint16_t temperature;
	uint32_t frequency_calibration;
	double clearance[3];
	double crest[3];
	double grms[3];
	double kurtosis[3];
	double skewness[3];
	//Included in v1.0.9
	double vrms[3];
	double peak[3];
	double sum[3];
	//Included in v1.0.13
	double peak_to_peak[3];
}__attribute__((packed)) resp_get_all_telemetry;

typedef struct resp_get_vrms{
	STATUS_SENSEWAY_WIRED status;
	double x;
	double y;
	double z;
}__attribute__((packed)) resp_get_vrms;

typedef struct resp_get_peak{
	STATUS_SENSEWAY_WIRED status;
	double x;
	double y;
	double z;
}__attribute__((packed)) resp_get_peak;

typedef struct resp_get_sum{
	STATUS_SENSEWAY_WIRED status;
	double x;
	double y;
	double z;
}__attribute__((packed)) resp_get_sum;


};
#endif


typedef struct ongoing_firmware_info_struct{
    FILE * bin_file; //File that contains the binary data
    
    /*
        const uint8_t * bin_array; //File that contains the binary data
        uint32_t array_iter; //Iterates in terms of bytes, no uint32_t iteration!
    */

    MP_Senseway_to_Wired::msg_enter_firmware_updater mefu;
    MP_Senseway_to_Wired::msg_firmware_packet_start mfpstart;
    MP_Senseway_to_Wired::msg_firmware_packet mfp;
    MP_Senseway_to_Wired::msg_firmware_packet_end mfpend;

    uint8_t sequence;

}ongoing_firmware_info_struct;

sm_err_t Senseway_WiredClient::device_firmware_update(const SM_MacAddress &devmac,const char* file_name){
    
    using namespace MP_Senseway_to_Wired;

    int8_t wired_id = get_wired_id(devmac);
    if(wired_id == -1) {
        return sm_err_t::NO_DEVICE;
    }

    FILE * fp = SM_File_System::get_file(file_name,"rb");
    if(fp == NULL){
        SM_LOGE("File '%s' pointer is NULL",file_name);
        return sm_err_t::FILE_SYSTEM_ERROR;
    }
    /*
        We can deduce the size of the file
    */

	fseek(fp,0,SEEK_END);
	uint32_t file_size = ftell(fp);
	rewind(fp);

    if(file_size == 0){
        SM_LOGE("Empty file %s",file_name);
        return sm_err_t::EMPTY_FILE;
    }


    SM_LOGW("Starting device firmware update, mac %s",devmac.as_string());
    
    //Do not forget the clean ongoing firmware, otherwise new firmware won't start!
	ongoing_firmware_info_struct * of = (ongoing_firmware_info_struct*) malloc(sizeof(ongoing_firmware_info_struct));
    if(of == NULL){
        return sm_err_t::ERROR;
    }



    //Sequence starts with init
    of->sequence = WIRED_FIRMWARE_SEQUENCE_INITIALIZED;
    //The file pointer that we are gonna use and read chunk by chunk
	of->bin_file = fp;
    //Total file size, file must not contain anything other than firmware binary
    of->mfpstart.size = file_size;
    /*
        This claculation is just telling wired how many packets we are gonna send through this process
        Probably it would be 240*N + half packet
    */
    of->mfpstart.no_packets = (file_size/sizeof(msg_firmware_packet::data));// + ((file_size % sizeof(msg_firmware_packet::data) != 0));
    
    SM_LOGI("Calculated total packets to send wired %lu",of->mfpstart.no_packets);

    //Start with packet number 'one'
    of->mfp.packet_no = 1;
    //Copy mac address to fields so no need to copy again and again
    memcpy(of->mefu.mac,devmac.get_native_address(),6);
    memcpy(of->mfpstart.mac,devmac.get_native_address(),6);
    memcpy(of->mfp.mac,devmac.get_native_address(),6);
    memcpy(of->mfpend.mac,devmac.get_native_address(),6);

    const TickType_t wait_time = pdMS_TO_TICKS(5000);

    auto dev = device_set.find(client_device(devmac));

    
    wired_id = MP_Senseway_to_Wired::BOOTLOADER_AND_FIRMWARE_UPDATER_ID;
    
    int retry = 0;
    while(retry < WIRED_FIRMWARE_MAX_RETRY_FOR_ONE_PACKET){
        if(of->sequence == WIRED_FIRMWARE_SEQUENCE_INITIALIZED){
            
            SM_LOGD("Sending enter firmware update msg retry[%d]",retry);
            
            rs485stw.sm_uart.set_baudrate(RS485_Senseway_to_Wired_BAUDRATE);
            
            xQueueReset(rs485stw_rx_queue);
            
            SMCom_Status_t ret = rs485stw.write(PUBLIC_ID_4BIT,
                                            MP_Senseway_to_Wired::ENTER_FIRMWARE_UPDATER_MODE,
                                            (uint8_t*)&(of->mefu),
                                            sizeof(MP_Senseway_to_Wired::msg_enter_firmware_updater));
            
            rs485stw.sm_uart.set_baudrate(RS485_Senseway_to_Wired_FIRMWARE_UPDATE_BAUDRATE);

            if(ret == SMCOM_STATUS_SUCCESS){
                SM_LOGV("Send message to wired succesfully");
                SMCOM_PUBLIC * packet = NULL;
                bool is_received = block_and_wait_wired_for_message(&packet, wired_id, MP_Senseway_to_Wired::ENTER_FIRMWARE_UPDATER_MODE, wait_time);
                if(is_received){
                    resp_enter_firmware_updater * refu = (resp_enter_firmware_updater *) packet->data;
                    //Also check the mac address
                    if(devmac == refu->mac){
                        SM_LOGI("Device entered the firmware updater mode");
                        of->sequence = WIRED_FIRMWARE_SEQUENCE_STARTED;
                        retry = 0;
                        free(packet);
                        continue;
                    }
                }
                SM_LOGI("Can't receive packet from wired");
                free(packet);
            }
            else{
                SM_LOGE("Smcom error:%d",ret);
            }
        }
        else if(of->sequence == WIRED_FIRMWARE_SEQUENCE_STARTED){
            
            xQueueReset(rs485stw_rx_queue);

            SM_LOGD("Sending firmware packet start retry[%d]",retry);
            SMCom_Status_t ret = rs485stw.write(wired_id,
                                                MP_Senseway_to_Wired::FIRMWARE_PACKET_START,
                                                (uint8_t*)&(of->mfpstart),
                                                sizeof(msg_firmware_packet_start));
            
            if(ret == SMCOM_STATUS_SUCCESS){
                SMCOM_PUBLIC * packet = NULL;
                bool is_received = block_and_wait_wired_for_message(&packet, wired_id, MP_Senseway_to_Wired::FIRMWARE_PACKET_START, wait_time);
                if(is_received){
                    resp_firmware_packet_start * rfps = (resp_firmware_packet_start *) packet->data;
                    //Also check the mac address
                    if(devmac == rfps->mac && rfps->status == MP_Senseway_to_Wired::SUCCESS){
                        of->sequence = WIRED_FIRMWARE_SEQUENCE_SENDING_PACKETS;
                        of->mfp.packet_no = 1;
                        retry = 0;
                        free(packet);
                        continue;
                    }
                }
                free(packet);
            }
        }
        else if(of->sequence == WIRED_FIRMWARE_SEQUENCE_SENDING_PACKETS){
        
            uint8_t read_len = sizeof(msg_firmware_packet::data) < file_size ? sizeof(msg_firmware_packet::data) : file_size;
            
            if(of->mfp.packet_no % 20 == 0){
                SM_LOGV("Wired Fw packet:%d",of->mfp.packet_no);
            }

            /*
                Instead of using fseek, we just read one time from file and try to send it after that we will continue to send the same data
            */
            if(retry == 0){
                fread(of->mfp.data, sizeof(uint8_t), read_len, fp);
            }

            xQueueReset(rs485stw_rx_queue);
            SMCom_Status_t ret = rs485stw.write(wired_id,
                                                MP_Senseway_to_Wired::FIRMWARE_PACKET,
                                                (uint8_t*)&(of->mfp),
                                                sizeof(msg_firmware_packet));

            if(ret == SMCOM_STATUS_SUCCESS){
                SMCOM_PUBLIC * packet = NULL;
                bool is_received = block_and_wait_wired_for_message(&packet, wired_id, MP_Senseway_to_Wired::FIRMWARE_PACKET, wait_time);
                if(is_received){
                    resp_firmware_packet * rfp = (resp_firmware_packet *) packet->data;
                    //Also check the mac address
                    if(devmac == rfp->mac && rfp->status == MP_Senseway_to_Wired::SUCCESS){
                        free(packet);
                        of->mfp.packet_no++;
                        file_size -= read_len;
                        retry = 0;
                        //A little bit of delay is good for wired internal buffer, pushing so fast is so fast for its DMA
                        vTaskDelay(5);

                        if(file_size == 0){
                            //Last packet is sent, change the state
                            SM_LOGV("Changing sequence packets to finished");
                            of->sequence = WIRED_FIRMWARE_SEQUENCE_FINISHED;
                        }
                        continue;
                    }
                    else{
                        SM_LOGW("Wired returned %u",rfp->status);
                    }
                }
                else{
                    SM_LOGE("Couldn't receive packet");
                }
                free(packet);
            }            
        }
        else if(of->sequence == WIRED_FIRMWARE_SEQUENCE_FINISHED){
            SM_LOGD("Sending firmware packet end retry[%d]",retry);
            xQueueReset(rs485stw_rx_queue);
            SMCom_Status_t ret = rs485stw.write(wired_id,
                                                MP_Senseway_to_Wired::FIRMWARE_PACKET_END,
                                                (uint8_t*)&(of->mfpend),
                                                sizeof(msg_firmware_packet_end));
            if(ret == SMCOM_STATUS_SUCCESS){
                SMCOM_PUBLIC * packet = NULL;
                bool is_received = block_and_wait_wired_for_message(&packet, wired_id, MP_Senseway_to_Wired::FIRMWARE_PACKET_END, wait_time);
                if(is_received){
                    resp_firmware_packet_end * rfpe = (resp_firmware_packet_end *) packet->data;
                    //Also check the mac address
                    if(devmac == rfpe->mac && rfpe->status == MP_Senseway_to_Wired::SUCCESS){
                        //Now we have finished succesfully, finish it
                        SM_LOGI("Firmware update for '%s' finished succesfully",devmac.as_string());
                        rs485stw.sm_uart.set_baudrate(RS485_Senseway_to_Wired_BAUDRATE);
                        free(packet);
                        free(of);
                        fclose(fp);

                        //Remove wired from the list otherwise we assume it is already in the list
                        //but device restarts itself
                        device_set.erase(dev);
                        update_shared_device_set(device_set);

                        vTaskDelay(pdMS_TO_TICKS(10000)); //Wait for wired to finish writing
                        return sm_err_t::SUCCESS;
                    }
                    else{
                        SM_LOGW("Wired returned %u",rfpe->status);
                    }
                }
                free(packet);
            }
        }
        ++retry;
    }

    if(of->sequence >= WIRED_FIRMWARE_SEQUENCE_STARTED){
        //If somehow wired entered to different sections other than APP
        device_set.erase(dev);
        update_shared_device_set(device_set);
    }

    fclose(fp);
    free(of);
    SM_LOGE("Timeout occured!");
    
    rs485stw.sm_uart.set_baudrate(RS485_Senseway_to_Wired_BAUDRATE);    
    return sm_err_t::TIMEOUT;
}
