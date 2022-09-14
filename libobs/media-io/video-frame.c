/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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
******************************************************************************/

#include "video-frame.h"

#define ALIGN_SIZE(size, align) size = (((size) + (align - 1)) & (~(align - 1)))

/* messy code alarm */
void video_frame_init(struct video_frame *frame, enum video_format format,
		      uint32_t width, uint32_t height)
{
	size_t size;
	size_t offsets[MAX_AV_PLANES];
	int alignment = base_get_alignment();

	if (!frame)
		return;

	memset(frame, 0, sizeof(struct video_frame));
	memset(offsets, 0, sizeof(offsets));

	switch (format) {
	case VIDEO_FORMAT_NONE:
		return;

	case VIDEO_FORMAT_I420: {
		size = width * height;
		ALIGN_SIZE(size, alignment);
		offsets[0] = size;
		const uint32_t half_width = (width + 1) / 2;
		const uint32_t half_height = (height + 1) / 2;
		const uint32_t quarter_area = half_width * half_height;
		size += quarter_area;
		ALIGN_SIZE(size, alignment);
		offsets[1] = size;
		size += quarter_area;
		ALIGN_SIZE(size, alignment);
		frame->data[0] = bmalloc(size);
		frame->data[1] = (uint8_t *)frame->data[0] + offsets[0];
		frame->data[2] = (uint8_t *)frame->data[0] + offsets[1];
		frame->linesize[0] = width;
		frame->linesize[1] = half_width;
		frame->linesize[2] = half_width;
		break;
	}

	case VIDEO_FORMAT_NV12: {
		size = width * height;
		ALIGN_SIZE(size, alignment);
		offsets[0] = size;
		const uint32_t cbcr_width = (width + 1) & (UINT32_MAX - 1);
		size += cbcr_width * ((height + 1) / 2);
		ALIGN_SIZE(size, alignment);
		frame->data[0] = bmalloc(size);
		frame->data[1] = (uint8_t *)frame->data[0] + offsets[0];
		frame->linesize[0] = width;
		frame->linesize[1] = cbcr_width;
		break;
	}

	case VIDEO_FORMAT_Y800:
		size = width * height;
		ALIGN_SIZE(size, alignment);
		frame->data[0] = bmalloc(size);
		frame->linesize[0] = width;
		break;

	case VIDEO_FORMAT_YVYU:
	case VIDEO_FORMAT_YUY2:
	case VIDEO_FORMAT_UYVY: {
		const uint32_t double_width =
			((width + 1) & (UINT32_MAX - 1)) * 2;
		size = double_width * height;
		ALIGN_SIZE(size, alignment);
		frame->data[0] = bmalloc(size);
		frame->linesize[0] = double_width;
		break;
	}

	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
	case VIDEO_FORMAT_AYUV:
		size = width * height * 4;
		ALIGN_SIZE(size, alignment);
		frame->data[0] = bmalloc(size);
		frame->linesize[0] = width * 4;
		break;

	case VIDEO_FORMAT_I444:
		size = width * height;
		ALIGN_SIZE(size, alignment);
		frame->data[0] = bmalloc(size * 3);
		frame->data[1] = (uint8_t *)frame->data[0] + size;
		frame->data[2] = (uint8_t *)frame->data[1] + size;
		frame->linesize[0] = width;
		frame->linesize[1] = width;
		frame->linesize[2] = width;
		break;

	case VIDEO_FORMAT_I412:
		size = width * height * 2;
		ALIGN_SIZE(size, alignment);
		frame->data[0] = bmalloc(size * 3);
		frame->data[1] = (uint8_t *)frame->data[0] + size;
		frame->data[2] = (uint8_t *)frame->data[1] + size;
		frame->linesize[0] = width * 2;
		frame->linesize[1] = width * 2;
		frame->linesize[2] = width * 2;
		break;

	case VIDEO_FORMAT_BGR3:
		size = width * height * 3;
		ALIGN_SIZE(size, alignment);
		frame->data[0] = bmalloc(size);
		frame->linesize[0] = width * 3;
		break;

	case VIDEO_FORMAT_I422: {
		size = width * height;
		ALIGN_SIZE(size, alignment);
		offsets[0] = size;
		const uint32_t half_width = (width + 1) / 2;
		const uint32_t half_area = half_width * height;
		size += half_area;
		ALIGN_SIZE(size, alignment);
		offsets[1] = size;
		size += half_area;
		ALIGN_SIZE(size, alignment);
		frame->data[0] = bmalloc(size);
		frame->data[1] = (uint8_t *)frame->data[0] + offsets[0];
		frame->data[2] = (uint8_t *)frame->data[0] + offsets[1];
		frame->linesize[0] = width;
		frame->linesize[1] = half_width;
		frame->linesize[2] = half_width;
		break;
	}

	case VIDEO_FORMAT_I210: {
		size = width * height * 2;
		ALIGN_SIZE(size, alignment);
		offsets[0] = size;
		const uint32_t half_width = (width + 1) / 2;
		const uint32_t half_area = half_width * height;
		const uint32_t half_area_size = 2 * half_area;
		size += half_area_size;
		ALIGN_SIZE(size, alignment);
		offsets[1] = size;
		size += half_area_size;
		ALIGN_SIZE(size, alignment);
		frame->data[0] = bmalloc(size);
		frame->data[1] = (uint8_t *)frame->data[0] + offsets[0];
		frame->data[2] = (uint8_t *)frame->data[0] + offsets[1];
		frame->linesize[0] = width * 2;
		frame->linesize[1] = half_width * 2;
		frame->linesize[2] = half_width * 2;
		break;
	}

	case VIDEO_FORMAT_I40A: {
		size = width * height;
		ALIGN_SIZE(size, alignment);
		offsets[0] = size;
		const uint32_t half_width = (width + 1) / 2;
		const uint32_t half_height = (height + 1) / 2;
		const uint32_t quarter_area = half_width * half_height;
		size += quarter_area;
		ALIGN_SIZE(size, alignment);
		offsets[1] = size;
		size += quarter_area;
		ALIGN_SIZE(size, alignment);
		offsets[2] = size;
		size += width * height;
		ALIGN_SIZE(size, alignment);
		frame->data[0] = bmalloc(size);
		frame->data[1] = (uint8_t *)frame->data[0] + offsets[0];
		frame->data[2] = (uint8_t *)frame->data[0] + offsets[1];
		frame->data[3] = (uint8_t *)frame->data[0] + offsets[2];
		frame->linesize[0] = width;
		frame->linesize[1] = half_width;
		frame->linesize[2] = half_width;
		frame->linesize[3] = width;
		break;
	}

	case VIDEO_FORMAT_I42A: {
		size = width * height;
		ALIGN_SIZE(size, alignment);
		offsets[0] = size;
		const uint32_t half_width = (width + 1) / 2;
		const uint32_t half_area = half_width * height;
		size += half_area;
		ALIGN_SIZE(size, alignment);
		offsets[1] = size;
		size += half_area;
		ALIGN_SIZE(size, alignment);
		offsets[2] = size;
		size += width * height;
		ALIGN_SIZE(size, alignment);
		frame->data[0] = bmalloc(size);
		frame->data[1] = (uint8_t *)frame->data[0] + offsets[0];
		frame->data[2] = (uint8_t *)frame->data[0] + offsets[1];
		frame->data[3] = (uint8_t *)frame->data[0] + offsets[2];
		frame->linesize[0] = width;
		frame->linesize[1] = half_width;
		frame->linesize[2] = half_width;
		frame->linesize[3] = width;
		break;
	}

	case VIDEO_FORMAT_YUVA:
		size = width * height;
		ALIGN_SIZE(size, alignment);
		offsets[0] = size;
		size += width * height;
		ALIGN_SIZE(size, alignment);
		offsets[1] = size;
		size += width * height;
		ALIGN_SIZE(size, alignment);
		offsets[2] = size;
		size += width * height;
		ALIGN_SIZE(size, alignment);
		frame->data[0] = bmalloc(size);
		frame->data[1] = (uint8_t *)frame->data[0] + offsets[0];
		frame->data[2] = (uint8_t *)frame->data[0] + offsets[1];
		frame->data[3] = (uint8_t *)frame->data[0] + offsets[2];
		frame->linesize[0] = width;
		frame->linesize[1] = width;
		frame->linesize[2] = width;
		frame->linesize[3] = width;
		break;

	case VIDEO_FORMAT_YA2L: {
		const uint32_t linesize = width * 2;
		const uint32_t plane_size = linesize * height;
		size = plane_size;
		ALIGN_SIZE(size, alignment);
		offsets[0] = size;
		size += plane_size;
		ALIGN_SIZE(size, alignment);
		offsets[1] = size;
		size += plane_size;
		ALIGN_SIZE(size, alignment);
		offsets[2] = size;
		size += plane_size;
		ALIGN_SIZE(size, alignment);
		frame->data[0] = bmalloc(size);
		frame->data[1] = (uint8_t *)frame->data[0] + offsets[0];
		frame->data[2] = (uint8_t *)frame->data[0] + offsets[1];
		frame->data[3] = (uint8_t *)frame->data[0] + offsets[2];
		frame->linesize[0] = linesize;
		frame->linesize[1] = linesize;
		frame->linesize[2] = linesize;
		frame->linesize[3] = linesize;
		break;
	}

	case VIDEO_FORMAT_I010: {
		size = width * height * 2;
		ALIGN_SIZE(size, alignment);
		offsets[0] = size;
		const uint32_t half_width = (width + 1) / 2;
		const uint32_t half_height = (height + 1) / 2;
		const uint32_t quarter_area = half_width * half_height;
		size += quarter_area * 2;
		ALIGN_SIZE(size, alignment);
		offsets[1] = size;
		size += quarter_area * 2;
		ALIGN_SIZE(size, alignment);
		frame->data[0] = bmalloc(size);
		frame->data[1] = (uint8_t *)frame->data[0] + offsets[0];
		frame->data[2] = (uint8_t *)frame->data[0] + offsets[1];
		frame->linesize[0] = width * 2;
		frame->linesize[1] = half_width * 2;
		frame->linesize[2] = half_width * 2;
		break;
	}

	case VIDEO_FORMAT_P010: {
		size = width * height * 2;
		ALIGN_SIZE(size, alignment);
		offsets[0] = size;
		const uint32_t cbcr_width = (width + 1) & (UINT32_MAX - 1);
		size += cbcr_width * ((height + 1) / 2) * 2;
		ALIGN_SIZE(size, alignment);
		frame->data[0] = bmalloc(size);
		frame->data[1] = (uint8_t *)frame->data[0] + offsets[0];
		frame->linesize[0] = width * 2;
		frame->linesize[1] = cbcr_width * 2;
		break;
	}
	}
}

void video_frame_copy(struct video_frame *dst, const struct video_frame *src,
		      enum video_format format, uint32_t cy)
{
	switch (format) {
	case VIDEO_FORMAT_NONE:
		return;

	case VIDEO_FORMAT_I420:
	case VIDEO_FORMAT_I010:
		memcpy(dst->data[0], src->data[0], src->linesize[0] * cy);
		memcpy(dst->data[1], src->data[1], src->linesize[1] * cy / 2);
		memcpy(dst->data[2], src->data[2], src->linesize[2] * cy / 2);
		break;

	case VIDEO_FORMAT_NV12:
	case VIDEO_FORMAT_P010:
		memcpy(dst->data[0], src->data[0], src->linesize[0] * cy);
		memcpy(dst->data[1], src->data[1], src->linesize[1] * cy / 2);
		break;

	case VIDEO_FORMAT_Y800:
	case VIDEO_FORMAT_YVYU:
	case VIDEO_FORMAT_YUY2:
	case VIDEO_FORMAT_UYVY:
	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
	case VIDEO_FORMAT_BGR3:
	case VIDEO_FORMAT_AYUV:
		memcpy(dst->data[0], src->data[0], src->linesize[0] * cy);
		break;

	case VIDEO_FORMAT_I444:
	case VIDEO_FORMAT_I422:
	case VIDEO_FORMAT_I210:
	case VIDEO_FORMAT_I412:
		memcpy(dst->data[0], src->data[0], src->linesize[0] * cy);
		memcpy(dst->data[1], src->data[1], src->linesize[1] * cy);
		memcpy(dst->data[2], src->data[2], src->linesize[2] * cy);
		break;

	case VIDEO_FORMAT_I40A:
		memcpy(dst->data[0], src->data[0], src->linesize[0] * cy);
		memcpy(dst->data[1], src->data[1], src->linesize[1] * cy / 2);
		memcpy(dst->data[2], src->data[2], src->linesize[2] * cy / 2);
		memcpy(dst->data[3], src->data[3], src->linesize[3] * cy);
		break;

	case VIDEO_FORMAT_I42A:
	case VIDEO_FORMAT_YUVA:
	case VIDEO_FORMAT_YA2L:
		memcpy(dst->data[0], src->data[0], src->linesize[0] * cy);
		memcpy(dst->data[1], src->data[1], src->linesize[1] * cy);
		memcpy(dst->data[2], src->data[2], src->linesize[2] * cy);
		memcpy(dst->data[3], src->data[3], src->linesize[3] * cy);
		break;
	}
}
