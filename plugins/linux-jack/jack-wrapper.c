/*
Copyright (C) 2015 by Bernd Buschinski <b.buschinski@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "jack-wrapper.h"

#include <util/threading.h>
#include <stdio.h>

#include <util/platform.h>

#define blog(level, msg, ...) blog(level, "jack-input: " msg, ##__VA_ARGS__)

/**
 * Get obs speaker layout from number of channels
 *
 * @param channels number of channels reported by jack
 *
 * @return obs speaker_layout id
 *
 * @note This *might* not work for some rather unusual setups, but should work
 *       fine for the majority of cases.
 */
static enum speaker_layout jack_channels_to_obs_speakers(uint_fast32_t channels)
{
	switch(channels) {
	case 1: return SPEAKERS_MONO;
	case 2: return SPEAKERS_STEREO;
	case 3: return SPEAKERS_2POINT1;
	case 5: return SPEAKERS_4POINT1;
	case 6: return SPEAKERS_5POINT1;
	/* What should we do with 7 channels? */
	/* case 7: return SPEAKERS_...; */
	case 8: return SPEAKERS_7POINT1;
	}

	return SPEAKERS_UNKNOWN;
}

int jack_process_callback(jack_nframes_t nframes, void* arg)
{
	struct jack_data* data = (struct jack_data*)arg;
	if (data == 0)
		return 0;

	pthread_mutex_lock(&data->jack_mutex);

	struct obs_source_audio out;
	out.speakers        = jack_channels_to_obs_speakers(data->channels);
	out.samples_per_sec = jack_get_sample_rate (data->jack_client);
	/* format is always 32 bit float for jack */
	out.format          = AUDIO_FORMAT_FLOAT_PLANAR;

	for (unsigned int i = 0; i < data->channels; ++i) {
		jack_default_audio_sample_t *jack_buffer =
			(jack_default_audio_sample_t *)jack_port_get_buffer(
				data->jack_ports[i], nframes);
		out.data[i] = (uint8_t *)jack_buffer;
	}

	out.frames    = nframes;
	out.timestamp = os_gettime_ns() -
				jack_frames_to_time(data->jack_client, nframes);

	obs_source_output_audio(data->source, &out);
	pthread_mutex_unlock(&data->jack_mutex);
	return 0;
}

int_fast32_t jack_init(struct jack_data* data)
{
	pthread_mutex_lock(&data->jack_mutex);

	if (data->jack_client != NULL)
		goto good;

	jack_options_t jack_option = data->start_jack_server ?
		JackNullOption : JackNoStartServer;

	data->jack_client = jack_client_open(data->device, jack_option, 0);
	if (data->jack_client == NULL) {
		blog(LOG_ERROR,
			"jack_client_open Error:"
			"Could not create JACK client! %s",
			data->device);
		goto error;
	}

	data->jack_ports = (jack_port_t**)bzalloc(
		sizeof(jack_port_t*) * data->channels);
	for (unsigned int i = 0; i < data->channels; ++i) {
		char port_name[10] = {'\0'};
		snprintf(port_name, sizeof(port_name), "in_%d", i+1);

		data->jack_ports[i] = jack_port_register(data->jack_client,
			port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
		if (data->jack_ports[i] == NULL) {
			blog(LOG_ERROR,
				"jack_port_register Error:"
				"Could not create JACK port! %s",
				port_name);
			goto error;
		}
	}

	if (jack_set_process_callback(data->jack_client,
			jack_process_callback, data) != 0) {
		blog(LOG_ERROR, "jack_set_process_callback Error");
		goto error;
	}

	if (jack_activate(data->jack_client) != 0) {
		blog(LOG_ERROR,
			"jack_activate Error:"
			"Could not activate JACK client!");
		goto error;
	}

good:
	pthread_mutex_unlock(&data->jack_mutex);
	return 0;

error:
	pthread_mutex_unlock(&data->jack_mutex);
	return 1;
}

void deactivate_jack(struct jack_data* data)
{
	pthread_mutex_lock(&data->jack_mutex);

	if (data->jack_client) {
		if (data->jack_ports != NULL) {
			for (int i = 0; i < data->channels; ++i) {
				if (data->jack_ports[i] != NULL)
					jack_port_unregister(data->jack_client,
						data->jack_ports[i]);
			}
			bfree(data->jack_ports);
			data->jack_ports = NULL;
		}
		jack_client_close(data->jack_client);
		data->jack_client = NULL;
	}
	pthread_mutex_unlock(&data->jack_mutex);
}
