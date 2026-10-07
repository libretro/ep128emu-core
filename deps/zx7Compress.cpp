#include "zx7Compress.h"
namespace TVC256 {

uint32_t input_data;
uint32_t output_data;     // Ha NULL, akkor csak a kicsomagolt méretet adja vissza a Decompress
int input_index;
int output_index;
int input_size;
int output_size;
int bit_mask;
int bit_value;
int decompressSize;

int read_byte(void);
int read_bit(void);
int read_elias_gamma(void);
int read_offset();
void write_byte(int value);
void write_bytes(int offset, int length);

int read_byte(void)
{
	return emuMem->readRaw(input_index++);
	//return input_data[input_index++];
}

int  read_bit(void)
{
	bit_mask >>= 1;
	if (bit_mask == 0)
	{
		bit_mask = 128;
		bit_value = read_byte();
	}
	return bit_value & bit_mask ? 1 : 0;
}

int read_elias_gamma(void)
{
	int i, value;

	i = 0;
	while (!read_bit()) i++;

	if (i > 15) return -1;

	value = 1;
	while (i--)
	{
		value = value << 1 | read_bit();
	}
	return value;
}

int  read_offset()
{
	int value,i;

	value = read_byte();
	if (value < 128)
	{
		return value;
	}
	else
	{
		i = read_bit();
		i = i << 1 | read_bit();
		i = i << 1 | read_bit();
		i = i << 1 | read_bit();
		return (value & 127 | i << 7) + 128;
	}
}

void write_byte(int value)
{
	++decompressSize;
/*	if(output_data)
		output_data[output_index++] = value;*/
  emuMem->writeRaw(output_data+output_index++,value);
}

void write_bytes(int offset, int length)
{
	int i;

	while (length-- > 0)
	{
		i = output_index - offset;
		//write_byte(output_data[i >= 0 ? i : BUFFER_SIZE+i]);
		//write_byte((int)output_data[i]);
    write_byte((int)emuMem->readRaw(input_data+i));
	}
}

int decompress(uint32_t src, uint32_t dest)
{
	int length;

	input_data = src;
	output_data = dest;

	input_size = 0;
	input_index = 0;
	output_index = 0;
	output_size = 0;
	bit_mask = 0;

	decompressSize = 0;

	write_byte(read_byte());
	while (1)
	{
		if (!read_bit())
		{
			write_byte(read_byte());
		}
		else
		{
			length = read_elias_gamma()+1;
			if (length == 0)
				return decompressSize;

			write_bytes(read_offset()+1, length);
		}
	}

	return decompressSize;
}
}