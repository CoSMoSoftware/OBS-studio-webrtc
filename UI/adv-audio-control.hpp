#pragma once

#include <obs.hpp>
#include <QWidget>
#include <QPointer>
#include <QDoubleSpinBox>
#include "balance-slider.hpp"

class QGridLayout;
class QLabel;
class QSpinBox;
class QCheckBox;
class QComboBox;

class OBSAdvAudioCtrl : public QObject {
	Q_OBJECT

private:
	OBSSource source;

	QPointer<QWidget> forceMonoContainer;
	QPointer<QWidget> mixerContainer;
	QPointer<QWidget> balanceContainer;

	QPointer<QLabel> nameLabel;
	QPointer<QDoubleSpinBox> volume;
	QPointer<QCheckBox> forceMono;
	QPointer<BalanceSlider> balance;
	QPointer<QLabel> labelL;
	QPointer<QLabel> labelR;
	QPointer<QSpinBox> syncOffset;
	QPointer<QComboBox> monitoringType;
	QPointer<QCheckBox> mixer1;
	QPointer<QCheckBox> mixer2;
	QPointer<QCheckBox> mixer3;
	QPointer<QCheckBox> mixer4;
	QPointer<QCheckBox> mixer5;
	QPointer<QCheckBox> mixer6;

	OBSSignal volChangedSignal;
	OBSSignal syncOffsetSignal;
	OBSSignal flagsSignal;
	OBSSignal mixersSignal;

	static void OBSSourceFlagsChanged(void *param, calldata_t *calldata);
	static void OBSSourceVolumeChanged(void *param, calldata_t *calldata);
	static void OBSSourceSyncChanged(void *param, calldata_t *calldata);
	static void OBSSourceMixersChanged(void *param, calldata_t *calldata);

public:
	OBSAdvAudioCtrl(QGridLayout *layout, obs_source_t *source_);
	virtual ~OBSAdvAudioCtrl();

	inline obs_source_t *GetSource() const { return source; }
	void ShowAudioControl(QGridLayout *layout);

public slots:
	void SourceFlagsChanged(uint32_t flags);
	void SourceVolumeChanged(float volume);
	void SourceSyncChanged(int64_t offset);
	void SourceMixersChanged(uint32_t mixers);

	void volumeChanged(double db);
	void downmixMonoChanged(bool checked);
	void balanceChanged(int val);
	void syncOffsetChanged(int milliseconds);
	void monitoringTypeChanged(int index);
	void mixer1Changed(bool checked);
	void mixer2Changed(bool checked);
	void mixer3Changed(bool checked);
	void mixer4Changed(bool checked);
	void mixer5Changed(bool checked);
	void mixer6Changed(bool checked);
	void ResetBalance();
};
