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