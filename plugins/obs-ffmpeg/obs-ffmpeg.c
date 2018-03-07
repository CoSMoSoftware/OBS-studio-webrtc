#include <obs-module.h>
#include <util/darray.h>
#include <util/platform.h>
#include <libavutil/log.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <pthread.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-ffmpeg", "en-US")

extern struct obs_source_info  ffmpeg_source;
extern struct obs_output_info  ffmpeg_output;
extern struct obs_output_info  ffmpeg_muxer;
extern struct obs_output_info  replay_buffer;
extern struct obs_encoder_info aac_encoder_info;
extern struct obs_encoder_info opus_encoder_info;
extern struct obs_encoder_info nvenc_encoder_info;

static DARRAY(struct log_context {
	void *context;
	char str[4096];
	int print_prefix;
} *) active_log_contexts;
static DARRAY(struct log_context *) cached_log_contexts;
pthread_mutex_t log_contexts_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct log_context *create_or_fetch_log_context(void *context)
{
	pthread_mutex_lock(&log_contexts_mutex);
	for (size_t i = 0; i < active_log_contexts.num; i++) {
		if (context == active_log_contexts.array[i]->context) {
			pthread_mutex_unlock(&log_contexts_mutex);
			return active_log_contexts.array[i];
		}
	}

	struct log_context *new_log_context = NULL;

	size_t cnt = cached_log_contexts.num;
	if (!!cnt) {
		new_log_context = cached_log_contexts.array[cnt - 1];
		da_pop_back(cached_log_contexts);
	}

	if (!new_log_context)
		new_log_context = bzalloc(sizeof(struct log_context));

	new_log_context->context = context;
	new_log_context->str[0] = '\0';
	new_log_context->print_prefix = 1;

	da_push_back(active_log_contexts, &new_log_context);

	pthread_mutex_unlock(&log_contexts_mutex);

	return new_log_context;
}

static void destroy_log_context(struct log_context *log_context)
{
	pthread_mutex_lock(&log_contexts_mutex);
	da_erase_item(active_log_contexts, &log_context);
	da_push_back(cached_log_contexts, &log_context);
	pthread_mutex_unlock(&log_contexts_mutex);
}

static void ffmpeg_log_callback(void* context, int level, const char* format,
	va_list args)
{
	if (format == NULL)
		return;

	struct log_context *log_context = create_or_fetch_log_context(context);

	char *str = log_context->str;

	av_log_format_line(context, level, format, args, str + strlen(str),
			(int)(sizeof(log_context->str) - strlen(str)),
			&log_context->print_prefix);

	int obs_level;
	switch (level) {
	case AV_LOG_PANIC:
	case AV_LOG_FATAL:
		obs_level = LOG_ERROR;
		break;
	case AV_LOG_ERROR:
	case AV_LOG_WARNING:
		obs_level = LOG_WARNING;
		break;
	case AV_LOG_INFO:
	case AV_LOG_VERBOSE:
		obs_level = LOG_INFO;
		break;
	case AV_LOG_DEBUG:
	default:
		obs_level = LOG_DEBUG;
	}

	if (!log_context->print_prefix)
		return;

	char *str_end = str + strlen(str) - 1;
	while(str < str_end) {
		if (*str_end != '\n')
			break;
		*str_end-- = '\0';
	}

	if (str_end <= str)
		goto cleanup;

	blog(obs_level, "[ffmpeg] %s", str);

cleanup:
	destroy_log_context(log_context);
}

#ifndef __APPLE__

static const char *nvenc_check_name = "nvenc_check";

static bool nvenc_supported(void)
{
	av_register_all();

	profile_start(nvenc_check_name);

	AVCodec *nvenc = avcodec_find_encoder_by_name("nvenc_h264");
	void *lib = NULL;
	bool success = false;

	if (!nvenc) {
		goto cleanup;
	}

#if defined(_WIN32)
	if (sizeof(void*) == 8) {
		lib = os_dlopen("nvEncodeAPI64.dll");
	} else {
		lib = os_dlopen("nvEncodeAPI.dll");
	}
#else
	lib = os_dlopen("libnvidia-encode.so.1");
#endif

	/* ------------------------------------------- */

	success = !!lib;

cleanup:
	if (lib)
		os_dlclose(lib);
	profile_end(nvenc_check_name);
	return success;
}

#endif

bool obs_module_load(void)
{
	da_init(active_log_contexts);
	da_init(cached_log_contexts);

	//av_log_set_callback(ffmpeg_log_callback);

	obs_register_source(&ffmpeg_source);
	obs_register_output(&ffmpeg_output);
	obs_register_output(&ffmpeg_muxer);
	obs_register_output(&replay_buffer);
	obs_register_encoder(&aac_encoder_info);
	obs_register_encoder(&opus_encoder_info);
#ifndef __APPLE__
	if (nvenc_supported()) {
		blog(LOG_INFO, "NVENC supported");
		obs_register_encoder(&nvenc_encoder_info);
	}
#endif
	return true;
}

void obs_module_unload(void)
{
	av_log_set_callback(av_log_default_callback);

#ifdef _WIN32
	pthread_mutex_destroy(&log_contexts_mutex);
#endif

	for (size_t i = 0; i < active_log_contexts.num; i++) {
		bfree(active_log_contexts.array[i]);
	}
	for (size_t i = 0; i < cached_log_contexts.num; i++) {
		bfree(cached_log_contexts.array[i]);
	}

	da_free(active_log_contexts);
	da_free(cached_log_contexts);
}
