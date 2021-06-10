/*
Copyright (C) 2014 by Leonhard Oelke <leonhard@in-verted.de>

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
#include <obs-module.h>
#include <obs-nix-platform.h>

#ifdef ENABLE_PIPEWIRE
#include "pipewire-capture.h"
#endif

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("linux-xshm", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
#ifdef ENABLE_PIPEWIRE
	if (obs_get_nix_platform() != OBS_NIX_PLATFORM_X11_GLX)
		return "PipeWire based window/screen capture for X11 and Wayland";
	else
#endif
		return "xcomposite/xshm based window/screen capture for X11";
}

extern struct obs_source_info xshm_input;

extern void xcomposite_load(void);
extern void xcomposite_unload(void);

bool obs_module_load(void)
{
	if (obs_get_nix_platform() == OBS_NIX_PLATFORM_X11_GLX) {
		obs_register_source(&xshm_input);
		xcomposite_load();
#ifdef ENABLE_PIPEWIRE
	} else {
		pipewire_capture_load();
#endif
	}

	return true;
}

void obs_module_unload(void)
{
	if (obs_get_nix_platform() == OBS_NIX_PLATFORM_X11_GLX)
		xcomposite_unload();
#ifdef ENABLE_PIPEWIRE
	else
		pipewire_capture_unload();
#endif
}
