# CPP Examples 

There are three examples that can be used with SMCom

To compile and run codes just type

```bash
make && ./output.out
```


1. Public node

   Imitates a public communication and assumes channel is built with queues
   Instead of writing to port it writes data to its queue

2. Private node

   Imitates a private communication and assumes channel is built with queues
   Instead of writing to port it writes data to its queue

3. Custom node

	Imitates a custom communication and assumes channel is built with queues
	In the header file the below struct is defined, some fields are required by SMCom, others are just to show as an example
	`PACK` and `PACKET_DATA_LEN` is defined in `SMCom.h`

```cpp
PACK(struct custom_packet{
	uint8_t data_len;		 //Required by SMCom		
	uint8_t message_type:2;  //Required by SMCom
	uint8_t message_id:6;	 //Required by SMCom		
	uint16_t receiver_id;	 //Custom definition		
	uint16_t transmitter_id; //Custom definition		
	uint32_t random_number;	 //Custom definition		
	uint8_t data[PACKET_DATA_LEN]; //Required by SMCom		
});
typedef struct custom_packet CUSTOM_PACKET;

```