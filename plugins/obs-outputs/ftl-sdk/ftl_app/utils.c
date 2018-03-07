#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bitstream.h"
#include "dec_obj.h"
#include "utils.h"

struct picture_params_set_t *pps_head = NULL;
struct sequence_params_set_t *sps_head = NULL;

void store_sps(struct sequence_params_set_t *sps)
{
	struct sequence_params_set_t *sps_new, *tmp;

	sps_new = (struct sequence_params_set_t *)malloc(sizeof(struct sequence_params_set_t));
	memcpy(sps_new, sps, sizeof(struct sequence_params_set_t));
	sps_new->next = NULL;

	if(sps_head == NULL)
	{
		sps_head = sps_new;
	}
	else
	{
		/*find the tail*/
		tmp = sps_head;

		while(tmp->next != NULL)
		{
			tmp = tmp->next;
		}

		tmp->next = sps_new;
	}
}

struct sequence_params_set_t * find_sps(int nal_unit_type, int seq_parameter_set_id)
{
	struct sequence_params_set_t *tmp;

	tmp = sps_head;

	while(tmp != NULL)
	{
		if(nal_unit_type == 20 || nal_unit_type == 14)
		{
			if( (tmp->profile_idc == 83 || tmp->profile_idc == 86) &&  tmp->seq_parameter_set_id == seq_parameter_set_id)
			{
				return tmp;
			}
		}
		else
		{

			if(tmp->seq_parameter_set_id == seq_parameter_set_id)
			{
				return tmp;
			}
		}

		tmp = tmp->next;
	}

	return NULL;
}



void store_pps(struct picture_params_set_t *pps)
{
	struct picture_params_set_t *pps_new, *tmp;

	pps_new = (struct picture_params_set_t *)malloc(sizeof(struct picture_params_set_t));
	memcpy(pps_new, pps, sizeof(struct picture_params_set_t));
	pps_new->next = NULL;

	if(pps_head == NULL)
	{
		pps_head = pps_new;
	}
	else
	{
		/*find the tail*/
		tmp = pps_head;

		while(tmp->next != NULL)
		{
			tmp = tmp->next;
		}

		tmp->next = pps_new;
	}
}

struct picture_params_set_t * find_pps(int pic_parameter_set_id)
{
	struct picture_params_set_t *tmp;

	tmp = pps_head;

	while(tmp != NULL)
	{
		if(tmp->pic_parameter_set_id == pic_parameter_set_id)
		{
			return tmp;
		}

		tmp = tmp->next;
	}

	return NULL;
}
