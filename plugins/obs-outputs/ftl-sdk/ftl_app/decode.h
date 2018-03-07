#include "dec_obj.h"

h264_dec_obj_t* H264_Decode_Open();
int H264_Decode_Nalu(h264_dec_obj_t* h264_dec_obj, unsigned char *nalu, int len);
