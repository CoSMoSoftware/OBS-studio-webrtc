/*****************************************************************************
Copyright (C) 2016 by c3r1c3 <c3r1c3@nevermindonline.com>

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
*****************************************************************************/
#include <obs-module.h>
#include <graphics/matrix4.h>
#include <graphics/quat.h>

/* clang-format off */

#define SETTING_GAMMA                  "gamma"
#define SETTING_CONTRAST               "contrast"
#define SETTING_BRIGHTNESS             "brightness"
#define SETTING_SATURATION             "saturation"
#define SETTING_HUESHIFT               "hue_shift"
#define SETTING_OPACITY                "opacity"
#define SETTING_COLOR                  "color"

#define TEXT_GAMMA                     obs_module_text("Gamma")
#define TEXT_CONTRAST                  obs_module_text("Contrast")
#define TEXT_BRIGHTNESS                obs_module_text("Brightness")
#define TEXT_SATURATION                obs_module_text("Saturation")
#define TEXT_HUESHIFT                  obs_module_text("HueShift")
#define TEXT_OPACITY                   obs_module_text("Opacity")
#define TEXT_COLOR                     obs_module_text("Color")

/* clang-format on */

struct color_correction_filter_data {
	obs_source_t *context;

	gs_effect_t *effect;

	gs_eparam_t *gamma_param;
	gs_eparam_t *final_matrix_param;

	struct vec3 gamma;
	float contrast;
	float brightness;
	float saturation;
	float hue_shift;
	float opacity;
	struct vec4 color;

	/* Pre-Computes */
	struct matrix4 con_matrix;
	struct matrix4 bright_matrix;
	struct matrix4 sat_matrix;
	struct matrix4 hue_op_matrix;
	struct matrix4 color_matrix;
	struct matrix4 final_matrix;

	struct vec3 rot_quaternion;
	float rot_quaternion_w;
	struct vec3 cross;
	struct vec3 square;
	struct vec3 wimag;
	struct vec3 diag;
	struct vec3 a_line;
	struct vec3 b_line;
	struct vec3 half_unit;
};

static const float root3 = 0.57735f;
static const float red_weight = 0.299f;
static const float green_weight = 0.587f;
static const float blue_weight = 0.114f;

/*
 * As the functions' namesake, this provides the internal name of your Filter,
 * which is then translated/referenced in the "data/locale" files.
 */
static const char *color_correction_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("ColorFilter");
}

/*
 * This function is called (see bottom of this file for more details)
 * whenever the OBS filter interface changes. So when the user is messing
 * with a slider this function is called to update the internal settings
 * in OBS, and hence the settings being passed to the CPU/GPU.
 */
static void color_correction_filter_update(void *data, obs_data_t *settings)
{
	struct color_correction_filter_data *filter = data;

	/* Build our Gamma numbers. */
	double gamma = obs_data_get_double(settings, SETTING_GAMMA);
	gamma = (gamma < 0.0) ? (-gamma + 1.0) : (1.0 / (gamma + 1.0));
	vec3_set(&filter->gamma, (float)gamma, (float)gamma, (float)gamma);

	/* Build our contrast number. */
	filter->contrast =
		(float)obs_data_get_double(settings, SETTING_CONTRAST) + 1.0f;
	float one_minus_con = (1.0f - filter->contrast) / 2.0f;

	/* Now let's build our Contrast matrix. */
	filter->con_matrix = (struct matrix4){filter->contrast,
					      0.0f,
					      0.0f,
					      0.0f,
					      0.0f,
					      filter->contrast,
					      0.0f,
					      0.0f,
					      0.0f,
					      0.0f,
					      filter->contrast,
					      0.0f,
					      one_minus_con,
					      one_minus_con,
					      one_minus_con,
					      1.0f};

	/* Build our brightness number. */
	filter->brightness =
		(float)obs_data_get_double(settings, SETTING_BRIGHTNESS);

	/*
	 * Now let's build our Brightness matrix.
	 * Earlier (in the function color_correction_filter_create) we set
	 * this matrix to the identity matrix, so now we only need
	 * to set the 3 variables that have changed.
	 */
	filter->bright_matrix.t.x = filter->brightness;
	filter->bright_matrix.t.y = filter->brightness;
	filter->bright_matrix.t.z = filter->brightness;

	/* Build our Saturation number. */
	filter->saturation =
		(float)obs_data_get_double(settings, SETTING_SATURATION) + 1.0f;

	/* Factor in the selected color weights. */
	float one_minus_sat_red = (1.0f - filter->saturation) * red_weight;
	float one_minus_sat_green = (1.0f - filter->saturation) * green_weight;
	float one_minus_sat_blue = (1.0f - filter->saturation) * blue_weight;
	float sat_val_red = one_minus_sat_red + filter->saturation;
	float sat_val_green = one_minus_sat_green + filter->saturation;
	float sat_val_blue = one_minus_sat_blue + filter->saturation;

	/* Now we build our Saturation matrix. */
	filter->sat_matrix = (struct matrix4){sat_val_red,
					      one_minus_sat_red,
					      one_minus_sat_red,
					      0.0f,
					      one_minus_sat_green,
					      sat_val_green,
					      one_minus_sat_green,
					      0.0f,
					      one_minus_sat_blue,
					      one_minus_sat_blue,
					      sat_val_blue,
					      0.0f,
					      0.0f,
					      0.0f,
					      0.0f,
					      1.0f};

	/* Build our Hue number. */
	filter->hue_shift =
		(float)obs_data_get_double(settings, SETTING_HUESHIFT);

	/* Build our Transparency number. */
	filter->opacity =
		(float)obs_data_get_int(settings, SETTING_OPACITY) * 0.01f;

	/* Hue is the radian of 0 to 360 degrees. */
	float half_angle = 0.5f * (float)(filter->hue_shift / (180.0f / M_PI));

	/* Pseudo-Quaternion To Matrix. */
	float rot_quad1 = root3 * (float)sin(half_angle);
	vec3_set(&filter->rot_quaternion, rot_quad1, rot_quad1, rot_quad1);
	filter->rot_quaternion_w = (float)cos(half_angle);

	vec3_mul(&filter->cross, &filter->rot_quaternion,
		 &filter->rot_quaternion);
	vec3_mul(&filter->square, &filter->rot_quaternion,
		 &filter->rot_quaternion);
	vec3_mulf(&filter->wimag, &filter->rot_quaternion,
		  filter->rot_quaternion_w);

	vec3_mulf(&filter->square, &filter->square, 2.0f);
	vec3_sub(&filter->diag, &filter->half_unit, &filter->square);
	vec3_add(&filter->a_line, &filter->cross, &filter->wimag);
	vec3_sub(&filter->b_line, &filter->cross, &filter->wimag);

	/* Now we build our Hue and Opacity matrix. */
	filter->hue_op_matrix = (struct matrix4){filter->diag.x * 2.0f,
						 filter->b_line.z * 2.0f,
						 filter->a_line.y * 2.0f,
						 0.0f,

						 filter->a_line.z * 2.0f,
						 filter->diag.y * 2.0f,
						 filter->b_line.x * 2.0f,
						 0.0f,

						 filter->b_line.y * 2.0f,
						 filter->a_line.x * 2.0f,
						 filter->diag.z * 2.0f,
						 0.0f,

						 0.0f,
						 0.0f,
						 0.0f,
						 filter->opacity};

	/* Now get the overlay color data. */
	uint32_t color = (uint32_t)obs_data_get_int(settings, SETTING_COLOR);
	vec4_from_rgba(&filter->color, color);

	/*
	* Now let's build our Color 'overlay' matrix.
	* Earlier (in the function color_correction_filter_create) we set
	* this matrix to the identity matrix, so now we only need
	* to set the 6 variables that have changed.
	*/
	filter->color_matrix.x.x = filter->color.x;
	filter->color_matrix.y.y = filter->color.y;
	filter->color_matrix.z.z = filter->color.z;

	filter->color_matrix.t.x = filter->color.w * filter->color.x;
	filter->color_matrix.t.y = filter->color.w * filter->color.y;
	filter->color_matrix.t.z = filter->color.w * filter->color.z;

	/* First we apply the Contrast & Brightness matrix. */
	matrix4_mul(&filter->final_matrix, &filter->bright_matrix,
		    &filter->con_matrix);
	/* Now we apply the Saturation matrix. */
	matrix4_mul(&filter->final_matrix, &filter->final_matrix,
		    &filter->sat_matrix);
	/* Next we apply the Hue+Opacity matrix. */
	matrix4_mul(&filter->final_matrix, &filter->final_matrix,
		    &filter->hue_op_matrix);
	/* Lastly we apply the Color Wash matrix. */
	matrix4_mul(&filter->final_matrix, &filter->final_matrix,
		    &filter->color_matrix);
}

/*
 * Since this is C we have to be careful when destroying/removing items from
 * OBS. Jim has added several useful functions to help keep memory leaks to
 * a minimum, and handle the destruction and construction of these filters.
 */
static void color_correction_filter_destroy(void *data)
{
	struct color_correction_filter_data *filter = data;

	if (filter->effect) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		obs_leave_graphics();
	}

	bfree(data);
}

/*
 * When you apply a filter OBS creates it, and adds it to the source. OBS also
 * starts rendering it immediately. This function doesn't just 'create' the
 * filter, it also calls the render function (farther below) that contains the
 * actual rendering code.
 */
static void *color_correction_filter_create(obs_data_t *settings,
					    obs_source_t *context)
{
	/*
	* Because of limitations of pre-c99 compilers, you can't create an
	* array that doesn't have a known size at compile time. The below
	* function calculates the size needed and allocates memory to
	* handle the source.
	*/
	struct color_correction_filter_data *filter =
		bzalloc(sizeof(struct color_correction_filter_data));

	/*
	 * By default the effect file is stored in the ./data directory that
	 * your filter resides in.
	 */
	char *effect_path = obs_module_file("color_correction_filter.effect");

	filter->context = context;

	/* Set/clear/assign for all necessary vectors. */
	vec3_set(&filter->half_unit, 0.5f, 0.5f, 0.5f);
	matrix4_identity(&filter->bright_matrix);
	matrix4_identity(&filter->color_matrix);

	/* Here we enter the GPU drawing/shader portion of our code. */
	obs_enter_graphics();

	/* Load the shader on the GPU. */
	filter->effect = gs_effect_create_from_file(effect_path, NULL);

	/* If the filter is active pass the parameters to the filter. */
	if (filter->effect) {
		filter->gamma_param = gs_effect_get_param_by_name(
			filter->effect, SETTING_GAMMA);
		filter->final_matrix_param = gs_effect_get_param_by_name(
			filter->effect, "color_matrix");
	}

	obs_leave_graphics();

	bfree(effect_path);

	/*
	 * If the filter has been removed/deactivated, destroy the filter
	 * and exit out so we don't crash OBS by telling it to update
	 * values that don't exist anymore.
	 */
	if (!filter->effect) {
		color_correction_filter_destroy(filter);
		return NULL;
	}

	/*
	 * It's important to call the update function here. If we don't
	 * we could end up with the user controlled sliders and values
	 * updating, but the visuals not updating to match.
	 */
	color_correction_filter_update(filter, settings);
	return filter;
}

/* This is where the actual rendering of the filter takes place. */
static void color_correction_filter_render(void *data, gs_effect_t *effect)
{
	struct color_correction_filter_data *filter = data;

	if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
					     OBS_ALLOW_DIRECT_RENDERING))
		return;

	/* Now pass the interface variables to the .effect file. */
	gs_effect_set_vec3(filter->gamma_param, &filter->gamma);
	gs_effect_set_matrix4(filter->final_matrix_param,
			      &filter->final_matrix);

	obs_source_process_filter_end(filter->context, filter->effect, 0, 0);

	UNUSED_PARAMETER(effect);
}

/*
 * This function sets the interface. the types (add_*_Slider), the type of
 * data collected (int), the internal name, user-facing name, minimum,
 * maximum and step values. While a custom interface can be built, for a
 * simple filter like this it's better to use the supplied functions.
 */
static obs_properties_t *color_correction_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_float_slider(props, SETTING_GAMMA, TEXT_GAMMA, -3.0,
					3.0, 0.01);

	obs_properties_add_float_slider(props, SETTING_CONTRAST, TEXT_CONTRAST,
					-2.0, 2.0, 0.01);
	obs_properties_add_float_slider(props, SETTING_BRIGHTNESS,
					TEXT_BRIGHTNESS, -1.0, 1.0, 0.01);
	obs_properties_add_float_slider(props, SETTING_SATURATION,
					TEXT_SATURATION, -1.0, 5.0, 0.01);
	obs_properties_add_float_slider(props, SETTING_HUESHIFT, TEXT_HUESHIFT,
					-180.0, 180.0, 0.01);
	obs_properties_add_int_slider(props, SETTING_OPACITY, TEXT_OPACITY, 0,
				      100, 1);

	obs_properties_add_color(props, SETTING_COLOR, TEXT_COLOR);

	UNUSED_PARAMETER(data);
	return props;
}

/*
 * As the functions' namesake, this provides the default settings for any
 * options you wish to provide a default for. Try to select defaults that
 * make sense to the end user, or that don't effect the data.
 * *NOTE* this function is completely optional, as is providing a default
 * for any particular setting.
 */
static void color_correction_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, SETTING_GAMMA, 0.0);
	obs_data_set_default_double(settings, SETTING_CONTRAST, 0.0);
	obs_data_set_default_double(settings, SETTING_BRIGHTNESS, 0.0);
	obs_data_set_default_double(settings, SETTING_SATURATION, 0.0);
	obs_data_set_default_double(settings, SETTING_HUESHIFT, 0.0);
	obs_data_set_default_double(settings, SETTING_OPACITY, 100.0);
	obs_data_set_default_int(settings, SETTING_COLOR, 0xFFFFFF);
}

/*
 * So how does OBS keep track of all these plug-ins/filters? How does OBS know
 * which function to call when it needs to update a setting? Or a source? Or
 * what type of source this is?
 *
 * OBS does it through the obs_source_info_struct. Notice how variables are
 * assigned the name of a function? Notice how the function name has the
 * variable name in it? While not mandatory, it helps a ton for you (and those
 * reading your code) to follow this convention.
 */
struct obs_source_info color_filter = {
	.id = "color_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = color_correction_filter_name,
	.create = color_correction_filter_create,
	.destroy = color_correction_filter_destroy,
	.video_render = color_correction_filter_render,
	.update = color_correction_filter_update,
	.get_properties = color_correction_filter_properties,
	.get_defaults = color_correction_filter_defaults,
};
