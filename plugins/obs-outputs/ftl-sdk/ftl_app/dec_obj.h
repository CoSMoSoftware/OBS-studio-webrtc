#include "bitstream.h"

enum nalu_type_codes
{
	NALU_TYPE_NON_IDR_SLICE = 1,
	NALU_TYPE_IDR_SLICE		= 5,
	NALU_TYPE_SPS			= 7,
	NALU_TYPE_PPS			= 8
};

enum slice_types
{
	SLICE_TYPE_P = 0,
	SLICE_TYPE_B = 1,
	SLICE_TYPE_I = 2,
	SLICE_TYPE_SP = 3,
	SLICE_TYPE_SI = 4
};

enum sample_aspect_ratio_indicator
{
	VUI_SAR_EXTENDED = 255
};

struct nalu_t
{
	int forbidden_zero_bit;
	int nal_ref_idc;
	int nal_unit_type;
	int svc_extension_flag;
	int idr_flag;
	int priority_id;
	int no_inter_layer_pred_flag;
	int dependency_id;
	int quality_id;
	int temporal_id;
	int use_ref_base_pic_flag;
	int discardable_flag;
	int output_flag;
	int reserved_three_2bits;
};

struct hrd_params_t
{
	int cpb_cnt_minus1;
	int bit_rate_scale;
	int cpb_size_scale;
	int *bit_rate_value_minus1;
	int *cpb_size_value_minus1;
	int *cbr_flag;
	int initial_cpb_removal_delay_length_minus1;
	int cpb_removal_delay_length_minus1;
	int dpb_output_delay_length_minus1;
	int time_offset_length;
};

struct vui_params_t
{
	int aspect_ratio_info_present_flag;
	int aspect_ratio_idc;
	int sar_width;
	int sar_height;
	int overscan_info_present_flag;
	int overscan_appropriate_flag;
	int video_signal_type_present_flag;
	int video_format;
	int video_full_range_flag;
	int colour_description_present_flag;
	int colour_primaries;
	int transfer_characteristics;
	int matrix_coefficients;
	int chroma_loc_info_present_flag;
	int chroma_sample_loc_type_top_field;
	int chroma_sample_loc_type_bottom_field;
	int timing_info_present_flag;
	int num_units_in_tick;
	int time_scale;
	int fixed_frame_rate_flag;
	int nal_hrd_parameters_present_flag;
	struct hrd_params_t nal_hrd;
	int vcl_hrd_parameters_present_flag;
	struct hrd_params_t vlc_hrd;
	int low_delay_hrd_flag;
	int pic_struct_present_flag;
	int bitstream_restriction_flag;
	int motion_vectors_over_pic_boundaries_flag;
	int max_bytes_per_pic_denom;
	int	max_bits_per_mb_denom;
	int log2_max_mv_length_horizontal;
	int log2_max_mv_length_vertical;
	int num_reorder_frames;
	int max_dec_frame_buffering;
};

struct sequence_params_set_t
{
	int profile_idc;
	int constraint_set0_flag;
	int constraint_set1_flag;
	int constraint_set2_flag;
	int constraint_set3_flag;
	int reserved_zero_4bits; /* equal to 0 */
	int level_idc;
	int seq_parameter_set_id;
	int log2_max_frame_num_minus4;
	int pic_order_cnt_type;
	int log2_max_pic_order_cnt_lsb_minus4;
	int delta_pic_order_always_zero_flag;
	int offset_for_non_ref_pic;
	int offset_for_top_to_bottom_field;
	int num_ref_frames_in_pic_order_cnt_cycle;
	int *offset_for_ref_frame;
	int num_ref_frames;
	int gaps_in_frame_num_value_allowed_flag;
	int pic_width_in_mbs_minus1;
	int pic_height_in_map_units_minus1;
	int frame_mbs_only_flag;
	int mb_adaptive_frame_field_flag;
	int direct_8x8_inference_flag;
	int frame_cropping_flag;
	int frame_crop_left_offset;
	int frame_crop_right_offset;
	int frame_crop_top_offset;
	int frame_crop_bottom_offset;
	int vui_parameters_present_flag;
	struct vui_params_t vui;

	struct sequence_params_set_t *next;
};

struct picture_params_set_t
{
	int pic_parameter_set_id;
	int seq_parameter_set_id;
	int entropy_coding_mode_flag;
	int pic_order_present_flag;
	int num_slice_groups_minus1;
	int slice_group_map_type;
	int *run_length_minus1;
	int *top_left;
	int *bottom_right;
	int slice_group_change_direction_flag;
	int slice_group_change_rate_minus1;
	int pic_size_in_map_units_minus1;
	int *slice_group_id;
	int num_ref_idx_l0_active_minus1;
	int num_ref_idx_l1_active_minus1;
	int weighted_pred_flag;
	int weighted_bipred_idc;
	int pic_init_qp_minus26;  /* relative to 26 */
	int pic_init_qs_minus26;  /* relative to 26 */
	int chroma_qp_index_offset;
	int deblocking_filter_control_present_flag;
	int constrained_intra_pred_flag;
	int redundant_pic_cnt_present_flag;
	int transform_8x8_mode_flag;
	int pic_scaling_matrix_present_flag;
	int *pic_scaling_list_present_flag;
	int second_chroma_qp_index_offset;
	
	struct picture_params_set_t *next;
};

struct ref_pic_list_t
{
	int reordering_of_pic_nums_idc;
	int abs_diff_pic_num_minus1;
	int long_term_pic_num;
};

struct ref_pic_reorder_t
{
	int ref_pic_list_reordering_flag_l0;
	struct ref_pic_list_t list0[4];
	int ref_pic_list_reordering_flag_l1;
	struct ref_pic_list_t list1[4];
};

struct ref_pic_marking_t
{
	int no_output_of_prior_pics_flag;
	int long_term_reference_flag;
	int adaptive_ref_pic_marking_mode_flag;
	int memory_management_control_operation[4];
	int difference_of_pic_nums_minus1;
	int long_term_pic_num;
	int long_term_frame_idx;
	int max_long_term_frame_idx_plus1;
};

struct ref_base_pic_marking_t
{
	int adaptive_ref_base_pic_marking_mode_flag;
	int memory_management_base_control_operation[4];
	int difference_of_base_pic_nums_minus1;
	int long_term_base_pic_num;
};

struct slice_header_t
{
	int first_mb_in_slice;
	int slice_type;
	int pic_parameter_set_id;
	int frame_num;
	int field_pic_flag;
	int bottom_field_flag;
	int idr_pic_id;
	int pic_order_cnt_lsb;
	int delta_pic_order_cnt_bottom;
	int delta_pic_order_cnt[2];
	int redundant_pic_cnt;
	int direct_spatial_mv_pred_flag;
	int num_ref_idx_active_override_flag;
	int num_ref_idx_l0_active_minus1;
	int num_ref_idx_l1_active_minus1;

	struct ref_pic_reorder_t ref_pic_reorder;

	struct ref_pic_marking_t ref_pic_marking;

	struct ref_base_pic_marking_t ref_base_pic_marking;

	int cabac_init_idc;
	int slice_qp_delta;
	int sp_for_switch_flag;
	int slice_qs_delta;
	int disable_deblocking_filter_idc;
	int slice_alpha_c0_offset_div2;
	int slice_beta_offset_div2;
	int slice_group_change_cycle;

	/*svc specific headers*/
	int store_ref_base_pic_flag;
	int slice_skip_flag;
	int scan_idx_start;
	int scan_idx_end;
};

typedef struct _h264_dec_obj_t
{
	struct bitstream_elmt_t bs;
	unsigned char *nalu_buf;
	struct nalu_t nalu;	
	struct sequence_params_set_t *sps;
	struct picture_params_set_t *pps;
	struct slice_header_t slice;
}h264_dec_obj_t;