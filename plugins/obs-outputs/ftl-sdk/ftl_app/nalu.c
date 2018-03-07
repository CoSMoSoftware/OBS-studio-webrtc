#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bitstream.h"
#include "dec_obj.h"
#include "utils.h"
#include "nalu.h"

int _rbsp_trailing_bits(struct bitstream_elmt_t *bs);

/*
	T-REC-H.264-200503-P!!MSW-E.doc
	Section 7.3.1
*/

int nal_unit(h264_dec_obj_t *obj, struct bitstream_elmt_t *bs, struct nalu_t *nalu, int len)
{
	int rbsp_len;
	int i;
	int val;
	unsigned char *rbsp;

	rbsp = obj->nalu_buf;
	
	/*forbidden zero bit*/
	nalu->forbidden_zero_bit = bitstream_u(bs, 1);

	/*nal ref idc*/
	nalu->nal_ref_idc = bitstream_u(bs, 2);

    /*nal unit type*/
    nalu->nal_unit_type = bitstream_u(bs, 5);

	if(nalu->nal_unit_type == 14 || nalu->nal_unit_type == 20)
	{
		nalu->svc_extension_flag = bitstream_u(bs, 1);

		if(nalu->svc_extension_flag != 1)
		{
			printf("nal_unit_header_mvc_extension() parsing not supported\n");
			return -1;
		}

		nalu->idr_flag = bitstream_u(bs, 1);
		
		nalu->priority_id = bitstream_u(bs, 6);

		nalu->no_inter_layer_pred_flag = bitstream_u(bs, 1);

		nalu->dependency_id = bitstream_u(bs, 3);
		
		nalu->quality_id = bitstream_u(bs, 4);

		nalu->temporal_id = bitstream_u(bs, 3);

		nalu->use_ref_base_pic_flag = bitstream_u(bs, 1);
		
		nalu->discardable_flag = bitstream_u(bs, 1);

		nalu->output_flag = bitstream_u(bs, 1);

		nalu->reserved_three_2bits = bitstream_u(bs, 2);
	}

	rbsp_len = 0;

	for(i=1; i < len; i++)
	{
		/*check for start code emulation prevention bits*/
		if(i + 2 < len && bitstream_peak(bs, 24) == 0x000003 )
		{
			rbsp[rbsp_len++] = bitstream_u(bs, 8);
			rbsp[rbsp_len++] = bitstream_u(bs, 8);
			i+=2;

			/*read off start code emulation prevention bits*/
			bitstream_u(bs, 8);
		}
		else
		{
			rbsp[rbsp_len++] = bitstream_u(bs, 8);
		}
	}

	return rbsp_len;
}

void nalu_parse_slice_header(struct bitstream_elmt_t *bs, struct nalu_t* nalu, struct slice_header_t *slice)
{
	struct sequence_params_set_t *sps;
	struct picture_params_set_t *pps;
	int slice_skip_flag = 0;
	int i;

	memset(slice, 0, sizeof(struct slice_header_t));

	slice->first_mb_in_slice = bitstream_ue(bs);
	slice->slice_type = bitstream_ue(bs) % 5;
	slice->pic_parameter_set_id = bitstream_ue(bs);

	pps = find_pps(slice->pic_parameter_set_id);
	sps = find_sps(nalu->nal_unit_type, pps->seq_parameter_set_id);

	slice->frame_num = bitstream_u(bs, sps->log2_max_frame_num_minus4 + 4);
#if 0
	slice->field_pic_flag = 0;

	if( !sps->frame_mbs_only_flag ) 
	{
		slice->field_pic_flag = bitstream_u(bs, 1);

		if( slice->field_pic_flag )
		{
			slice->bottom_field_flag = bitstream_u(bs, 1);
		}
	}

	if( nalu->nal_unit_type  ==  NALU_TYPE_IDR_SLICE || (nalu->nal_unit_type == 20 && nalu->idr_flag)) 
	{
		slice->idr_pic_id = bitstream_ue(bs);
	}

	if( sps->pic_order_cnt_type  ==  0 ) 
	{
		slice->pic_order_cnt_lsb = bitstream_u(bs, sps->log2_max_pic_order_cnt_lsb_minus4 + 4);

		if( pps->pic_order_present_flag &&  !slice->field_pic_flag )
		{
			slice->delta_pic_order_cnt_bottom = bitstream_se(bs);
		}
	}

	if( sps->pic_order_cnt_type == 1 && !sps->delta_pic_order_always_zero_flag ) 
	{
		slice->delta_pic_order_cnt[0] = bitstream_se(bs);

		if( pps->pic_order_present_flag  &&  !slice->field_pic_flag )
		{
			slice->delta_pic_order_cnt[1] = bitstream_se(bs);
		}
	}

	if( pps->redundant_pic_cnt_present_flag )
	{
		slice->redundant_pic_cnt = bitstream_ue(bs);
	}

	if( slice->slice_type  ==  SLICE_TYPE_B )
	{
		slice->direct_spatial_mv_pred_flag = bitstream_u(bs, 1);
	}

	if( slice->slice_type == SLICE_TYPE_P || slice->slice_type == SLICE_TYPE_SP || slice->slice_type == SLICE_TYPE_B ) 
	{
		slice->num_ref_idx_active_override_flag = bitstream_u(bs, 1);

		if( slice->num_ref_idx_active_override_flag ) 
		{
			slice->num_ref_idx_l0_active_minus1 = bitstream_ue(bs);

			if( slice->slice_type  ==  SLICE_TYPE_B )
			{
				slice->num_ref_idx_l1_active_minus1 = bitstream_ue(bs);
			}
		}
	}

	/*7.3.3.1 -- Reference picture list reordering */
	if( slice->slice_type != SLICE_TYPE_I  &&  slice->slice_type != SLICE_TYPE_SI ) 
	{ 
		slice->ref_pic_reorder.ref_pic_list_reordering_flag_l0 = bitstream_u(bs, 1);

		if( slice->ref_pic_reorder.ref_pic_list_reordering_flag_l0 )
		{
			i = -1;
			
			do {
				i++;

				slice->ref_pic_reorder.list0[i].reordering_of_pic_nums_idc = bitstream_ue(bs);

				if(		slice->ref_pic_reorder.list0[i].reordering_of_pic_nums_idc == 0 
					||	slice->ref_pic_reorder.list0[i].reordering_of_pic_nums_idc == 1)
				{
					slice->ref_pic_reorder.list0[i].abs_diff_pic_num_minus1 = bitstream_ue(bs);
				}
				else if(slice->ref_pic_reorder.list0[i].reordering_of_pic_nums_idc == 2)
				{
					slice->ref_pic_reorder.list0[i].long_term_pic_num = bitstream_ue(bs);
				}
			} while( slice->ref_pic_reorder.list0[i].reordering_of_pic_nums_idc  !=  3 );
		}
	}

	if( slice->slice_type == SLICE_TYPE_B ) 
	{
		slice->ref_pic_reorder.ref_pic_list_reordering_flag_l1 = bitstream_u(bs, 1);

		if( slice->ref_pic_reorder.ref_pic_list_reordering_flag_l1 )
		{
			i = -1;
			
			do {
				i++;

				slice->ref_pic_reorder.list1[i].reordering_of_pic_nums_idc = bitstream_ue(bs);

				if(		slice->ref_pic_reorder.list1[i].reordering_of_pic_nums_idc == 0 
					||	slice->ref_pic_reorder.list1[i].reordering_of_pic_nums_idc == 1)
				{
					slice->ref_pic_reorder.list1[i].abs_diff_pic_num_minus1 = bitstream_ue(bs);
				}
				else if(slice->ref_pic_reorder.list1[i].reordering_of_pic_nums_idc == 2)
				{
					slice->ref_pic_reorder.list1[i].long_term_pic_num = bitstream_ue(bs);
				}
			} while( slice->ref_pic_reorder.list1[i].reordering_of_pic_nums_idc  !=  3 );
		}
	}
	

	if(		( pps->weighted_pred_flag  &&  ( slice->slice_type == SLICE_TYPE_P  ||  slice->slice_type == SLICE_TYPE_SP ) )  
		||	( pps->weighted_bipred_idc  ==  1  &&  slice->slice_type  ==  SLICE_TYPE_B ) 
      )
	{
		//pred_weight_table( )
		//printf("Weighted Table not supported\n");
	}

	if( nalu->nal_ref_idc != 0 )
	{
		/*7.3.3.3	Decoded reference picture marking*/
		if( nalu->nal_unit_type  ==  NALU_TYPE_IDR_SLICE || (nalu->nal_unit_type == 20 && nalu->idr_flag))  
		{
			slice->ref_pic_marking.no_output_of_prior_pics_flag = bitstream_u(bs, 1);
			slice->ref_pic_marking.long_term_reference_flag = bitstream_u(bs, 1);
		} 
		else 
		{
			slice->ref_pic_marking.adaptive_ref_pic_marking_mode_flag = bitstream_u(bs, 1);

			if( slice->ref_pic_marking.adaptive_ref_pic_marking_mode_flag )
			{
				slice->ref_pic_marking.difference_of_pic_nums_minus1 = -1;
				slice->ref_pic_marking.long_term_pic_num = -1;
				slice->ref_pic_marking.long_term_frame_idx = -1;
				slice->ref_pic_marking.max_long_term_frame_idx_plus1 = -1;

				i = -1;

				memset(slice->ref_pic_marking.memory_management_control_operation, 0, sizeof(slice->ref_pic_marking.memory_management_control_operation));

				do {
					i++;

					slice->ref_pic_marking.memory_management_control_operation[i] = bitstream_ue(bs);

					if(		slice->ref_pic_marking.memory_management_control_operation[i] == 1 
						||	slice->ref_pic_marking.memory_management_control_operation[i] == 3 )
					{
						slice->ref_pic_marking.difference_of_pic_nums_minus1 = bitstream_ue(bs);
					}

					if(slice->ref_pic_marking.memory_management_control_operation[i] == 2 )
					{
						slice->ref_pic_marking.long_term_pic_num = bitstream_ue(bs);
					}

			 		if( slice->ref_pic_marking.memory_management_control_operation[i] == 3 ||
						slice->ref_pic_marking.memory_management_control_operation[i] == 6)
					{
						slice->ref_pic_marking.long_term_frame_idx = bitstream_ue(bs);
					}

					if( slice->ref_pic_marking.memory_management_control_operation[i] == 4)
					{
						slice->ref_pic_marking.max_long_term_frame_idx_plus1 = bitstream_ue(bs);
					}

				} while( slice->ref_pic_marking.memory_management_control_operation[i] != 0 );
			}
		}

#ifdef READ_SVC
		if( sps->profile_idc == 83 || sps->profile_idc == 86 )
		{
			if(!sps->sps_svc.slice_header_restriction_flag)
			{
				slice->store_ref_base_pic_flag = bitstream_u(bs, 1);		

				if ( ( nalu->use_ref_base_pic_flag || slice->store_ref_base_pic_flag ) && !nalu->idr_flag )
				{
					int i = -1;

					//dec_ref_base_pic_marking( )
					slice->ref_base_pic_marking.adaptive_ref_base_pic_marking_mode_flag = bitstream_u(bs, 1);

					if( slice->ref_base_pic_marking.adaptive_ref_base_pic_marking_mode_flag )
					{
						do 
						{
							i++;

							slice->ref_base_pic_marking.memory_management_base_control_operation[i] = bitstream_ue(bs);

							if( slice->ref_base_pic_marking.memory_management_base_control_operation[i] == 1 )
							{
								slice->ref_base_pic_marking.difference_of_base_pic_nums_minus1 = bitstream_ue(bs);
							}
							
							if( slice->ref_base_pic_marking.memory_management_base_control_operation[i] == 2 )
							{
								slice->ref_base_pic_marking.long_term_base_pic_num = bitstream_ue(bs);
							}
						} while( slice->ref_base_pic_marking.memory_management_base_control_operation[i] != 0 );
					}
				}
			}
		}
#endif
	}

	if( pps->entropy_coding_mode_flag  &&  slice->slice_type  !=  SLICE_TYPE_I  &&  slice->slice_type  !=  SLICE_TYPE_SI )
	{
		slice->cabac_init_idc = bitstream_ue(bs);
	}

	slice->slice_qp_delta = bitstream_se(bs);

	if( slice->slice_type  ==  SLICE_TYPE_SP  ||  slice->slice_type  ==  SLICE_TYPE_SI ) 
	{
		if( slice->slice_type  ==  SLICE_TYPE_SP )
		{
			slice->sp_for_switch_flag = bitstream_u(bs, 1);
		}

		slice->slice_qs_delta = bitstream_se(bs);
	}

	if( pps->deblocking_filter_control_present_flag ) 
	{
		slice->disable_deblocking_filter_idc = bitstream_ue(bs);

		if( slice->disable_deblocking_filter_idc  !=  1 ) 
		{
			slice->slice_alpha_c0_offset_div2 = bitstream_se(bs);
			slice->slice_beta_offset_div2 = bitstream_se(bs);
		}
	}

	if(		pps->num_slice_groups_minus1 > 0  
		&&	pps->slice_group_map_type >= 3  
		&&  pps->slice_group_map_type <= 5)
	{

		int bits=0;
		int pic_size, change_rate;
		unsigned int value;

		pic_size = pps->pic_size_in_map_units_minus1 + 1;
		change_rate = pps->slice_group_change_rate_minus1 + 1;

		value = pic_size / change_rate + 1; 

		while(value != 0)
		{
			bits++;
			value >>= 1;
		}

		slice->slice_group_change_cycle = bitstream_u(bs, bits);
	}

#ifdef READ_SVC
 	if( sps->profile_idc == 83 || sps->profile_idc == 86 )
	{
		if(!sps->sps_svc.slice_header_restriction_flag && !slice_skip_flag)
		{
			slice->scan_idx_start = read_u(bs, 4);
			slice->scan_idx_end = read_u(bs, 4);
		}
	}
#endif
#endif
}


int nalu_parse_sps(struct bitstream_elmt_t *bs, struct sequence_params_set_t *sps)
{
	int i;

	sps->profile_idc = bitstream_u(bs, 8);
	sps->constraint_set0_flag = bitstream_u(bs, 1);
	sps->constraint_set1_flag = bitstream_u(bs, 1);
	sps->constraint_set2_flag = bitstream_u(bs, 1);
	sps->constraint_set3_flag = bitstream_u(bs, 1);
	sps->reserved_zero_4bits = bitstream_u(bs, 4); /* equal to 0 */
	sps->level_idc = bitstream_u(bs, 8);
	sps->seq_parameter_set_id = bitstream_ue(bs);
	sps->log2_max_frame_num_minus4 = bitstream_ue(bs);
	sps->pic_order_cnt_type = bitstream_ue(bs);

	if(sps->pic_order_cnt_type == 0)
	{
		sps->log2_max_pic_order_cnt_lsb_minus4 = bitstream_ue(bs);
	}
	else if(sps->pic_order_cnt_type == 1)
	{
		sps->delta_pic_order_always_zero_flag = bitstream_u(bs, 1);
		sps->offset_for_non_ref_pic = bitstream_se(bs);
		sps->offset_for_top_to_bottom_field = bitstream_se(bs);
		sps->num_ref_frames_in_pic_order_cnt_cycle = bitstream_ue(bs);

		sps->offset_for_ref_frame = malloc(sizeof(sps->offset_for_ref_frame[0]) * sps->num_ref_frames_in_pic_order_cnt_cycle);

		for(i=0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
		{
			sps->offset_for_ref_frame[i] = bitstream_se(bs);
		}
	}

	sps->num_ref_frames = bitstream_ue(bs);
	sps->gaps_in_frame_num_value_allowed_flag = bitstream_u(bs, 1);
	sps->pic_width_in_mbs_minus1 = bitstream_ue(bs);
	sps->pic_height_in_map_units_minus1 = bitstream_ue(bs);
	sps->frame_mbs_only_flag = bitstream_u(bs, 1);

	if(!sps->frame_mbs_only_flag)
	{
		sps->mb_adaptive_frame_field_flag = bitstream_u(bs, 1);
	}

	sps->direct_8x8_inference_flag = bitstream_u(bs, 1);
	sps->frame_cropping_flag = bitstream_u(bs, 1);

	if(sps->frame_cropping_flag)
	{
		sps->frame_crop_left_offset = bitstream_ue(bs);
		sps->frame_crop_right_offset = bitstream_ue(bs);
		sps->frame_crop_top_offset = bitstream_ue(bs);
		sps->frame_crop_bottom_offset = bitstream_ue(bs);
	}

	sps->vui_parameters_present_flag = bitstream_u(bs, 1);

	if(sps->vui_parameters_present_flag)
	{
		sps->vui.aspect_ratio_info_present_flag = bitstream_u(bs, 1);

		if( sps->vui.aspect_ratio_info_present_flag ) 
		{
			sps->vui.aspect_ratio_idc = bitstream_u(bs, 8);

			if( sps->vui.aspect_ratio_idc == VUI_SAR_EXTENDED) 
			{
				sps->vui.sar_width = bitstream_u(bs, 16);
				sps->vui.sar_height = bitstream_u(bs, 16);
			}
		}

		sps->vui.overscan_info_present_flag = bitstream_u(bs, 1);

		if( sps->vui.overscan_info_present_flag )
		{
			sps->vui.overscan_appropriate_flag = bitstream_u(bs, 1);
		}

		sps->vui.video_signal_type_present_flag = bitstream_u(bs, 1);

		if( sps->vui.video_signal_type_present_flag ) 
		{
			sps->vui.video_format = bitstream_u(bs, 3);
			sps->vui.video_full_range_flag = bitstream_u(bs, 1);
			sps->vui.colour_description_present_flag = bitstream_u(bs, 1);

			if( sps->vui.colour_description_present_flag ) 
			{
				sps->vui.colour_primaries = bitstream_u(bs, 8);
				sps->vui.transfer_characteristics = bitstream_u(bs, 8);
				sps->vui.matrix_coefficients = bitstream_u(bs, 8);
			}
		}

		sps->vui.chroma_loc_info_present_flag = bitstream_u(bs, 1);

		if( sps->vui.chroma_loc_info_present_flag ) 
		{
			sps->vui.chroma_sample_loc_type_top_field = bitstream_ue(bs);
			sps->vui.chroma_sample_loc_type_bottom_field = bitstream_ue(bs);
		}

		sps->vui.timing_info_present_flag = bitstream_u(bs, 1);

		if( sps->vui.timing_info_present_flag ) 
		{
			sps->vui.num_units_in_tick = bitstream_u(bs, 32);
			sps->vui.time_scale = bitstream_u(bs, 32);
			sps->vui.fixed_frame_rate_flag = bitstream_u(bs, 1);
		}

		sps->vui.nal_hrd_parameters_present_flag = bitstream_u(bs, 1);

		if( sps->vui.nal_hrd_parameters_present_flag )
		{
			int SchedSelIdx;

			sps->vui.nal_hrd.cpb_cnt_minus1 = bitstream_ue(bs);
			sps->vui.nal_hrd.bit_rate_scale = bitstream_u(bs, 4);
			sps->vui.nal_hrd.cpb_size_scale = bitstream_u(bs, 4);

			sps->vui.nal_hrd.bit_rate_value_minus1	= malloc(sizeof(sps->vui.nal_hrd.bit_rate_value_minus1[0]) * (sps->vui.nal_hrd.cpb_cnt_minus1 + 1));
			sps->vui.nal_hrd.cpb_size_value_minus1	= malloc(sizeof(sps->vui.nal_hrd.cpb_size_value_minus1[0]) * (sps->vui.nal_hrd.cpb_cnt_minus1 + 1));
			sps->vui.nal_hrd.cbr_flag				= malloc(sizeof(sps->vui.nal_hrd.cbr_flag[0]) * (sps->vui.nal_hrd.cpb_cnt_minus1 + 1));

			for( SchedSelIdx = 0; SchedSelIdx <= sps->vui.nal_hrd.cpb_cnt_minus1; SchedSelIdx++ ) 
			{
				sps->vui.nal_hrd.bit_rate_value_minus1[ SchedSelIdx ] = bitstream_ue(bs);
				sps->vui.nal_hrd.cpb_size_value_minus1[ SchedSelIdx ] = bitstream_ue(bs);
				sps->vui.nal_hrd.cbr_flag[ SchedSelIdx ] = bitstream_u(bs, 1);
			}

			sps->vui.nal_hrd.initial_cpb_removal_delay_length_minus1 = bitstream_u(bs, 5);
			sps->vui.nal_hrd.cpb_removal_delay_length_minus1 = bitstream_u(bs, 5);
			sps->vui.nal_hrd.dpb_output_delay_length_minus1 = bitstream_u(bs, 5);
			sps->vui.nal_hrd.time_offset_length = bitstream_u(bs, 5);
		}

		sps->vui.vcl_hrd_parameters_present_flag = bitstream_u(bs, 1);

		if( sps->vui.vcl_hrd_parameters_present_flag )
		{
			int SchedSelIdx;

			sps->vui.vlc_hrd.cpb_cnt_minus1 = bitstream_ue(bs);
			sps->vui.vlc_hrd.bit_rate_scale = bitstream_u(bs, 4);
			sps->vui.vlc_hrd.cpb_size_scale = bitstream_u(bs, 4);

			sps->vui.vlc_hrd.bit_rate_value_minus1	= malloc(sizeof(sps->vui.vlc_hrd.bit_rate_value_minus1[0]) * (sps->vui.vlc_hrd.cpb_cnt_minus1 + 1));
			sps->vui.vlc_hrd.cpb_size_value_minus1	= malloc(sizeof(sps->vui.vlc_hrd.cpb_size_value_minus1[0]) * (sps->vui.vlc_hrd.cpb_cnt_minus1 + 1));
			sps->vui.vlc_hrd.cbr_flag				= malloc(sizeof(sps->vui.vlc_hrd.cbr_flag[0]) * (sps->vui.vlc_hrd.cpb_cnt_minus1 + 1));

			for( SchedSelIdx = 0; SchedSelIdx <= sps->vui.vlc_hrd.cpb_cnt_minus1; SchedSelIdx++ ) 
			{
				sps->vui.vlc_hrd.bit_rate_value_minus1[ SchedSelIdx ] = bitstream_ue(bs);
				sps->vui.vlc_hrd.cpb_size_value_minus1[ SchedSelIdx ] = bitstream_ue(bs);
				sps->vui.vlc_hrd.cbr_flag[ SchedSelIdx ] = bitstream_u(bs, 1);
			}

			sps->vui.vlc_hrd.initial_cpb_removal_delay_length_minus1 = bitstream_u(bs, 5);
			sps->vui.vlc_hrd.cpb_removal_delay_length_minus1 = bitstream_u(bs, 5);
			sps->vui.vlc_hrd.dpb_output_delay_length_minus1 = bitstream_u(bs, 5);
			sps->vui.vlc_hrd.time_offset_length = bitstream_u(bs, 5);
		}

		if( sps->vui.nal_hrd_parameters_present_flag  ||  sps->vui.vcl_hrd_parameters_present_flag )
		{
			sps->vui.low_delay_hrd_flag = bitstream_u(bs, 1);
		}

		sps->vui.pic_struct_present_flag = bitstream_u(bs, 1); 
		sps->vui.bitstream_restriction_flag = bitstream_u(bs, 1);

		if( sps->vui.bitstream_restriction_flag ) 
		{
			sps->vui.motion_vectors_over_pic_boundaries_flag = bitstream_u(bs, 1);
			sps->vui.max_bytes_per_pic_denom = bitstream_ue(bs);
			sps->vui.max_bits_per_mb_denom = bitstream_ue(bs);
			sps->vui.log2_max_mv_length_horizontal = bitstream_ue(bs);
			sps->vui.log2_max_mv_length_vertical = bitstream_ue(bs);
			sps->vui.num_reorder_frames = bitstream_ue(bs);
			sps->vui.max_dec_frame_buffering = bitstream_ue(bs);
		}
	}

	_rbsp_trailing_bits(bs);

	return 0;
}

int nalu_parse_pps(struct bitstream_elmt_t *bs, struct picture_params_set_t *pps)
{
	pps->pic_parameter_set_id = bitstream_ue(bs);
	pps->seq_parameter_set_id = bitstream_ue(bs);
	pps->entropy_coding_mode_flag = bitstream_u(bs, 1);
	pps->pic_order_present_flag = bitstream_u(bs, 1);
	pps->num_slice_groups_minus1 = bitstream_ue(bs);

	if( pps->num_slice_groups_minus1 > 0 ) 
	{
		pps->slice_group_map_type = bitstream_ue(bs);

		if( pps->slice_group_map_type  ==  0 )
		{
			int iGroup;

			pps->run_length_minus1 = malloc( (pps->num_slice_groups_minus1 + 1) * sizeof(pps->run_length_minus1[0]));

			for( iGroup = 0; iGroup <= pps->num_slice_groups_minus1; iGroup++ )
			{
				pps->run_length_minus1[iGroup] = bitstream_ue(bs);
			}
		}
		else if( pps->slice_group_map_type  ==  2 )
		{
			int iGroup;

			pps->top_left = malloc( (pps->num_slice_groups_minus1 + 1) * sizeof(pps->top_left[0]));
			pps->bottom_right = malloc( (pps->num_slice_groups_minus1 + 1) * sizeof(pps->bottom_right[0]));

			for( iGroup = 0; iGroup < pps->num_slice_groups_minus1; iGroup++ ) 
			{
				pps->top_left[iGroup] = bitstream_ue(bs);
				pps->bottom_right[iGroup] = bitstream_ue(bs);
			}
		}
		else if(  pps->slice_group_map_type  ==  3  ||  
				  pps->slice_group_map_type  ==  4  ||  
				  pps->slice_group_map_type  ==  5 ) 
		{
			pps->slice_group_change_direction_flag = bitstream_u(bs, 1);
			pps->slice_group_change_rate_minus1 = bitstream_ue(bs);
		} 
		else if( pps->slice_group_map_type  ==  6 ) 
		{
			int i;
			int bits=0;
			int pic_size;


			pps->pic_size_in_map_units_minus1 = bitstream_ue(bs);

			pps->slice_group_id = malloc( (pps->pic_size_in_map_units_minus1 + 1) * sizeof(pps->slice_group_id[0]));

			pic_size = pps->pic_size_in_map_units_minus1 + 1;

			while(pic_size != 0)
			{
				bits++;
				pic_size >>= 1;
			}


			for( i = 0; i <= pps->pic_size_in_map_units_minus1; i++ )
			{
				pps->slice_group_id[i] = bitstream_u(bs, bits);
			}
		}
	}

	pps->num_ref_idx_l0_active_minus1 = bitstream_ue(bs);
	pps->num_ref_idx_l1_active_minus1 = bitstream_ue(bs);
	pps->weighted_pred_flag = bitstream_u(bs, 1);
	pps->weighted_bipred_idc = bitstream_u(bs, 2);
	pps->pic_init_qp_minus26 = bitstream_se(bs);  /* relative to 26 */
	pps->pic_init_qs_minus26 = bitstream_se(bs);  /* relative to 26 */
	pps->chroma_qp_index_offset = bitstream_se(bs);
	pps->deblocking_filter_control_present_flag = bitstream_u(bs, 1);
	pps->constrained_intra_pred_flag = bitstream_u(bs, 1);
	pps->redundant_pic_cnt_present_flag = bitstream_u(bs, 1);

#if 0
	if( more_rbsp_data( ) ) 
	{
		pps->transform_8x8_mode_flag
		pps->pic_scaling_matrix_present_flag
		if( pps->pic_scaling_matrix_present_flag )
		{
			for( i = 0; i < 6 + 2 * pps->transform_8x8_mode_flag; i++ ) 
			{
				pps->pic_scaling_list_present_flag[i]
				if( pps->pic_scaling_list_present_flag[i] )
				{
					if( i < 6 ) 
					{
						scaling_list( ScalingList4x4[ i ], 16, 
										   UseDefaultScalingMatrix4x4Flag[ i ] )
					}
					else
					{
						scaling_list( ScalingList8x8[ i - 6 ], 64,
										   UseDefaultScalingMatrix8x8Flag[ i - 6 ] )
					}
				}
			}
		}

		second_chroma_qp_index_offset
	}
#endif

	_rbsp_trailing_bits(bs);
	
	return 0;
}



int _rbsp_trailing_bits(struct bitstream_elmt_t *bs)
{
	int bit;

	if( (bit = bitstream_u(bs, 1)) != 1)/*stop bit -- should be 1*/
	{
		return -1;
	}

	while(!bitstream_bytealigned(bs))
	{
		if( (bit = bitstream_u(bs, 1)) != 0)/*alignment bit -- should be 0*/
		{
			return -1;
		}
	}

	return 0;
}

int nalu_read(FILE *fp, unsigned char *buf)
{
	/*buffer in 100 bytes then start parsing*/
	unsigned int psc = 0, byte = 0;
	int psc_found = 0;
	int i;
	int nalu_len;
	int bytes_in_buffer;

	struct bitstream_elmt_t bs;
	int leading_zero_byte_cnt = 0;

#if 1
	while( (fread(&byte, 1, 1, fp)) == 1)
	{
		if(byte == 1)
		{
			if(leading_zero_byte_cnt >= 2)
			{
				psc_found = 1;
				break;
			}
		}
		
		if(byte == 0)
			leading_zero_byte_cnt++;
		else
			leading_zero_byte_cnt = 0;
	}

	if(!psc_found)
		return -1;

	leading_zero_byte_cnt = 0;
	psc_found = 0;
	nalu_len = 0;
	bytes_in_buffer = 0;
	while( (fread(&byte, 1, 1, fp)) == 1)
	{
		if(byte == 1)
		{
			if(leading_zero_byte_cnt >= 2)
			{
				/*backup 3 bytes so next run sees the start code*/
				psc_found = 1;
				fseek(fp, -3, SEEK_CUR);
				bytes_in_buffer -= 3;
				break;
			}
		}
		
		if(byte == 0)
			leading_zero_byte_cnt++;
		else
			leading_zero_byte_cnt = 0;

		buf[bytes_in_buffer] = byte;
		bytes_in_buffer++;
	}

	if(leading_zero_byte_cnt > 3)
	{
		printf("Found packet with too many leading zero's in start code");
	}

	if(!psc_found)
	{
		printf("End of stream\n");
	}

	nalu_len = bytes_in_buffer;
#endif

#if 0

	fread(&psc, 1, 3, fp);

	if(psc != 0x000001)
	{
		psc = psc << 8;

		fread(&byte, 1, 1, fp);

		psc = (psc & 0xFFFFFF00) | (byte & 0xFF);

		if(psc != 0x00000001)
		{
			/*rewind stream*/
			fseek(fp, -4, SEEK_CUR);
			
			return -1;
		}
	}

	bitstream_init(&bs, buf);
	nalu_len = 0;
	bytes_in_buffer = 0;
	do
	{
		bytes_in_buffer += fread(buf, 1, NALU_READ_PRELOAD, fp);

		for(i=0; i < bytes_in_buffer-3; i++)
		{
			if(bitstream_peak(&bs, 24) == 1 || bitstream_peak(&bs, 32) == 1)
			{
				psc_found = 1;
				break;
			}

			bitstream_u(&bs, 8);
			nalu_len++;
		}

		bytes_in_buffer -= i;
		buf += NALU_READ_PRELOAD;
	}while(!psc_found);

	fseek(fp, -bytes_in_buffer, SEEK_CUR);
#endif

#if 0

	fread(&psc, 1, 4, fp);

	fread(buf, 1, 8, fp);

	nalu_len = 8;
#endif

	return nalu_len;
}
