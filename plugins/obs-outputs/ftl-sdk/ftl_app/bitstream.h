#ifndef _BITSTREAM_H_
#define _BITSTREAM_H_

struct bitstream_elmt_t
{
	unsigned char *buf;
	int byte_idx;
	int bit_idx;
};

int				bitstream_init(struct bitstream_elmt_t *bs, unsigned char *buf);


unsigned int	bitstream_ue(struct bitstream_elmt_t *bs);
int				bitstream_se(struct bitstream_elmt_t *bs);
unsigned int	bitstream_u(struct bitstream_elmt_t *bs, int bits);
int				bitstream_i(struct bitstream_elmt_t *bs, int bits);
unsigned int    bitstream_peak(struct bitstream_elmt_t *bs, int bits);
int				bitstream_bytealigned(struct bitstream_elmt_t *bs);

#endif