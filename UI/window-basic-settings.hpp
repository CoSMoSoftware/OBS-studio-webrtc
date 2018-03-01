/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>
                          Philippe Groarke <philippe.groarke@gmail.com>

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

#pragma once

#include <util/util.hpp>
#include <QDialog>
#include <memory>
#include <string>

#include <libff/ff-util.h>

#include <obs.h>

class OBSBasic;
class QAbstractButton;
class QComboBox;
class QCheckBox;
class QLabel;
class OBSPropertiesView;
class OBSHotkeyWidget;

#include "ui_OBSBasicSettings.h"

class SilentUpdateCheckBox : public QCheckBox {
	Q_OBJECT

public slots:
	void setCheckedSilently(bool checked)
	{
		bool blocked = blockSignals(true);
		setChecked(checked);
		blockSignals(blocked);
	}
};

class SilentUpdateSpinBox : public QSpinBox {
	Q_OBJECT

public slots:
	void setValueSilently(int val)
	{
		bool blocked = blockSignals(true);
		setValue(val);
		blockSignals(blocked);
	}
};

class OBSFFDeleter
{
public:
	void operator()(const ff_format_desc *format)
	{
		ff_format_desc_free(format);
	}
	void operator()(const ff_codec_desc *codec)
	{
		ff_codec_desc_free(codec);
	}
};
using OBSFFCodecDesc = std::unique_ptr<const ff_codec_desc,
		OBSFFDeleter>;
using OBSFFFormatDesc = std::unique_ptr<const ff_format_desc,
		OBSFFDeleter>;

class OBSBasicSettings : public QDialog {
	Q_OBJECT

private:
	OBSBasic *main;

	std::unique_ptr<Ui::OBSBasicSettings> ui;

	bool generalChanged = false;
	bool stream1Changed = false;
	bool outputsChanged = false;
	bool audioChanged = false;
	bool videoChanged = false;
	bool hotkeysChanged = false;
	bool advancedChanged = false;
	int  pageIndex = 0;
	bool loading = true;
	std::string savedTheme;

	int lastSimpleRecQualityIdx = 0;

	OBSFFFormatDesc formats;

	OBSPropertiesView *streamProperties = nullptr;
	OBSPropertiesView *streamEncoderProps = nullptr;
	OBSPropertiesView *recordEncoderProps = nullptr;

	QPointer<QLabel> advOutRecWarning;
	QPointer<QLabel> simpleOutRecWarning;

	QString curPreset;
	QString curQSVPreset;
	QString curNVENCPreset;
	QString curAMDPreset;

	QString curAdvStreamEncoder;
	QString curAdvRecordEncoder;

	using AudioSource_t =
		std::tuple<OBSWeakSource,
			QPointer<QCheckBox>, QPointer<QSpinBox>,
			QPointer<QCheckBox>, QPointer<QSpinBox>>;
	std::vector<AudioSource_t> audioSources;
	std::vector<OBSSignal> audioSourceSignals;
	OBSSignal sourceCreated;
	OBSSignal channelChanged;

	std::vector<std::pair<bool, QPointer<OBSHotkeyWidget>>> hotkeys;
	OBSSignal hotkeyRegistered;
	OBSSignal hotkeyUnregistered;

	uint32_t outputCX = 0;
	uint32_t outputCY = 0;

	void SaveCombo(QComboBox *widget, const char *section,
			const char *value);
	void SaveComboData(QComboBox *widget, const char *section,
			const char *value);
	void SaveCheckBox(QAbstractButton *widget, const char *section,
			const char *value, bool invert = false);
	void SaveEdit(QLineEdit *widget, const char *section,
			const char *value);
	void SaveSpinBox(QSpinBox *widget, const char *section,
			const char *value);
	void SaveFormat(QComboBox *combo);
	void SaveEncoder(QComboBox *combo, const char *section,
			const char *value);

	inline bool Changed() const
	{
		return generalChanged || outputsChanged || stream1Changed ||
			audioChanged || videoChanged || advancedChanged ||
			hotkeysChanged;
	}

	inline void EnableApplyButton(bool en)
	{
		ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(en);
	}

	inline void ClearChanged()
	{
		generalChanged = false;
		stream1Changed = false;
		outputsChanged = false;
		audioChanged   = false;
		videoChanged   = false;
		hotkeysChanged = false;
		advancedChanged= false;
		EnableApplyButton(false);
	}

#ifdef _WIN32
	bool aeroWasDisabled = false;
	QCheckBox *toggleAero = nullptr;
	void ToggleDisableAero(bool checked);
#endif

	void HookWidget(QWidget *widget, const char *signal, const char *slot);

	bool QueryChanges();

	void LoadServiceTypes();
	void LoadEncoderTypes();
	void LoadColorRanges();
	void LoadFormats();
	void ReloadCodecs(const ff_format_desc *formatDesc);

	void LoadGeneralSettings();
	void LoadStream1Settings();
	void LoadOutputSettings();
	void LoadAudioSettings();
	void LoadVideoSettings();
	void LoadHotkeySettings(obs_hotkey_id ignoreKey=OBS_INVALID_HOTKEY_ID);
	void LoadAdvancedSettings();
	void LoadSettings(bool changedOnly);

	OBSPropertiesView *CreateEncoderPropertyView(const char *encoder,
			const char *path, bool changed = false);

	/* general */
	void LoadLanguageList();
	void LoadThemeList();

	/* output */
	void LoadSimpleOutputSettings();
	void LoadAdvOutputStreamingSettings();
	void LoadAdvOutputStreamingEncoderProperties();
	void LoadAdvOutputRecordingSettings();
	void LoadAdvOutputRecordingEncoderProperties();
	void LoadAdvOutputFFmpegSettings();
	void LoadAdvOutputAudioSettings();
	void SetAdvOutputFFmpegEnablement(
		ff_codec_type encoderType, bool enabled,
		bool enableEncode = false);

	/* audio */
	void LoadListValues(QComboBox *widget, obs_property_t *prop, int index);
	void LoadAudioDevices();
	void LoadAudioSources();

	/* video */
	void LoadRendererList();
	void ResetDownscales(uint32_t cx, uint32_t cy);
	void LoadDownscaleFilters();
	void LoadResolutionLists();
	void LoadFPSData();

	void SaveGeneralSettings();
	void SaveStream1Settings();
	void SaveOutputSettings();
	void SaveAudioSettings();
	void SaveVideoSettings();
	void SaveHotkeySettings();
	void SaveAdvancedSettings();
	void SaveSettings();

	void UpdateSimpleOutStreamDelayEstimate();
	void UpdateAdvOutStreamDelayEstimate();

	void FillSimpleRecordingValues();
	void FillSimpleStreamingValues();
	void FillAudioMonitoringDevices();

	void RecalcOutputResPixels(const char *resText);

private slots:
	void on_theme_activated(int idx);

	void on_listWidget_itemSelectionChanged();
	void on_buttonBox_clicked(QAbstractButton *button);

	void on_streamType_currentIndexChanged(int idx);
	void on_simpleOutputBrowse_clicked();
	void on_advOutRecPathBrowse_clicked();
	void on_advOutFFPathBrowse_clicked();
	void on_advOutEncoder_currentIndexChanged(int idx);
	void on_advOutRecEncoder_currentIndexChanged(int idx);
	void on_advOutFFIgnoreCompat_stateChanged(int state);
	void on_advOutFFFormat_currentIndexChanged(int idx);
	void on_advOutFFAEncoder_currentIndexChanged(int idx);
	void on_advOutFFVEncoder_currentIndexChanged(int idx);
	void on_advOutFFType_currentIndexChanged(int idx);

	void on_colorFormat_currentIndexChanged(const QString &text);

	void on_filenameFormatting_textEdited(const QString &text);
	void on_outputResolution_editTextChanged(const QString &text);
	void on_baseResolution_editTextChanged(const QString &text);

	void on_disableOSXVSync_clicked();

	void GeneralChanged();
	void AudioChanged();
	void AudioChangedRestart();
	void ReloadAudioSources();
	void OutputsChanged();
	void Stream1Changed();
	void VideoChanged();
	void VideoChangedResolution();
	void VideoChangedRestart();
	void HotkeysChanged();
	void ReloadHotkeys(obs_hotkey_id ignoreKey=OBS_INVALID_HOTKEY_ID);
	void AdvancedChanged();
	void AdvancedChangedRestart();

	void UpdateStreamDelayEstimate();

	void UpdateAutomaticReplayBufferCheckboxes();

	void AdvOutRecCheckWarnings();

	void SimpleRecordingQualityChanged();
	void SimpleRecordingEncoderChanged();
	void SimpleRecordingQualityLosslessWarning(int idx);

	void SimpleReplayBufferChanged();

	void SimpleStreamingEncoderChanged();

protected:
	virtual void closeEvent(QCloseEvent *event);

public:
	OBSBasicSettings(QWidget *parent);
	~OBSBasicSettings();
};
