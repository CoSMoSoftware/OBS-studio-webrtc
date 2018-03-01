#include <obs-module.h>
#include <util/threading.h>
#include <util/platform.h>
#include <util/darray.h>
#include <util/dstr.h>

#define do_log(level, format, ...) \
	blog(level, "[slideshow: '%s'] " format, \
			obs_source_get_name(ss->source), ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)

#define S_TR_SPEED                     "transition_speed"
#define S_CUSTOM_SIZE                  "use_custom_size"
#define S_SLIDE_TIME                   "slide_time"
#define S_TRANSITION                   "transition"
#define S_RANDOMIZE                    "randomize"
#define S_FILES                        "files"

#define TR_CUT                         "cut"
#define TR_FADE                        "fade"
#define TR_SWIPE                       "swipe"
#define TR_SLIDE                       "slide"

#define T_(text) obs_module_text("SlideShow." text)
#define T_TR_SPEED                     T_("TransitionSpeed")
#define T_CUSTOM_SIZE                  T_("CustomSize")
#define T_CUSTOM_SIZE_AUTO             T_("CustomSize.Auto")
#define T_SLIDE_TIME                   T_("SlideTime")
#define T_TRANSITION                   T_("Transition")
#define T_RANDOMIZE                    T_("Randomize")
#define T_FILES                        T_("Files")

#define T_TR_(text) obs_module_text("SlideShow.Transition." text)
#define T_TR_CUT                       T_TR_("Cut")
#define T_TR_FADE                      T_TR_("Fade")
#define T_TR_SWIPE                     T_TR_("Swipe")
#define T_TR_SLIDE                     T_TR_("Slide")

/* ------------------------------------------------------------------------- */

struct image_file_data {
	char *path;
	obs_source_t *source;
};

struct slideshow {
	obs_source_t *source;

	bool randomize;
	float slide_time;
	uint32_t tr_speed;
	const char *tr_name;
	obs_source_t *transition;

	float elapsed;
	size_t cur_item;

	uint32_t cx;
	uint32_t cy;

	pthread_mutex_t mutex;
	DARRAY(struct image_file_data) files;
};

static obs_source_t *get_transition(struct slideshow *ss)
{
	obs_source_t *tr;

	pthread_mutex_lock(&ss->mutex);
	tr = ss->transition;
	obs_source_addref(tr);
	pthread_mutex_unlock(&ss->mutex);

	return tr;
}

static obs_source_t *get_source(struct darray *array, const char *path)
{
	DARRAY(struct image_file_data) files;
	obs_source_t *source = NULL;

	files.da = *array;

	for (size_t i = 0; i < files.num; i++) {
		const char *cur_path = files.array[i].path;

		if (strcmp(path, cur_path) == 0) {
			source = files.array[i].source;
			obs_source_addref(source);
			break;
		}
	}

	return source;
}

static obs_source_t *create_source_from_file(const char *file)
{
	obs_data_t *settings = obs_data_create();
	obs_source_t *source;

	obs_data_set_string(settings, "file", file);
	obs_data_set_bool(settings, "unload", false);
	source = obs_source_create_private("image_source", NULL, settings);

	obs_data_release(settings);
	return source;
}

static void free_files(struct darray *array)
{
	DARRAY(struct image_file_data) files;
	files.da = *array;

	for (size_t i = 0; i < files.num; i++) {
		bfree(files.array[i].path);
		obs_source_release(files.array[i].source);
	}

	da_free(files);
}

static inline size_t random_file(struct slideshow *ss)
{
	return (size_t)rand() % ss->files.num;
}

/* ------------------------------------------------------------------------- */

static const char *ss_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("SlideShow");
}

static void add_file(struct slideshow *ss, struct darray *array,
		const char *path, uint32_t *cx, uint32_t *cy)
{
	DARRAY(struct image_file_data) new_files;
	struct image_file_data data;
	obs_source_t *new_source;

	new_files.da = *array;

	pthread_mutex_lock(&ss->mutex);
	new_source = get_source(&ss->files.da, path);
	pthread_mutex_unlock(&ss->mutex);

	if (!new_source)
		new_source = get_source(&new_files.da, path);
	if (!new_source)
		new_source = create_source_from_file(path);

	if (new_source) {
		uint32_t new_cx = obs_source_get_width(new_source);
		uint32_t new_cy = obs_source_get_height(new_source);

		data.path = bstrdup(path);
		data.source = new_source;
		da_push_back(new_files, &data);

		if (new_cx > *cx) *cx = new_cx;
		if (new_cy > *cy) *cy = new_cy;
	}

	*array = new_files.da;
}

static bool valid_extension(const char *ext)
{
	if (!ext)
		return false;
	return astrcmpi(ext, ".bmp") == 0 ||
	       astrcmpi(ext, ".tga") == 0 ||
	       astrcmpi(ext, ".png") == 0 ||
	       astrcmpi(ext, ".jpeg") == 0 ||
	       astrcmpi(ext, ".jpg") == 0 ||
	       astrcmpi(ext, ".gif") == 0;
}

static void ss_update(void *data, obs_data_t *settings)
{
	DARRAY(struct image_file_data) new_files;
	DARRAY(struct image_file_data) old_files;
	obs_source_t *new_tr = NULL;
	obs_source_t *old_tr = NULL;
	struct slideshow *ss = data;
	obs_data_array_t *array;
	const char *tr_name;
	uint32_t new_duration;
	uint32_t new_speed;
	uint32_t cx = 0;
	uint32_t cy = 0;
	size_t count;

	/* ------------------------------------- */
	/* get settings data */

	da_init(new_files);

	tr_name = obs_data_get_string(settings, S_TRANSITION);
	if (astrcmpi(tr_name, TR_CUT) == 0)
		tr_name = "cut_transition";
	else if (astrcmpi(tr_name, TR_SWIPE) == 0)
		tr_name = "swipe_transition";
	else if (astrcmpi(tr_name, TR_SLIDE) == 0)
		tr_name = "slide_transition";
	else
		tr_name = "fade_transition";

	ss->randomize = obs_data_get_bool(settings, S_RANDOMIZE);

	if (!ss->tr_name || strcmp(tr_name, ss->tr_name) != 0)
		new_tr = obs_source_create_private(tr_name, NULL, NULL);

	new_duration = (uint32_t)obs_data_get_int(settings, S_SLIDE_TIME);
	new_speed = (uint32_t)obs_data_get_int(settings, S_TR_SPEED);

	array = obs_data_get_array(settings, S_FILES);
	count = obs_data_array_count(array);

	/* ------------------------------------- */
	/* create new list of sources */

	for (size_t i = 0; i < count; i++) {
		obs_data_t *item = obs_data_array_item(array, i);
		const char *path = obs_data_get_string(item, "value");
		os_dir_t *dir = os_opendir(path);

		if (dir) {
			struct dstr dir_path = {0};
			struct os_dirent *ent;

			for (;;) {
				const char *ext;

				ent = os_readdir(dir);
				if (!ent)
					break;
				if (ent->directory)
					continue;

				ext = os_get_path_extension(ent->d_name);
				if (!valid_extension(ext))
					continue;

				dstr_copy(&dir_path, path);
				dstr_cat_ch(&dir_path, '/');
				dstr_cat(&dir_path, ent->d_name);
				add_file(ss, &new_files.da, dir_path.array,
						&cx, &cy);
			}

			dstr_free(&dir_path);
			os_closedir(dir);
		} else {
			add_file(ss, &new_files.da, path, &cx, &cy);
		}

		obs_data_release(item);
	}

	/* ------------------------------------- */
	/* update settings data */

	pthread_mutex_lock(&ss->mutex);

	old_files.da = ss->files.da;
	ss->files.da = new_files.da;
	if (new_tr) {
		old_tr = ss->transition;
		ss->transition = new_tr;
	}

	if (new_duration < 50)
		new_duration = 50;
	if (new_speed > (new_duration - 50))
		new_speed = new_duration - 50;

	ss->tr_speed = new_speed;
	ss->tr_name = tr_name;
	ss->slide_time = (float)new_duration / 1000.0f;

	pthread_mutex_unlock(&ss->mutex);

	/* ------------------------------------- */
	/* clean up and restart transition */

	if (old_tr)
		obs_source_release(old_tr);
	free_files(&old_files.da);

	/* ------------------------- */

	const char *res_str = obs_data_get_string(settings, S_CUSTOM_SIZE);
	bool aspect_only = false, use_auto = true;
	int cx_in = 0, cy_in = 0;

	if (strcmp(res_str, T_CUSTOM_SIZE_AUTO) != 0) {
		int ret = sscanf(res_str, "%dx%d", &cx_in, &cy_in);
		if (ret == 2) {
			aspect_only = false;
			use_auto = false;
		} else {
			ret = sscanf(res_str, "%d:%d", &cx_in, &cy_in);
			if (ret == 2) {
				aspect_only = true;
				use_auto = false;
			}
		}
	}

	if (!use_auto) {
		double cx_f = (double)cx;
		double cy_f = (double)cy;

		double old_aspect = cx_f / cy_f;
		double new_aspect = (double)cx_in / (double)cy_in;

		if (aspect_only) {
			if (fabs(old_aspect - new_aspect) > EPSILON) {
				if (new_aspect > old_aspect)
					cx = (uint32_t)(cy_f * new_aspect);
				else
					cy = (uint32_t)(cx_f / new_aspect);
			}
		} else {
			cx = (uint32_t)cx_in;
			cy = (uint32_t)cy_in;
		}
	}

	/* ------------------------- */

	ss->cx = cx;
	ss->cy = cy;
	ss->cur_item = 0;
	ss->elapsed = 0.0f;
	obs_transition_set_size(ss->transition, cx, cy);
	obs_transition_set_alignment(ss->transition, OBS_ALIGN_CENTER);
	obs_transition_set_scale_type(ss->transition,
			OBS_TRANSITION_SCALE_ASPECT);

	if (ss->randomize && ss->files.num)
		ss->cur_item = random_file(ss);
	if (new_tr)
		obs_source_add_active_child(ss->source, new_tr);
	if (ss->files.num)
		obs_transition_start(ss->transition, OBS_TRANSITION_MODE_AUTO,
				ss->tr_speed,
				ss->files.array[ss->cur_item].source);

	obs_data_array_release(array);
}

static void ss_destroy(void *data)
{
	struct slideshow *ss = data;

	obs_source_release(ss->transition);
	free_files(&ss->files.da);
	pthread_mutex_destroy(&ss->mutex);
	bfree(ss);
}

static void *ss_create(obs_data_t *settings, obs_source_t *source)
{
	struct slideshow *ss = bzalloc(sizeof(*ss));
	ss->source = source;

	pthread_mutex_init_value(&ss->mutex);
	if (pthread_mutex_init(&ss->mutex, NULL) != 0)
		goto error;

	obs_source_update(source, NULL);

	UNUSED_PARAMETER(settings);
	return ss;

error:
	ss_destroy(ss);
	return NULL;
}

static void ss_video_render(void *data, gs_effect_t *effect)
{
	struct slideshow *ss = data;
	obs_source_t *transition = get_transition(ss);

	if (transition) {
		obs_source_video_render(transition);
		obs_source_release(transition);
	}

	UNUSED_PARAMETER(effect);
}

static void ss_video_tick(void *data, float seconds)
{
	struct slideshow *ss = data;

	if (!ss->transition || !ss->slide_time)
		return;

	ss->elapsed += seconds;
	if (ss->elapsed > ss->slide_time) {
		ss->elapsed -= ss->slide_time;

		if (ss->randomize) {
			size_t next = ss->cur_item;
			if (ss->files.num > 1) {
				while (next == ss->cur_item)
					next = random_file(ss);
			}
			ss->cur_item = next;

		} else if (++ss->cur_item >= ss->files.num) {
			ss->cur_item = 0;
		}

		if (ss->files.num)
			obs_transition_start(ss->transition,
					OBS_TRANSITION_MODE_AUTO, ss->tr_speed,
					ss->files.array[ss->cur_item].source);
	}
}

static inline bool ss_audio_render_(obs_source_t *transition, uint64_t *ts_out,
		struct obs_source_audio_mix *audio_output,
		uint32_t mixers, size_t channels, size_t sample_rate)
{
	struct obs_source_audio_mix child_audio;
	uint64_t source_ts;

	if (obs_source_audio_pending(transition))
		return false;

	source_ts = obs_source_get_audio_timestamp(transition);
	if (!source_ts)
		return false;

	obs_source_get_audio_mix(transition, &child_audio);
	for (size_t mix = 0; mix < MAX_AUDIO_MIXES; mix++) {
		if ((mixers & (1 << mix)) == 0)
			continue;

		for (size_t ch = 0; ch < channels; ch++) {
			float *out = audio_output->output[mix].data[ch];
			float *in = child_audio.output[mix].data[ch];

			memcpy(out, in, AUDIO_OUTPUT_FRAMES *
					MAX_AUDIO_CHANNELS * sizeof(float));
		}
	}

	*ts_out = source_ts;

	UNUSED_PARAMETER(sample_rate);
	return true;
}

static bool ss_audio_render(void *data, uint64_t *ts_out,
		struct obs_source_audio_mix *audio_output,
		uint32_t mixers, size_t channels, size_t sample_rate)
{
	struct slideshow *ss = data;
	obs_source_t *transition = get_transition(ss);
	bool success;

	if (!transition)
		return false;

	success = ss_audio_render_(transition, ts_out, audio_output, mixers,
			channels, sample_rate);

	obs_source_release(transition);
	return success;
}

static void ss_enum_sources(void *data, obs_source_enum_proc_t cb, void *param)
{
	struct slideshow *ss = data;

	pthread_mutex_lock(&ss->mutex);
	if (ss->transition)
		cb(ss->source, ss->transition, param);
	pthread_mutex_unlock(&ss->mutex);
}

static uint32_t ss_width(void *data)
{
	struct slideshow *ss = data;
	return ss->transition ? ss->cx : 0;
}

static uint32_t ss_height(void *data)
{
	struct slideshow *ss = data;
	return ss->transition ? ss->cy : 0;
}

static void ss_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, S_TRANSITION, "fade");
	obs_data_set_default_int(settings, S_SLIDE_TIME, 8000);
	obs_data_set_default_int(settings, S_TR_SPEED, 700);
	obs_data_set_default_string(settings, S_CUSTOM_SIZE, T_CUSTOM_SIZE_AUTO);
}

static const char *file_filter =
	"Image files (*.bmp *.tga *.png *.jpeg *.jpg *.gif)";

static const char *aspects[] = {
	"16:9",
	"16:10",
	"4:3",
	"1:1"
};

#define NUM_ASPECTS (sizeof(aspects) / sizeof(const char *))

static obs_properties_t *ss_properties(void *data)
{
	obs_properties_t *ppts = obs_properties_create();
	struct slideshow *ss = data;
	struct obs_video_info ovi;
	struct dstr path = {0};
	obs_property_t *p;
	int cx;
	int cy;

	/* ----------------- */

	obs_get_video_info(&ovi);
	cx = (int)ovi.base_width;
	cy = (int)ovi.base_height;

	/* ----------------- */

	p = obs_properties_add_list(ppts, S_TRANSITION, T_TRANSITION,
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, T_TR_CUT, TR_CUT);
	obs_property_list_add_string(p, T_TR_FADE, TR_FADE);
	obs_property_list_add_string(p, T_TR_SWIPE, TR_SWIPE);
	obs_property_list_add_string(p, T_TR_SLIDE, TR_SLIDE);

	obs_properties_add_int(ppts, S_SLIDE_TIME, T_SLIDE_TIME,
			50, 3600000, 50);
	obs_properties_add_int(ppts, S_TR_SPEED, T_TR_SPEED,
			0, 3600000, 50);
	obs_properties_add_bool(ppts, S_RANDOMIZE, T_RANDOMIZE);

	p = obs_properties_add_list(ppts, S_CUSTOM_SIZE, T_CUSTOM_SIZE,
			OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);

	obs_property_list_add_string(p, T_CUSTOM_SIZE_AUTO, T_CUSTOM_SIZE_AUTO);

	for (size_t i = 0; i < NUM_ASPECTS; i++)
		obs_property_list_add_string(p, aspects[i], aspects[i]);

	char str[32];
	snprintf(str, 32, "%dx%d", cx, cy);
	obs_property_list_add_string(p, str, str);

	if (ss) {
		pthread_mutex_lock(&ss->mutex);
		if (ss->files.num) {
			struct image_file_data *last = da_end(ss->files);
			const char *slash;

			dstr_copy(&path, last->path);
			dstr_replace(&path, "\\", "/");
			slash = strrchr(path.array, '/');
			if (slash)
				dstr_resize(&path, slash - path.array + 1);
		}
		pthread_mutex_unlock(&ss->mutex);
	}

	obs_properties_add_editable_list(ppts, S_FILES, T_FILES,
			OBS_EDITABLE_LIST_TYPE_FILES, file_filter, path.array);
	dstr_free(&path);

	return ppts;
}

struct obs_source_info slideshow_info = {
	.id                  = "slideshow",
	.type                = OBS_SOURCE_TYPE_INPUT,
	.output_flags        = OBS_SOURCE_VIDEO |
	                       OBS_SOURCE_CUSTOM_DRAW |
	                       OBS_SOURCE_COMPOSITE,
	.get_name            = ss_getname,
	.create              = ss_create,
	.destroy             = ss_destroy,
	.update              = ss_update,
	.video_render        = ss_video_render,
	.video_tick          = ss_video_tick,
	.audio_render        = ss_audio_render,
	.enum_active_sources = ss_enum_sources,
	.get_width           = ss_width,
	.get_height          = ss_height,
	.get_defaults        = ss_defaults,
	.get_properties      = ss_properties
};
