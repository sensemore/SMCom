
// //like rs485, rx-tx are connected to multiple devices. Buffer contains rx id, tx id 4bits which can only support 15 devices, 0 for public
// // adds 7-bytes to message packet including start byte, end byte crc etc.
// PACK(struct smcom_public_4bit_adr{
// 	 data_len;			//<! Data len in the packet, user can use this instead defining data length in the message max data length is 255
// 	 receiver_id:4;		//<! Receiver id in the communication can take value between 0-13 (14 and 15 is reserved), located at 0b0000xxxx
// 	 transmitter_id:4;	//<! Transmitter id in the communication can take value between 0-13 (14 and 15 is reserved), located at 0bxxxx0000
// 	 message_type:2;		//<! Message type , can be write/request/response/indicate etc. User does not change this max value is 3, located at 0b000000xx
// 	 message_id:6;		//<! Message id is defined by the user similar to uuid in BLE communication. Max value is 63, located at 0bxxxxxx00
// 	 data[PACKET_DATA_LEN];			//<! Data pointer for derived classes
// });
// typedef struct smcom_public_4bit_adr SMCOM_PUBLIC;


function bin(x){
	return (x>>>0).toString(2);
}

class smcom_public_4bit_adr{
	constructor(){
		this.data_len = 0;
		this.receiver_id = 0;
		this.transmitter_id = 0;
		this.message_type = 0;
		this.message_id = 0;
	}
}




class SMCom{
	//Polynomial x^16 + x^15 + x^2 + 1
	POLY_IBM = 0x8005; //'0b1000000000000101'
	CRC_IBM_SEED = 0xffff;

	constructor(){
		console.log("smcom Created!")
	}

	compute_crc_ibm(crc,  data){
		for (let i = 0; i < 8; ++i){
			let b = ((crc & 0x8000) >> 8);
			crc <<= 1; // shift left once
			if( (b^(data & 0x80)) != 0){
				crc ^= this.POLY_IBM; //xor with polynomial
			}
			data<<=1; // shift data to get next bit
		}
		return crc;
	}
	get_crc_ibm(buffer, len, crc){
		for (let i = 0; i < len; ++i){
			crc = this.compute_crc_ibm(crc,buffer[i]);
		}
		return crc;
	}
}


packet = new smcom_public_4bit_adr;
packet.data_len = 5;

smcom = new SMCom();
//console.log("Hello there",packet,bin(5))
//console.log(smcom.compute_crc_ibm(smcom.CRC_IBM_SEED,0xAB))

x = new Uint8Array([1,2,3,4])
console.log(smcom.get_crc_ibm(x,x.length,smcom.CRC_IBM_SEED))