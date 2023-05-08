#pragma once

#include <windows.h>
#include <cstdint>
#include <thread>
#include "../shared-memory-queue.h"
#include "../tiny-nv12-scale.h"
#include "../libdshowcapture/source/output-filter.hpp"
#include "../libdshowcapture/source/dshow-formats.hpp"
#include "../../../libobs/util/windows/WinHandle.hpp"
#include "../../../libobs/util/threading-windows.h"

typedef struct {
	int cx;
	int cy;
	nv12_scale_t scaler;
	const uint8_t *source_data;
	uint8_t *scaled_data;
} placeholder_t;

class VCamFilter : public DShow::OutputFilter {
	std::thread th;

	video_queue_t *vq = nullptr;
	int queue_mode = 0;
	bool in_obs = false;
	enum queue_state prev_state = SHARED_QUEUE_STATE_INVALID;
	placeholder_t placeholder;
	uint32_t obs_cx = 0;
	uint32_t obs_cy = 0;
	uint64_t obs_interval = 0;
	uint32_t filter_cx = 0;
	uint32_t filter_cy = 0;
	DShow::VideoFormat format;
	WinHandle thread_start;
	WinHandle thread_stop;
	volatile bool active = false;

	nv12_scale_t scaler = {};

	inline bool stopped() const
	{
		return WaitForSingleObject(thread_stop, 0) != WAIT_TIMEOUT;
	}

	inline uint64_t GetTime();

	void Thread();
	void Frame(uint64_t ts);
	void ShowOBSFrame(uint8_t *ptr);
	void ShowDefaultFrame(uint8_t *ptr);
	void UpdatePlaceholder(void);
	const int GetOutputBufferSize(void);

protected:
	const wchar_t *FilterName() const override;

public:
	VCamFilter();
	~VCamFilter() override;

	STDMETHODIMP Pause() override;
	STDMETHODIMP Run(REFERENCE_TIME tStart) override;
	STDMETHODIMP Stop() override;
};
