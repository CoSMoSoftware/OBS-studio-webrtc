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

#define DEFAULT_CX 1920
#define DEFAULT_CY 1080
#define DEFAULT_INTERVAL 333333ULL

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
	uint32_t obs_cx = DEFAULT_CX;
	uint32_t obs_cy = DEFAULT_CY;
	uint64_t obs_interval = DEFAULT_INTERVAL;
	uint32_t filter_cx = DEFAULT_CX;
	uint32_t filter_cy = DEFAULT_CY;
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
