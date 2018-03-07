#include "decode.h"

#define MAX_OGG_PAGE_LEN 100000


typedef struct {
  FILE *fp;
  uint8_t *page_buf;
  int page_len;
  int consumed;
  int raw_opus;
  uint8_t version;
  uint8_t header_type;
  uint8_t seg_length;
  uint8_t page_segs;
  uint64_t granule_pos;
  uint32_t bs_serial;
  uint32_t page_sn;
  uint32_t checksum;
  uint8_t seg_len_table[255];
  uint8_t current_segment;
  uint8_t packets_in_page;
}opus_obj_t;

typedef struct _nalu_item_t {
	uint8_t *buf;
	int len;
	struct slice_header_t slice;
	struct nalu_t nalu;
}nalu_item_t;

typedef struct {
  FILE *fp;
  h264_dec_obj_t *h264_handle;
  nalu_item_t *curr_nalu;
  nalu_item_t *next_nalu;
}h264_obj_t;


int init_video(h264_obj_t *handle, const char *video_file);
int reset_video(h264_obj_t *handle);
int get_video_frame(h264_obj_t *handle, uint8_t *buf, uint32_t *length, int *end_of_frame);
int init_audio(opus_obj_t *handle, const char *audio_file, int raw_opus);
void close_audio(opus_obj_t *handle);
int reset_audio(opus_obj_t *handle);
int get_audio_packet(opus_obj_t *handle, uint8_t *buf, uint32_t *length);

