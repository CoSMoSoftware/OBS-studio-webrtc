#pragma once

#include <memory>
#include <obs.hpp>
#include <QWidget>

class Ui_BrowserSourceToolbar;
class Ui_DeviceSelectToolbar;
class Ui_GameCaptureToolbar;
class Ui_ImageSourceToolbar;
class Ui_ColorSourceToolbar;
class Ui_TextSourceToolbar;

class SourceToolbar : public QWidget {
	Q_OBJECT

	OBSWeakSource weakSource;

protected:
	using properties_delete_t = decltype(&obs_properties_destroy);
	using properties_t =
		std::unique_ptr<obs_properties_t, properties_delete_t>;

	properties_t props;
	OBSDataAutoRelease oldData;

	void SaveOldProperties(obs_source_t *source);
	void SetUndoProperties(obs_source_t *source, bool repeatable = false);

public:
	SourceToolbar(QWidget *parent, OBSSource source);

	OBSSource GetSource() { return OBSGetStrongRef(weakSource); }

public slots:
	virtual void Update() {}
};

class BrowserToolbar : public SourceToolbar {
	Q_OBJECT

	std::unique_ptr<Ui_BrowserSourceToolbar> ui;

public:
	BrowserToolbar(QWidget *parent, OBSSource source);
	~BrowserToolbar();

public slots:
	void on_refresh_clicked();
};

class ComboSelectToolbar : public SourceToolbar {
	Q_OBJECT

protected:
	std::unique_ptr<Ui_DeviceSelectToolbar> ui;
	const char *prop_name;
	bool is_int = false;

public:
	ComboSelectToolbar(QWidget *parent, OBSSource source);
	~ComboSelectToolbar();
	virtual void Init();

public slots:
	void on_device_currentIndexChanged(int idx);
};

class AudioCaptureToolbar : public ComboSelectToolbar {
	Q_OBJECT

public:
	AudioCaptureToolbar(QWidget *parent, OBSSource source);
	void Init() override;
};

class WindowCaptureToolbar : public ComboSelectToolbar {
	Q_OBJECT

public:
	WindowCaptureToolbar(QWidget *parent, OBSSource source);
	void Init() override;
};

class ApplicationAudioCaptureToolbar : public ComboSelectToolbar {
	Q_OBJECT

public:
	ApplicationAudioCaptureToolbar(QWidget *parent, OBSSource source);
	void Init() override;
};

class DisplayCaptureToolbar : public ComboSelectToolbar {
	Q_OBJECT

public:
	DisplayCaptureToolbar(QWidget *parent, OBSSource source);
	void Init() override;
};

class DeviceCaptureToolbar : public QWidget {
	Q_OBJECT

	OBSWeakSource weakSource;

	std::unique_ptr<Ui_DeviceSelectToolbar> ui;
	const char *activateText;
	const char *deactivateText;
	bool active;

public:
	DeviceCaptureToolbar(QWidget *parent, OBSSource source);
	~DeviceCaptureToolbar();

public slots:
	void on_activateButton_clicked();
};

class GameCaptureToolbar : public SourceToolbar {
	Q_OBJECT

	std::unique_ptr<Ui_GameCaptureToolbar> ui;

	void UpdateWindowVisibility();

public:
	GameCaptureToolbar(QWidget *parent, OBSSource source);
	~GameCaptureToolbar();

public slots:
	void on_mode_currentIndexChanged(int idx);
	void on_window_currentIndexChanged(int idx);
};

class ImageSourceToolbar : public SourceToolbar {
	Q_OBJECT

	std::unique_ptr<Ui_ImageSourceToolbar> ui;

public:
	ImageSourceToolbar(QWidget *parent, OBSSource source);
	~ImageSourceToolbar();

public slots:
	void on_browse_clicked();
};

class ColorSourceToolbar : public SourceToolbar {
	Q_OBJECT

	std::unique_ptr<Ui_ColorSourceToolbar> ui;
	QColor color;

	void UpdateColor();

public:
	ColorSourceToolbar(QWidget *parent, OBSSource source);
	~ColorSourceToolbar();

public slots:
	void on_choose_clicked();
};

class TextSourceToolbar : public SourceToolbar {
	Q_OBJECT

	std::unique_ptr<Ui_TextSourceToolbar> ui;
	QFont font;
	QColor color;

public:
	TextSourceToolbar(QWidget *parent, OBSSource source);
	~TextSourceToolbar();

public slots:
	void on_selectFont_clicked();
	void on_selectColor_clicked();
	void on_text_textChanged();
};
