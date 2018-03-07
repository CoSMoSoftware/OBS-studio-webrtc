#define MIN(x,y) ((x < y) ? x : y)
#define MAX(x,y) ((x > y) ? x : y)

void store_sps(struct sequence_params_set_t *sps);
struct sequence_params_set_t * find_sps(int nal_unit_type, int seq_parameter_set_id);
void store_pps(struct picture_params_set_t *pps);
struct picture_params_set_t * find_pps(int pic_parameter_set_id);