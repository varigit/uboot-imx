/*
 * Copyright (C) 2015 Variscite Ltd. All Rights Reserved.
 * Maintainer: Ron Donio <ron.d@variscite.com>
 * Configuration settings for the Variscite VAR_SOM_MX6 and DART6UL board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <common.h>
#include <i2c.h>

#include "mx6var_v2_eeprom.h"

static u32 get_address_by_index(unsigned char index);
static u32 get_value_by_index(unsigned char index);
static int handle_one_command(struct eeprom_command_type *eeprom_commands,int command_num);
 
//DDR Register struct
//The eeprom contains structure of 
// 1 byte index in this table, 1byte index to common values in the next tableto write to this address.
// if there is some new addresses got from the calibration program they should be added in the end of the array.
// The maximum array size is 256 addresses.
static const u32 rom_addresses[]=
	{
		#include "addresses.inc"
	};
				
static const u32 rom_values[] = 
	{
		#include "values.inc"
	};

static u32 ram_addresses[MAXIMUM_RAM_ADDRESSES] __attribute__ ((section ("sram")));
static u32 ram_values[MAXIMUM_RAM_VALUES] __attribute__ ((section ("sram")));
				
void load_custom_data(u32 *custom_addresses_values)
{
	int i;
	
	for(i=0;i<MAXIMUM_RAM_ADDRESSES;i++)
	{
		ram_addresses[i]=0;
		ram_values[i]=0;
	}
	
	int j=0;
	for(i=0;i<32;i++)
	{
		if(custom_addresses_values[i]==0)
			break;
		ram_addresses[j]=custom_addresses_values[i];
		j++;
	}
	
	i++;
	if(i>sizeof(custom_addresses_values))
		return;
	
	j=0;
	for(;i<32;i++)
	{
		if(custom_addresses_values[i]==0)
			break;
		ram_values[j]=custom_addresses_values[i];
		j++;
	}
}
	
	
	
int handle_eeprom_data(struct var_eeprom_config_struct_v2_type *var_eeprom_config_struct_v2)
{
	load_custom_data(var_eeprom_config_struct_v2->custom_addresses_values);

	return setup_ddr_parameters(var_eeprom_config_struct_v2->eeprom_commands);
}

int setup_ddr_parameters(struct eeprom_command_type *eeprom_commands)
{
	int i=0;
	
	while(i<MAXIMUM_COMMANDS_NUMBER)
	{
		i=handle_one_command(eeprom_commands, i);
		if(i<0)
			return -1;
		if(i==0)
			return 0;
	}
	return 0;
}


static int handle_one_command(struct eeprom_command_type *eeprom_commands,int command_num)
{
	volatile u32 *data;
	u32 address;
	u32 value;
#if 0
	printf("Executing command %03d: %03d,  %03d\n",
													command_num,
													eeprom_commands[command_num].address_index,
													eeprom_commands[command_num].value_index);
#endif
	
	switch(eeprom_commands[command_num].address_index)
	{
		case WHILE_NOT_EQUAL_INDEX:
			command_num++;
			address=get_address_by_index(eeprom_commands[command_num].address_index);
			value=get_value_by_index(eeprom_commands[command_num].value_index);
			data=(u32*)address;
			//printf("waiting while data at address %08x is not equal %08x\n",address,value);
			while(data[0]!=value);
			
			command_num++;
			break;
		case WHILE_EQUAL_INDEX:
			command_num++;
			address=get_address_by_index(eeprom_commands[command_num].address_index);
			value=get_value_by_index(eeprom_commands[command_num].value_index);
			data=(u32*)address;
			//printf("waiting while data at address %08x is equal %08x\n",address,value);
			while(data[0]==value);
			
			command_num++;
			break;
		case WHILE_AND_INDEX:
			command_num++;
			address=get_address_by_index(eeprom_commands[command_num].address_index);
			value=get_value_by_index(eeprom_commands[command_num].value_index);
			data=(u32*)address;
			//printf("waiting while data at address %08x and %08x is not zero\n",address,value);
			while(data[0]&value);
			
			command_num++;
			break;
		case WHILE_NOT_AND_INDEX:
			command_num++;
			address=get_address_by_index(eeprom_commands[command_num].address_index);
			value=get_value_by_index(eeprom_commands[command_num].value_index);
			data=(u32*)address;
			//printf("waiting while data at address %08x and %08x is zero\n",address,value);
			while(!(data[0]&value));
			
			command_num++;
			break;
		case DELAY_10USEC_INDEX:
			//Delay for Value * 10 uSeconds
			//printf("Delaying for %d microseconds\n",eeprom_commands[command_num].value_index*10);
			p_udelay((int)(eeprom_commands[command_num].value_index*10));
			command_num++;
			break;
		case LAST_COMMAND_INDEX:
			command_num=0;
			break;
		default:
			address=get_address_by_index(eeprom_commands[command_num].address_index);
			value=get_value_by_index(eeprom_commands[command_num].value_index);
			data=(u32*)address;
			//printf("Setting data at address %08x to %08x\n",address,value);
			data[0]=value;
			command_num++;
			break;
	}
	
	return command_num;
}

static u32 get_address_by_index(unsigned char index)
{
	if(index>=MAXIMUM_ROM_ADDR_INDEX)
		return ram_addresses[index-MAXIMUM_ROM_ADDR_INDEX];

	return rom_addresses[index];
}
	
static u32 get_value_by_index(unsigned char index)
{
	if(index>=MAXIMUM_ROM_VALUE_INDEX)
		return ram_values[index-MAXIMUM_ROM_VALUE_INDEX];

	return rom_values[index];
}	
	
int var_eeprom_v2_read_struct(struct var_eeprom_config_struct_v2_type *var_eeprom_config_struct_v2,unsigned char address)
{
	int eeprom_found;
	int ret = 0;
	int oldbus;
	
	oldbus = i2c_get_bus_num();
	i2c_set_bus_num(1);

	eeprom_found = i2c_probe(address);
	if (0 == eeprom_found)
	{
		if (i2c_read(address,0,1,(void*)var_eeprom_config_struct_v2,sizeof(struct var_eeprom_config_struct_v2_type)))
		{
			printf("Read device ID error!\n");
			return -1;
		} 
	} 
	else 
		printf("Error! Couldn't find EEPROM device\n");
	
	i2c_set_bus_num(oldbus);
	return ret;
}

	
	
	
	
