#include<stdio.h>
#include "public_node.h"
#include "private_node.h"
#include "custom_node.h"


int main(int argc, char const *argv[]){
	(void)argc; //Unused params
	(void)argv; //Unused params

	//Public nodes, 
	public_node pubn0(0x00,"node-0");
	public_node pubn1(0x01,"node-1");
	public_node pubn2(0x02,"node-2");
	public_node pubn3(0x03,"node-3");
	public_node pubn4(0x04,"node-4");
	
	

	//private nodes
	private_node priv_node_A("Node-A");
	private_node priv_node_B("Node-B");


	//custom nodes
	custom_node custom_node_A(100,"Custom Node-A");
	custom_node custom_node_B(200,"Custom Node-B");
	custom_node custom_node_C(300,"Custom Node-C");
	custom_node custom_node_D(400,"Custom Node-D");

	uint8_t dummy_buf[] = {0,5,10,15,20,25,30};
	custom_node_A.write(200,0,dummy_buf,sizeof(dummy_buf),1);
	

	return 0;
}