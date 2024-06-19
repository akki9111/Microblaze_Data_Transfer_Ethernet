//DEFINE STATEMENTS TO INCREASE SPEED
#undef LWIP_TCP
#undef LWIP_DHCP
//#undef CHECKSUM_CHECK_UDP
//#undef LWIP_CHECKSUM_ON_COPY
//#undef CHECKSUM_GEN_UDP

#include <stdio.h>
#include <stdlib.h>

#include "xparameters.h"

#include "netif/xadapter.h"

#include "platform.h"
#include "platform_config.h"
#if defined (__arm__) || defined(__aarch64__)
#include "xil_printf.h"
#endif

#include "lwip/udp.h"
#include "xil_cache.h"

// Include our own definitions
#include "includes.h"

//#include "xscugic.h"
#include "xbram.h"

/* defined by each RAW mode application */
void print_app_header();
int start_application();
//int transfer_data();
void tcp_fasttmr(void);
void tcp_slowtmr(void);

/* missing declaration in lwIP */
void lwip_init();

extern volatile int TcpFastTmrFlag;
extern volatile int TcpSlowTmrFlag;
/* set up netif stuctures */
static struct netif server_netif;
struct netif *echo_netif;

// Global Variables to store results and handle data flow
char Centroid[5];
	//u32 Centroid;
	u32 *ptr;
	u32 Cen;

// Global variables for data flow
volatile u8		IndArrDone;
volatile u32		EthBytesReceived;
volatile u8		SendResults;
volatile u8   		DMA_TX_Busy;
volatile u8		Error;

// Global Variables for Ethernet handling
u16_t    		RemotePort = 8;
struct ip4_addr  	RemoteAddr;
struct udp_pcb 		send_pcb;



/////////////////////////////////////////////////
/*
void strng(char str[5], int num) {
    int i, rem, len, n;
    len = 0;
    n = num;
    while (n != 0) {
        len++;
        n /= 10;
    }
    for (i = 0; i < len; i++) {
        rem = num % 10;
        num = num / 10;
        str[len - (i + 1)] = rem + '0';
    }
    str[len] = '\0';
}

*/

void StartDMATransfer ( unsigned int dstAddress, unsigned int len ) {

	// write destination address to S2MM_DA register
	Xil_Out32 ( XPAR_AXI_DMA_0_BASEADDR + 0x48 , dstAddress );

	// write length to S2MM_LENGTH register
	Xil_Out32 ( XPAR_AXI_DMA_0_BASEADDR + 0x58 , len );

}


/////////////////////////////////////////////////////
/*
//Interrupt Controller
XScuGic InterruptController;
static XScuGic_Config *GicConfig;
u32 global_frame_counter = 0;

//int *ptr;
//Interrupt Handling

void InterruptHandler ( void ) {

	// if you have a device, which may produce several interrupts one after another, the first thing you should do is to disable interrupts, but axi dma is not this case.
	u32 tmpValue;

	// xil_printf("Interrupt acknowledged.\n\r");

	// clear interrupt. just perform a write to bit no. 12 of S2MM_DMASR
	tmpValue = Xil_In32 ( XPAR_AXI_DMA_0_BASEADDR + 0x34 );
	tmpValue = tmpValue | 0x1000;
	Xil_Out32 ( XPAR_AXI_DMA_0_BASEADDR + 0x34 , tmpValue );

	// Data is in the DRAM ! do your processing here !

	global_frame_counter++;
	if ( global_frame_counter > 10000000 ) {
		xil_printf ( "Frame number : %d \n\r", global_frame_counter );
		return;


		//xil_printf ( "Frame number : %d \n\r", global_frame_counter );
	// initiate a new data transfer
	// StartDMATransfer ( 0xa000000 + 128 * global_frame_counter, 256 );
		StartDMATransfer ( XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR , 256 );

	}

}

int SetUpInterruptSystem( XScuGic *XScuGicInstancePtr )
{
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT , (Xil_ExceptionHandler) XScuGic_InterruptHandler , XScuGicInstancePtr);
	Xil_ExceptionEnable();		// enable interrupts in ARM
	return XST_SUCCESS;
}

int InitializeInterruptSystem ( deviceID ) {
	int Status;

	GicConfig = XScuGic_LookupConfig ( deviceID );
	if ( NULL == GicConfig ) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize ( &InterruptController, GicConfig, GicConfig->CpuBaseAddress );
	if ( Status != XST_SUCCESS ) {
		return XST_FAILURE;
	}

	Status = SetUpInterruptSystem ( &InterruptController );
		if ( Status != XST_SUCCESS ) {
			return XST_FAILURE;
		}

	Status = XScuGic_Connect ( &InterruptController, XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR, (Xil_ExceptionHandler)InterruptHandler, NULL );
	if ( Status != XST_SUCCESS ) {
		return XST_FAILURE;
	}

	XScuGic_Enable (&InterruptController, XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR);

	return XST_SUCCESS;

}

////////////////////////////////////////////////////////////////
 *
 */

// Initialize AXI DMA..
int InitializeAXIDma(void) {

	unsigned int tmpVal;
	//S2MM DMACR.RS = 1
	tmpVal = Xil_In32 ( XPAR_AXI_DMA_0_BASEADDR + 0x30 );
	tmpVal = tmpVal | 0x1001;
	Xil_Out32 ( XPAR_AXI_DMA_0_BASEADDR + 0x30 , tmpVal );
	tmpVal = Xil_In32 ( XPAR_AXI_DMA_0_BASEADDR + 0x30 );
	xil_printf ( "Value for DMA control register : %x\n\r", tmpVal );

	return 0;

}

// Enable SampleGenerator
int EnableSampleGenerator( unsigned int numberOfWords) {

	// set the gpios direction as output
	// the gpio is by default output, so this is not needed

	// set the value for frame size
	Xil_Out32 ( XPAR_AXI_GPIO_0_BASEADDR, numberOfWords );

	// enable the sample generator
	Xil_Out32 ( XPAR_AXI_GPIO_1_BASEADDR, 1 );

}



////////////////////////////////////////////////



void
print_ip(char *msg, struct ip4_addr *ip)
{
	print(msg);
	xil_printf("%d.%d.%d.%d\n\r", ip4_addr1(ip), ip4_addr2(ip),
			ip4_addr3(ip), ip4_addr4(ip));
}

void
print_ip_settings(struct ip4_addr *ip, struct ip4_addr *mask, struct ip4_addr *gw)
{

	print_ip("Board IP: ", ip);
	print_ip("Netmask : ", mask);
	print_ip("Gateway : ", gw);
}

/* print_app_header: function to print a header at start time */
void print_app_header()
{
	xil_printf("\n\r\n\r------lwIP UDP GetCentroid Application------\n\r");
	xil_printf("UDP packets sent to port 7 will be processed\n\r");
}

int main()
{




	/////////////////////////////////////////////


	xil_printf("Initialize AXI DMA..\n\r");
	    InitializeAXIDma();

	    // enable sample generator
	    // end of frame will come after 128 bytes (32 words) are transferred
	    xil_printf("Setting up Sample Generator..\n\r");
	    EnableSampleGenerator(32);

	    // set the interrupt system and interrupt handling
	    // interrupt
	   // xil_printf("enabling the interrupt handling system..\n\r");
	   // InitializeInterruptSystem( XPAR_PS7_SCUGIC_0_DEVICE_ID );

	    // start the first dma transfer
	    xil_printf ("performing the first dma transfer ... press a key to begin ...\n\r");
	    getchar();
	    StartDMATransfer ( XPAR_MIG_7SERIES_0_BASEADDR, 256 );

	    //StartDMATransfer ( XPAR_MICROBLAZE_0_LOCAL_MEMORY_ILMB_BRAM_IF_CNTLR_BASEADDR, 256 );




	    //int *ptr = 0xa000008;
	    //xil_printf("Data is %0x\n\r", *ptr);


	    /*
	    int i;
	    for(i=0;i<2;i=i+1)
	    {
	    	//ptr = XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR + (4*i);
	    ptr = 0x400000F0;
	    	xil_printf("Data at address: %0x = %d\n\r", ptr, *ptr);
	    }

	    xil_printf ( "Frame number : %d \n\r", global_frame_counter );
	    xil_printf("Hello World\n\r");
	    xil_printf("Successfully ran Hello World application");

	    */





	/////////////////////////////////////////////






	struct ip4_addr ipaddr, netmask, gw /*, Remotenetmask, Remotegw*/;
	struct pbuf * psnd;
	err_t udpsenderr;
	int status = 0;

	/* the mac address of the board. this should be unique per board */
	unsigned char mac_ethernet_address[] =
	{ 0x00, 0x0a, 0x35, 0x00, 0x01, 0x10 };

	/* Use the same structure for the server and the echo server */
	echo_netif = &server_netif;
	init_platform();

	/* initialize IP addresses to be used */
	IP4_ADDR(&ipaddr,  192, 168,   1, 10);
	IP4_ADDR(&netmask, 255, 255, 255,  0);
	IP4_ADDR(&gw,      192, 168,   1,  1);

	IP4_ADDR(&RemoteAddr,  192, 168,   1, 20);
	//IP4_ADDR(&Remotenetmask, 255, 255, 255,  0);
	//IP4_ADDR(&Remotegw,      10, 0,   0,  1);

	print_app_header();

	/* Initialize the lwip for UDP */
	lwip_init();

  	/* Add network interface to the netif_list, and set it as default */
	if (!xemac_add(echo_netif, &ipaddr, &netmask,
						&gw, mac_ethernet_address,
						PLATFORM_EMAC_BASEADDR)) {
		xil_printf("Error adding N/W interface\n\r");
		return -1;
	}
	netif_set_default(echo_netif);

	/* now enable interrupts */
	platform_enable_interrupts();

	/* specify that the network if is up */
	netif_set_up(echo_netif);

	xil_printf("Zedboard IP settings: \r\n");
	print_ip_settings(&ipaddr, &netmask, &gw);
	xil_printf("Remote IP settings: \r\n");
	//print_ip_settings(&RemoteAddr, &Remotenetmask, &Remotegw);
	print_ip("Board IP: ", &RemoteAddr);

	/* start the application (web server, rxtest, txtest, etc..) */
	status = start_application();
	if (status != 0){
		xil_printf("Error in start_application() with code: %d\n\r", status);
		goto ErrorOrDone;
	}



	        int i;
		    for(i=0;i<2;i=i+1)
		    {
		    	//ptr = XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR + (4*i);
		    ptr = 0x800000F0;
		    	xil_printf("Data at address: %0x = %d\n\r", ptr, *ptr);
		    }

		    //xil_printf ( "Frame number : %d \n\r", global_frame_counter );
		    xil_printf("Hello World\n\r");
		    xil_printf("Successfully ran Hello World application");


//
		    //
		    //

		    Cen = *ptr;



		    //xil_printf("Cen = %d\n\r",Cen);
		    //
		    //


		    //Centroid = Cen;
		    //strng(Centroid[5],Cen);



		    //char result[50];
		        //float num = 23.34;
		       // sprintf(Centroid, "%d", Cen);

		   // char Centroid[10];

		    // Convert 123 to string [buf]
		    itoa(Cen, Centroid, 10);

		     //Print our string
		    xil_printf("%s\n\r", Centroid);
		    //

	/* receive and process packets */
	while (Error==0) {

		if (TcpFastTmrFlag) {
			tcp_fasttmr();
			TcpFastTmrFlag = 0;
			//SendResults = 1;
			//xil_printf("*");
		}
		if (TcpSlowTmrFlag) {
			tcp_slowtmr();
			TcpSlowTmrFlag = 0;
			SendResults = 1;
		}
		//xemacif_input(echo_netif);
		//transfer_data();

		/* Receive packets */
		xemacif_input(echo_netif);

		/* Send results back from time to time */
		if (SendResults == 1){

			SendResults = 0;
			// Read the results from the FPGA
			//int Centroid = *ptr;

			// Send out the centroid result over UDP
			psnd = pbuf_alloc(PBUF_TRANSPORT, sizeof(Centroid), PBUF_RAM);
			psnd->payload = Centroid;
			udpsenderr = udp_sendto(&send_pcb, psnd, &RemoteAddr, RemotePort);
			//xil_printf(".");

			xil_printf("Data at address: %0x = %d\n\r", ptr, *ptr);

			xil_printf("Data : %d\n\r", Cen);
		    xil_printf("%s\n\r", Centroid);



			if (udpsenderr != ERR_OK){
				xil_printf("UDP Send failed with Error %d\n\r", udpsenderr);
				goto ErrorOrDone;
			}
			pbuf_free(psnd);
		}
	}

	// Jump point for failure
ErrorOrDone:
	xil_printf("Catastrophic Error! Shutting down and exiting...\n\r");

	// Disable the GetCentroid interrupts and disconnect from the GIC
	//DisableIntrSystem(&Intc, GETCENTROID_INTR_ID, &GetCentroid, TX_INTR_ID);

	cleanup_platform();
	return 0;
}
