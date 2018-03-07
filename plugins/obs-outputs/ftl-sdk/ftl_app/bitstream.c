#include "utils.h"
#include "bitstream.h"


unsigned int _read_bits(struct bitstream_elmt_t *bs, int bits, int update);


int	bitstream_init(struct bitstream_elmt_t *bs, unsigned char *buf)
{
	bs->bit_idx = 8;
	bs->byte_idx = 0;
	bs->buf = buf;

	return 0;
}


/*
	T-REC-H.264-200503-P!!MSW-E.doc
	Section 9.1
*/
unsigned int bitstream_ue(struct bitstream_elmt_t *bs)
{
	int leading_zeros = -1;
	int b;
	unsigned int code_num;

	for(b = 0; !b; leading_zeros++)
	{
		b = _read_bits(bs, 1, 1);
	}

	code_num = (1 << leading_zeros) - 1 + _read_bits(bs, leading_zeros, 1);

	return code_num;
}

/*
	T-REC-H.264-200503-P!!MSW-E.doc
	Section 9.1.1
*/
int bitstream_se(struct bitstream_elmt_t *bs)
{
	unsigned int code_num;
	int signed_code_num;
	int sign;

	code_num = bitstream_ue(bs);

	sign = ~code_num & 0x1;

	signed_code_num = ((code_num + 1) >> 1) * -sign;

	return signed_code_num;
}

/*
	T-REC-H.264-200503-P!!MSW-E.doc
	Section 7.2
*/
unsigned int bitstream_u(struct bitstream_elmt_t *bs, int bits)
{
	unsigned int value;

	value = _read_bits(bs, bits, 1);

	return value;
}

/*
	T-REC-H.264-200503-P!!MSW-E.doc
	Section 7.2
*/
int bitstream_i(struct bitstream_elmt_t *bs, int bits)
{
	unsigned int value;

	value = _read_bits(bs, bits, 1);

	return ~value + 1;
}

unsigned int bitstream_peak(struct bitstream_elmt_t *bs, int bits)
{
	unsigned int value;

	value = _read_bits(bs, bits, 0);

	return value;	
}

int bitstream_bytealigned(struct bitstream_elmt_t *bs)
{
	return (bs->bit_idx == 8);
}

unsigned int _read_bits(struct bitstream_elmt_t *bs, int bits, int update)
{
	unsigned int val = 0;
	int byte_idx;
	int bit_idx;
	int b_cnt;
	int rshft, lshft;

	if(bits == 0)
	{
		return 0;
	}

	byte_idx = bs->byte_idx;
	bit_idx = bs->bit_idx;

	do{
		b_cnt = MIN(bit_idx, bits);

		val <<= b_cnt;

		lshft = 8 - bit_idx; 
		rshft = bit_idx - b_cnt + lshft;

		val |= ((bs->buf[byte_idx] << lshft) & 0xFF) >> rshft;

		bits -= b_cnt;

		bit_idx -= b_cnt;

		if(bit_idx == 0)
		{
			bit_idx = 8;
			byte_idx++;
		}

	}while(bits > 0);

	if(update)
	{
		bs->byte_idx = byte_idx;
		bs->bit_idx = bit_idx;
	}

	return val;
}