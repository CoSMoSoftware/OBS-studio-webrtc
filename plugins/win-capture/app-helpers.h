#pragma once

#include <stdbool.h>

extern bool is_app(HANDLE process);
extern wchar_t *get_app_sid(HANDLE process);
HANDLE create_app_mutex(const wchar_t *sid, const wchar_t *name);
HANDLE open_app_mutex(const wchar_t *sid, const wchar_t *name);
HANDLE open_app_event(const wchar_t *sid, const wchar_t *name);
HANDLE open_app_map(const wchar_t *sid, const wchar_t *name);
