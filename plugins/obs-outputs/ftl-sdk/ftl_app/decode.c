#include <stdlib.h>
#include <string.h>

#include "decode.h"
#include "nalu.h"
#include "utils.h"


h264_dec_obj_t * H264_Decode_Open()
{
	h264_dec_obj_t *obj;

	if( (obj = malloc(sizeof(h264_dec_obj_t))) == NULL)
	{
		return NULL;
	}

	memset(obj, 0, sizeof(h264_dec_obj_t));

	
	if ((obj->nalu_buf = malloc(10000000)) == NULL)
	{
		return NULL;
	}

	return obj;
}

int H264_Decode_Nalu(h264_dec_obj_t* h264_dec_obj, unsigned char *nalu, int len)
{
	static int last_mba = -1, last_frame_num = -1;

	bitstream_init(&h264_dec_obj->bs, nalu);

	/*get the nalu type and remove picture start code emulation prevention bits*/
	len = nal_unit(h264_dec_obj, &h264_dec_obj->bs, &h264_dec_obj->nalu, len);

	bitstream_init(&h264_dec_obj->bs, h264_dec_obj->nalu_buf);

	switch(h264_dec_obj->nalu.nal_unit_type)
	{
	case NALU_TYPE_NON_IDR_SLICE:
	case NALU_TYPE_IDR_SLICE:
		nalu_parse_slice_header(&h264_dec_obj->bs, &h264_dec_obj->nalu,  &h264_dec_obj->slice);
		break;
	case NALU_TYPE_SPS:
		{
			struct sequence_params_set_t sps;

			nalu_parse_sps(&h264_dec_obj->bs, &sps);

			store_sps(&sps);
			break;
		}
	case NALU_TYPE_PPS:
		{
			struct picture_params_set_t pps;

			nalu_parse_pps(&h264_dec_obj->bs, &pps);

			store_pps(&pps);
			break;
		}
	default:
		printf("Unknown nal unit type: %d\n", h264_dec_obj->nalu.nal_unit_type);
	}

	if(h264_dec_obj->nalu.nal_unit_type == NALU_TYPE_NON_IDR_SLICE || h264_dec_obj->nalu.nal_unit_type == NALU_TYPE_IDR_SLICE )
	{
		if(h264_dec_obj->slice.first_mb_in_slice == 0)
		{
			//printf("Frame %d\n",  h264_dec_obj->slice.frame_num);
		}

		if(last_mba == -1)
		{
			last_mba = h264_dec_obj->slice.first_mb_in_slice;
			last_frame_num = h264_dec_obj->slice.frame_num;
		}
		else if(last_frame_num == h264_dec_obj->slice.frame_num)
		{
			if(last_mba >= h264_dec_obj->slice.first_mb_in_slice)
				printf("Error: frame %d: current mba is %d, last was %d\n", h264_dec_obj->slice.frame_num, h264_dec_obj->slice.first_mb_in_slice, last_mba);
		}

		last_mba = h264_dec_obj->slice.first_mb_in_slice;
		last_frame_num = h264_dec_obj->slice.frame_num;
	}

	
	return 0;
}

int H264_Decode_Close()
{

	return 0;
}