/************************************************************************/
/*                          TWI SLAVE PROGRAM                           */
/************************************************************************/
/************************************************************************\
* This is a TWI  Slave  program  written  for the  XMEGA A  A3BU  XPLAIN *
* The program  is designed to work with  the TWI  Master program written *
* for the XMEGA A A1U PRO XPLAIN board. PC0 and PC1 should  be connected *
* together  from one board to another. When run, the  program  will wait *
* for an interrupt to  receive data from the master. Upon  reception the *
* slave program should  receive the data  and prepare a  packet with the *
* received data incremented by 1. The master program should then request *
* a read operation where the slave sends the updated data. Optimal break *
* points have been marked with comments in the program.					 *
\************************************************************************/
/************************************************************************\
* To include from ASF:                                                   *
*	% Default (Generic Board Support, GPIO, IOPORT						 *
*	% System Clock Control (service)									 *
*	% TWI - Two-wire Interface (driver) both							 *
\************************************************************************/

#include <asf.h>

#define TWI_SLAVE_ADDR 0x60
#define TWI_SPEED 5000
#define PATTERN_TEST_LENGTH sizeof(test_pattern)
// Define the TWI to be used
#define TWI_SLAVE TWIC

// Initialise a test pattern
volatile uint8_t test_pattern[] = {
	0x55,
	0xA5,
/*	0x5A,
	0x77,
	0x99*/
};

// Initialise buffers for receiving and transmitting Data
volatile uint8_t rx_pattern[TWIS_SEND_BUFFER_SIZE] = {0x55,0,0,0,0,0,0,0};
volatile uint8_t m_rx_pattern[TWIS_SEND_BUFFER_SIZE] = {0};

// Initialise a slave structure (note case sensitivity)
TWI_Slave_t slave;

// Setup a user flag, its use will become clear later on
volatile Bool tx_in_progress_f = false;

// Setup a slave process, the process is accessed each time a byte is received
static void slave_process(void)
{
	tx_in_progress_f = true;
	//slave.sendData[0] = (i >> 8);
	//slave.sendData[1] = (i) & 0xff;
}

static void my_callback(void)
{
	// User code to execute when the overflow occurs here
	//Important to clear Interrupt Flag
	tc_clear_overflow(&TCC0);
}

// The ISR is accessed each time a byte is received
ISR(TWIC_TWIS_vect)
{
	TWI_SlaveInterruptHandler(&slave);
}

void twi_slave_init(	TWI_Slave_t *twi, TWI_t *module,void (*processDataFunction) (void),uint8_t address,
TWI_SLAVE_INTLVL_t intLevel) {
	uint8_t i=0;
	
	/*initialise the slave module by setting:
		slave structure to define the different data patterns and status, 
		module to serve as a slave i.e TWIx {C,D,E,F}, 
		slave call function i.e. specify a process for slave, 
		slave address (has to be compliant with master),
		slave module interrupt level
	*/

	// Initialise Slave
	for (i=0;i<TWIS_SEND_BUFFER_SIZE;i++)
	{
		twi->receivedData[i] = 0;
		//twi->sendData[i] - Used to specify the send data when a master reads
	}

	sysclk_enable_peripheral_clock(module);	//enable the TWI module
	TWI_SlaveInitializeDriver(twi, module, processDataFunction);	//I2C driver
	TWI_SlaveInitializeModule(twi,address,intLevel);	//set slave address and interrupt level
}

int main (void)
{
	sysclk_init();
	board_init();
	
	pmic_init();
	
	//cpu_irq_enable();
	

	uint16_t i=0;
	uint8_t storeA = 0;
	uint8_t	 storeB = 0;

	irq_initialize_vectors();
	
	twi_slave_init(	&slave,
	&TWI_SLAVE,
	*slave_process,
	TWI_SLAVE_ADDR,
	TWI_SLAVE_INTLVL_MED_gc);
	
	//T/C module at portD set to function as a timer/counter
	tc_enable(&TCD0);
	tc_set_overflow_interrupt_callback(&TCD0, my_callback);
	tc_set_wgm(&TCD0, TC_WG_NORMAL);
	tc_write_period(&TCD0, 1000);
	tc_set_overflow_interrupt_level(&TCD0, TC_INT_LVL_LO);

	cpu_irq_enable();
	tc_write_clock_source(&TCD0, TC_CLKSEL_DIV1_gc);
	
	/*slave.sendData[0] = 0x55;
	slave.sendData[1] = 0xA5;
	slave.sendData[2] = 0x5A;
	slave.sendData[3] = 0x77;
	slave.sendData[4] = 0x99;*/

	//slave.sendData = 
	while (1) {
		//Test case use of Counter to set the sendData buffer of the slave
		i = TCD0_CNT;
		slave.sendData[0] = (i >> 8);
		slave.sendData[1] = (i) & 0xff; 

		// Check transmission in progress flag
		if(tx_in_progress_f)
		{
			storeA = slave.sendData[0];
			storeB = slave.sendData[1];

			// prepare packet to be transmitted
			twi_package_t packet = {
				//.addr = 
				.addr_length=0,				// no commands will be sent
				.chip	=TWI_SLAVE_ADDR,
				.buffer	=(void*)m_rx_pattern,
				.length	=PATTERN_TEST_LENGTH,
				.no_wait=false				// if busy wait
			};

			LED_Toggle(LED0);				// toggle LED
			// Reset Flag
			/*** Break Point ***/
			tx_in_progress_f = false;
		}
	}
}