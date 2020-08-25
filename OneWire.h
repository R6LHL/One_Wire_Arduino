/* 
* OneWire.h
*
* Created: 20.04.2018 20:07:46
* Author: tsv1986

This a template class for working with 1-Wire bus
in template<> section you must out addresses of 
PIN, PORT, DDR registers of MCU in hex. Then you
must put number of bit of MCU port and quantity
of devices, that will be connected to the bus.

void busReset(void) -	function is pulling down bus level for 480us, that means bus reset.
						Then freeing bus for 50us and read state from bus. If the device on
						the bus pulling bus level down - that means the bus is ok.
						Interrupts are disabling and enabling if function;



*/


#ifndef __ONEWIRE_H__
#define __ONEWIRE_H__

#define _FAMILY_CODE_BYTE_NUMBER	0
#define _FAMILY_CODE_LENGHT_BYTE	1
#define _SERIAL_CODE_BYTE_NUMBER	1
#define _SERIAL_CODE_LENGHT_BYTE	6
#define _CRC_BYTE_NUMBER			7
#define _CRC_LENGHT_BYTE			1
#define _ROM_CODE_LENGHT			(_FAMILY_CODE_LENGHT_BYTE + _SERIAL_CODE_LENGHT_BYTE)
#define _ROM_CODE_LENGHT_WITH_CRC	(_ROM_CODE_LENGHT + _CRC_LENGHT_BYTE)

#define _SEARCH_ROM_COMMAND			0xf0
#define _READ_ROM_COMMAND			0x33
#define _SKIP_ROM_COMMAND			0xCC
#define _MATCH_ROM_COMMAND			0x55

#include <util/delay.h>
#include "../Mega16_dep_macros.h"

template <unsigned char T_PIN, unsigned char T_PORT, unsigned char T_DDR, unsigned char T_BIT, unsigned char T_DEV_QUAN>
class OneWire
{
//variables
public:
		inline unsigned char getID_(unsigned char device, unsigned char byte)
		{
			return device_id_buffer_[device][byte];
		}

		inline void setID_(unsigned char device, unsigned char byte_ptr, unsigned char id_byte)
		{
			device_id_buffer_[device][byte_ptr] = id_byte;
		}
		
		 enum Error_code 
		{
		 no_error		= 0,
		 no_answer		= 1,	
		 bus_level_low	= 2,
		 bus_level_high	= 3,
		};
		
		inline unsigned char getError_(void)
		{
			return error_;
		}
		
unsigned char readSlot_(void)
{
	unsigned char bit;
	
	*(volatile unsigned char*)(T_PORT) &= ~(1 << T_BIT);
	*(volatile unsigned char*)(T_DDR) |= (1 << T_BIT);  //master bus reset pulse
	_delay_us(1);
	*(volatile unsigned char*)(T_DDR) &= ~(1 << T_BIT); //master makes bus free
	_delay_us(12);
	
	bit = *(volatile unsigned char*)(T_PIN);
	bit =  bit & (1 << T_BIT);
	bit = (bit >> T_BIT);
	_delay_us(45);
	return bit;
}		

protected:

private:
		
		unsigned char error_;
		
		unsigned char data_buffer_[8];
		unsigned char conflict_bit_buffer_[_ROM_CODE_LENGHT];
		unsigned char device_id_buffer_[T_DEV_QUAN][_ROM_CODE_LENGHT_WITH_CRC];
		
//functions
public:
	OneWire()
	{
		*(volatile unsigned char*)(T_DDR) &= ~(1 << T_BIT);
		*(volatile unsigned char*)(T_PORT) &= ~(1 << T_BIT);
	}
	
	void init_memory_(void)
	{
		for (unsigned char device_counter = 0; device_counter < T_DEV_QUAN; device_counter++)
		{
			for (unsigned char byte_counter = 0; byte_counter < _ROM_CODE_LENGHT_WITH_CRC; byte_counter++)
			{
				device_id_buffer_[device_counter][byte_counter] = 0;
				data_buffer_[byte_counter] = 0;
			}
		}
	}
	
void readROM_(void)
{
	busReset_();

	if (error_ == no_answer) //check bus
	{
		return;		//bus error
	}
	else
	{
		unsigned char next_bit = 0;				//next bit to transmit
		unsigned char byte;
		
		put(_READ_ROM_COMMAND);

		for (unsigned char byte_counter = 0; byte_counter < _ROM_CODE_LENGHT_WITH_CRC; byte_counter++)
		{
			byte = 0;
			
			for (unsigned char bit_counter = 0; bit_counter < 8; bit_counter++)
			{	
				next_bit = readSlot_();
				if (next_bit == 1)
				{
					byte |= (1 << bit_counter); // saving bit to byte
				}
				else
				{
					byte &= ~(1 << bit_counter);
				}
				
			}
			device_id_buffer_[0][byte_counter] = byte; // save byte to data buffer
		}
		return;
	}
}

void skipROM_(void)
{
	busReset_();
	
	if (error_ == no_answer) //check bus
	{
		return;		//bus error
	}
	else
	{
		put(_SKIP_ROM_COMMAND);
		
		return;
	}
}

void matchROM_(unsigned char device_number)
{
	busReset_();
	
	if (error_ == no_answer) //check bus
	{
		return;		//bus error
	}
	else
	{
		put(_MATCH_ROM_COMMAND);
		
		unsigned char next_byte;
		
		for (unsigned char byte_counter = 0; byte_counter < _ROM_CODE_LENGHT_WITH_CRC; byte_counter++)
		{
			 next_byte = device_id_buffer_[device_number][byte_counter];
			 put(next_byte);
		}
		return;
	}
}

void searchROM_(void) // first search of ROMs
{
		for(unsigned char device_counter = 0; device_counter < T_DEV_QUAN; device_counter++ )
		{	
			busReset_();
			
			if (error_ == no_answer) //check bus
			{
				return;		//bus error
			}
			else
			{
				unsigned char next_bit = 0;				//next bit to transmit
				unsigned char byte;
				
				put(_SEARCH_ROM_COMMAND);								//search ROM command
			
				for (unsigned char byte_counter = 0; byte_counter < _ROM_CODE_LENGHT_WITH_CRC; byte_counter++)
				{
					byte = 0;
				
					for (unsigned char bit_counter = 0; bit_counter < 8; bit_counter++)
					{
						switch (get_Bus_code_())
						{
						case 0:				//straight_bit == 0 inverted_bit == 0
								{
									unsigned char bit_mask = (1 << bit_counter);
									
									if (((conflict_bit_buffer_[byte_counter] & bit_mask) >> bit_counter) == 0)
									{
										conflict_bit_buffer_[byte_counter] |= (1 << bit_counter);
										next_bit = 0;
									} 
									else
									{
										conflict_bit_buffer_[byte_counter] &= ~(1 << bit_counter);
										next_bit = 1;
									}
								
								break;		// bus level conflict
								}
								
						case 1:				 //straight_bit == 0 inverted_bit == 1
								{
								next_bit = 0; //devices with 0 in this position remain
								break;
								}
								
						case 2:				//straight_bit == 1 inverted_bit == 0
								{
								next_bit = 1; //devices with 0 in this position remain
								break;	
								}
						
						case 3:				//straight_bit == 1 inverted_bit == 1
								{
								error_ = bus_level_high;
								return; //bus error	
								}
						}
						
					byte |= (next_bit << bit_counter); // saving bit to byte
					bit_2_trasmit_(next_bit);	
					
					}
					device_id_buffer_[device_counter][byte_counter] = byte; // save byte to data buffer
				}
			}
		}
	return;
}

void put(unsigned char byte_to_write) //write byte to 1-wire bus. Named "put" for "iostream" compatibility
{
	unsigned char temp_bit;
	_delay_us(5);
	cli();
	for (unsigned char bit_counter = 0; bit_counter < 8; bit_counter++)
	{
		temp_bit = byte_to_write & (1 << bit_counter); // put mask
		temp_bit = (temp_bit >> bit_counter);		   // shift bit to 0-position to get 0b00000000 or 0b00000001
		if (temp_bit == 0)
		{
			write0_toSlot_();
		}
		else if (temp_bit == 1)
		{
			write1_toSlot_();
		}
	}
	sei();
	_delay_us(5);
	return;
}

unsigned char get(void) //read byte from 1-wire bus. Named "get" for "iostream" compatibility
{
	unsigned char temp_bit;
	unsigned char byte = 0;

	for (unsigned char bit_counter = 0; bit_counter < 8; bit_counter++)
	{
		temp_bit = readSlot_();				// get 0b00000000 or 0b00000001 from slot
		byte |= (temp_bit << bit_counter); //set bit to its position
	}
	
	return byte;
}

void busReset_(void)
{
	volatile unsigned char bus_state = 0x0;
	cli();
	*(volatile unsigned char*)(T_PORT) &= ~(1 << T_BIT);
	*(volatile unsigned char*)(T_DDR) |= (1 << T_BIT);  //master bus reset pulse
	_delay_us(480); //480
	*(volatile unsigned char*)(T_DDR) &= ~(1 << T_BIT); //master makes bus free
	_delay_us(50); //
	
	bus_state = (*(volatile unsigned char*)(T_PIN));		// check response from bus
	bus_state = bus_state & (1 << T_BIT);
	_delay_us(410); //
	sei();
	
	if (bus_state == 0)
	{
		error_ = no_error;
		return;
	} 
	else
	{
		error_ = no_answer;
		return;
	}
}

protected:
private:
	OneWire( const OneWire &c );
	OneWire& operator=( const OneWire &c );
	
unsigned char get_Bus_code_(void)
{
	volatile unsigned char straight_bit  = readSlot_();
	volatile unsigned char inverted_bit  = readSlot_();
	
	return (straight_bit<<1) | (inverted_bit);	//make code from bits for code optimization
}
	
void bit_2_trasmit_(unsigned char bit)
{
	switch (bit)				   // transmit next bit
	{
		case 0:
		{
			write0_toSlot_();
			break;
		}
		case 1:
		{
			write1_toSlot_();
			break;
		}
	}
}

void write0_toSlot_(void)
{
	*(volatile unsigned char*)(T_PORT) &= ~(1 << T_BIT);
	*(volatile unsigned char*)(T_DDR) |= (1 << T_BIT);  //master bus reset pulse
	_delay_us(60);
	//_delay_us(120);
	*(volatile unsigned char*)(T_DDR) &= ~(1 << T_BIT); //master makes bus free
	_delay_us(1);
}

void write1_toSlot_(void)
{
	*(volatile unsigned char*)(T_PORT) &= ~(1 << T_BIT);
	*(volatile unsigned char*)(T_DDR) |= (1 << T_BIT);  //master bus reset pulse
	//_delay_us(1);
	_delay_us(15);
	*(volatile unsigned char*)(T_DDR) &= ~(1 << T_BIT); //master makes bus free
	//_delay_us(59);
	_delay_us(45);
}

}; //OneWire

#endif //__ONEWIRE_H__
