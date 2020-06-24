/* Copyright Dr. Alex. Gouaillard (2015, 2020) */

#pragma once

#include <QStatusBar>
#include <QPointer>
#include <QTimer>
#include <QPlainTextEdit>
#include <util/platform.h>
#include <obs.h>

class QLabel;

class OBSBasicStatusBar : public QStatusBar {
	Q_OBJECT

private:
	QLabel *delayInfo;
	QLabel *droppedFrames;
	QLabel *streamTime;
  // NOTE LUDO: #193 remove recording time information from main window
	// QLabel *recordTime;
	QLabel *cpuUsage;
	QLabel *kbps;
	QLabel *statusSquare;
  // NOTE LUDO: #80 add getStats
  QPlainTextEdit *getstatsTextBox;

	obs_output_t *streamOutput = nullptr;
	obs_output_t *recordOutput = nullptr;
	bool active = false;
	bool overloadedNotify = true;

	int retries = 0;
	int totalStreamSeconds = 0;
	int totalRecordSeconds = 0;

	int reconnectTimeout = 0;

	int delaySecTotal = 0;
	int delaySecStarting = 0;
	int delaySecStopping = 0;

	int startSkippedFrameCount = 0;
	int startTotalFrameCount = 0;
	int lastSkippedFrameCount = 0;

	int statsUpdateSeconds = 0;
	uint64_t lastBytesSent = 0;
	uint64_t lastBytesSentTime = 0;

	QPixmap transparentPixmap;
	QPixmap greenPixmap;
	QPixmap grayPixmap;
	QPixmap redPixmap;

	float lastCongestion = 0.0f;

	QPointer<QTimer> refreshTimer;

	obs_output_t *GetOutput();

	void Activate();
	void Deactivate();

	void UpdateDelayMsg();
  // NOTE LUDO: #80 add getStats
  void UpdateStats();
	void UpdateBandwidth();
	void UpdateStreamTime();
	void UpdateRecordTime();
	void UpdateDroppedFrames();

	static void OBSOutputReconnect(void *data, calldata_t *params);
	static void OBSOutputReconnectSuccess(void *data, calldata_t *params);

private slots:
	void Reconnect(int seconds);
	void ReconnectSuccess();
	void UpdateStatusBar();
	void UpdateCPUUsage();

public:
	OBSBasicStatusBar(QWidget *parent);

	void StreamDelayStarting(int sec);
	void StreamDelayStopping(int sec);
	void StreamStarted(obs_output_t *output);
	void StreamStopped();
	void RecordingStarted(obs_output_t *output);
	void RecordingStopped();

	void ReconnectClear();
};
