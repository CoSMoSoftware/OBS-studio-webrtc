#include <stdio.h>

#define NALU_READ_PRELOAD 100

int nal_unit(h264_dec_obj_t *obj, struct bitstream_elmt_t *bs, struct nalu_t *nalu, int len);
int nalu_read(FILE *fp, unsigned char *buf);
int nalu_parse_sps(struct bitstream_elmt_t *bs, struct sequence_params_set_t *sps);
int nalu_parse_pps(struct bitstream_elmt_t *bs, struct picture_params_set_t *pps);
void nalu_parse_slice_header(struct bitstream_elmt_t *bs, struct nalu_t *nalu, struct slice_header_t *slice);