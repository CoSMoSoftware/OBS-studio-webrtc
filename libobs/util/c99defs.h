/*
 * Copyright (c) 2013 Hugh Bailey <obs.jim@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

/*
 * Contains hacks for getting some C99 stuff working in VC, things like
 * bool, stdint
 */

#define UNUSED_PARAMETER(param) (void)param

#ifdef _MSC_VER
#define OBS_DEPRECATED __declspec(deprecated)
#define OBS_NORETURN __declspec(noreturn)
#define FORCE_INLINE __forceinline
#else
#define OBS_DEPRECATED __attribute__((deprecated))
#define OBS_NORETURN __attribute__((noreturn))
#define FORCE_INLINE inline __attribute__((always_inline))
#endif

#if defined(IS_LIBOBS) || defined(SWIG)
#define OBS_EXTERNAL_DEPRECATED
#else
#define OBS_EXTERNAL_DEPRECATED OBS_DEPRECATED
#endif

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
