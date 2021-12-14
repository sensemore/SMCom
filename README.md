# SMCom

SMCom is a communication protocol can work on data link-layer such as UART, SPI or any wireless link-layer communications.
This repository provides different implementation in C++ and Python.

SMCom provides node-to-node communication (public-addressable) or private communication from one device to another (private-no address). Users can implement their own communication frame if SMCom's native frames are not adequate.

SMCom requires functions to handle communication channel, which are write & read functions and two callbacks, for user to define.

**SMCom :**
- Easy to implement for many devices (Communication between embedded devices or embedded to computers such as ESP32, Arduino boards, Raspberry pi etc.)
- Low memory budget (User can define their internal buffers)
- CRC check on messages so data loss can be detected and recovered
- Provides cross platform communication mainly for embedded devices

## API for Python

We provide pip package to download easily on Linux and Windows platforms.

```bash
pip install SMComPy
```

To see a python example: [SMWiredPy](https://github.com/sensemore/SMWiredPy)

## API for C++

In order to implement SMCom in C++ projects, see /test folder how to add libraries into Makefile.

SMCom class is a template class implementation which allows to create custom communication frames or provides common scenarios. Two common scenarios is implemented and can be inherited to a new communication class, see an example below.

In order to use the library, a new class must be defined which is inherited from `SMCom<SMCOM_PUBLIC>` or `SMCom<SMCOM_PRIVATE>` types, or can be defined with desired fields.

### SMCom frames


1. SMCOM_PUBLIC FRAME
	

```cpp
//SMCOM_PUBLIC frame
  1B    1B        4bit       4bit    6bit    2bit       1B     1B    1B
┌─────┬──────┬───────────┬────────┬───────┬───────┬────┬─────┬─────┬─────┐
│Start│Data  │Transmitter│Receiver│Message│Message│Data│CRC-H│CRC-L│End  │
│Byte │Length│    ID     │   ID   │ Index │  Type │    │     │     │Byte │
└─────┴──────┴───────────┴────────┴───────┴───────┴────┴─────┴─────┴─────┘
 0xFB  0-255     0-13       0-13    0-60     ..     ..   ..     ..   0xBF
```
  * Start byte and end byte are both fixed
  * Data length is 1 byte so up to 255 bytes can be send in one SMCom packet
  * Address field
    - SMCOM_PUBLIC uses 4-bit addresses for receiver and transmitters. In 4-bit address scheme (available 2^4 = 16  communication nodes) 
  Two addresses are predefined and cannot be used, so up to 14 node-to-node communication SMCOM_PUBLIC can be used.

    - **0x00-0x0D** user defined
    - **0x0E** is used for default address (reserved)
    - **0x0F** is used for public address (reserved)

  * Message index is 6-bit and some of them are reserved for internal SMCom communications, 60 message id is allowed for users

See [public_node](./examples/cpp/public_node.cpp) for more examples and sample codes



2. SMCOM_PRIVATE FRAME
   
SMCOM_PRIVATE frame is the same as above but without transmitter and receiver address fields.
See [private_node](./examples/cpp/private_node.cpp) for more examples and sample codes
```cpp
//SMCOM_PRIVATE
  1B    1B      6bit    2bit       1B     1B    1B
┌─────┬──────┬───────┬───────┬────┬─────┬─────┬─────┐
│Start│Data  │Message│Message│Data│CRC-H│CRC-L│End  │
│Byte │Length│ Index │  Type │    │     │     │Byte │
└─────┴──────┴───────┴───────┴────┴─────┴─────┴─────┘
 0xFB  0-255   0-60    ..      ..   ..     ..   0xBF
```

3. CUSTOM FRAME

Since SMCom is a template library, it supports custom headers.
Users can put whatever they want in the frame and can send their own packets and configurations etc.
You may put random numbers, preambles or addresses etc. In the below picture some fields are requisite, other than custom header field, since SMCom needs to handle messages, data lengths etc.
```cpp
//Custom packet definition
  1B    1B      6bit    2bit                              1B     1B    1B
┌─────┬──────┬───────┬───────┬──────────────────────┬────┬─────┬─────┬─────┐
│Start│Data  │Message│Message│  Custom header field │Data│CRC-H│CRC-L│End  │
│Byte │Length│ Index │  Type │    (user defined)    │    │     │     │Byte │
└─────┴──────┴───────┴───────┴──────────────────────┴────┴─────┴─────┴─────┘
  0xFB  0-255   0-60   ...                            ..   ..     ..   0xBF

```

See [custom_node](./examples/cpp/custom_node.cpp) for more examples and sample codes



Example class declaration
```cpp
class my_device : public SMCom<SMCOM_PUBLIC>{
public:
    my_device(uint16_t rx_buffer_size, uint16_t tx_buffer_size, uint8_t id);
    SMCom_Status_t __write__(const uint8_t * buffer, uint16_t len);
	SMCom_Status_t __read__(uint8_t * buffer, uint16_t len);
	size_t __available__();
    void __tx_callback__(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PUBLIC * packet);
    void __rx_callback__(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PUBLIC * packet);
};
```

Now my_device became a communication class and my_device objects can be used to communication over the channel

```cpp
my_device my_device_object(1024,1024,0x0B); //Now the current device address is 0x0B
my_device_object.write(0x0A,buffer,len) //To send a message 0x0A
my_device_object.listener() //Can be called inside a while(1) loop or can be called after a byte received event if any
```

### SMCom public functions

- Constructor family
```cpp

//Constructors with dynamic buffers
//available only for SMCOM_PRIVATE
SMCom(uint16_t rx_buf_size, uint16_t tx_buf_size, rx_event_handler_callback rx, tx_event_handler_callback tx);

//available only for SMCOM_PUBLIC
SMCom(uint16_t rx_buf_size, uint16_t tx_buf_size, uint8_t id, rx_event_handler_callback rx, tx_event_handler_callback tx);

//Constructors with static buffers
//available only for SMCOM_PRIVATE
SMCom(uint8_t * rx_buffer, uint16_t rx_buf_size, uint8_t * tx_buffer, uint16_t tx_buf_size, rx_event_handler_callback rx, tx_event_handler_callback tx);

//available only for SMCOM_PUBLIC
SMCom(uint8_t * rx_buffer, uint16_t rx_buf_size, uint8_t * tx_buffer, uint16_t tx_buf_size, uint8_t id, rx_event_handler_callback rx, tx_event_handler_callback tx);

```

- `write` function family
```cpp
//Used for private communications! Receiver address cannot be provided
SMCom_Status_t write(uint8_t message_id, const uint8_t * buffer, uint8_t len, uint8_t retry = 1); 

//Used for public communications
SMCom_Status_t write(uint8_t receiver_id, uint8_t message_id, const uint8_t * buffer, uint8_t len, uint8_t retry = 1);

//Used for public communications, all devices can receive this message
SMCom_Status_t write_public(uint8_t message_id, const uint8_t * buffer, uint8_t len, uint8_t retry = 1);
```

- `listener` function to listen the network
This function can be called inside an infinite loop or a task/thread. When listener is called, listener provides necessary checks for SMCom frames and if message is on the line it will call `__rx_callback__` function to notify the user.
```cpp
	SMCom_Status_t listener(void);
```

_In order to call `listener`, user must provide `__read__` and `__available__` functions_

### SMCom class overrideable functions

SMCom requires some main functions to write to actual port, or read from the port etc. In order SMCom to know this user must provide some of the functions definition and must call the communicaton channles write/read functions inside these definitions. Thus, when SMCom tries to write to port it will call its own function which is defined by the user and the actual port write function will be called as desired.

These functions are specifically defined in two underscores such as `__FunctionName___`.

- `__write__`, this function writes the desired data to actual communication channel.
  **SMCom requires child class to override this function!**
  For example if SMCom is used over UART, in user's new defined `__write__` function serial.write function must be called.
```cpp
SMCom_Status_t __write__(const uint8_t * buffer, uint16_t len);
```
- `__read__`, this function reads data from the actual channel. User must provide the defined of this function and must call channel's own read function such as Serial.read
```cpp
SMCom_Status_t __read__(uint8_t * buffer, uint16_t len);
```

- `__available__`, some communication channels provide available bytes in their queue, buffer or DMA etc. This can be overrided for 

```cpp
size_t __available__();
```

_Note : `__write__`, `__read__` and `__available__` definition is mandatory!, SMCom `listener` function requires read and available functions so if `listener` will be used, the definition of `__read__` and `__available__` are necessary to implement._

- `__rx_callback__`, when bytes are read and message is decoded internal library calls this function to notify user there is a message on the network. The incoming message can be accessed via 
`const SMCOM_PUBLIC * packet` variable.
```cpp
void __rx_callback__(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PUBLIC * packet);
```

- `__tx_callback__`, when bytes are written to the port and the message is coded, internal library calls this function to notify user desired message is put on the network. (Optional to define)

```cpp
void __tx_callback__(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PUBLIC * packet);
```



#### Example definitions :

A Serial library in different languages are good example to provide and see how to define such functions. Assuming the communication channel is UART so we need to call Serial library's write,read and available functions inside our definitions.

```cpp
SMCom_Status_t __write__(const uint8_t * buffer, uint16_t len){
	//We write to the serial
	if(Serial.write(buffer, len) == len){
		return SMCom_Status_t::SUCCESS;
	}
	return return SMCom_Status_t::ERROR;
}
```
```cpp
SMCom_Status_t __read__(const uint8_t * buffer, uint16_t len){
	//We read from the serial
	if(Serial.readBytes(buffer, len) == len){
		return SMCom_Status_t::SUCCESS;
	}
	return return SMCom_Status_t::ERROR;
}
```

```cpp
size_t __available__(){
	return Serial.available();
}
```
```cpp
void __rx_callback__(SMCom_event_types event, SMCom_Status_t status, const SMCOM_PUBLIC * packet){
    printf("Packet length %d\n",packet->data_len);
	printf("Message Id:[%d] invoked! Transmitter id[%d], Receiver id[%d]\n",packet->message_id,packet->transmitter_id,packet->receiver_id);
	uint8_t * data = packet->data;
    switch(packet->message_id){
        /*
			Handle different message id's here
		*/
    }
}
```
SMCom is developed by [Sensemore](www.sensemore.io)