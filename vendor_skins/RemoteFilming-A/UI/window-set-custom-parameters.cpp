#include <QMessageBox>
#include <QScreen>

#include <iostream>

#include <obs.hpp>

#include "window-set-custom-parameters.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"
#include "obs-app.hpp"

#include "ui_SetCustomParametersPage.h"

#include "ui-config.h"

#define wiz reinterpret_cast<SetCustomParameters *>(wizard());

/* ------------------------------------------------------------------------- */

SetCustomParametersPage::SetCustomParametersPage(QWidget *parent)
	: QWizardPage(parent), ui(new Ui_SetCustomParametersPage)
{
	ui->setupUi(this);
	setTitle(QTStr("Set Custom Parameters"));

	connect(ui->stream_name, SIGNAL(textChanged(const QString)), this,
		SLOT(on_stream_name_textChanged()));
	connect(ui->publishing_token, SIGNAL(textChanged(const QString)), this,
		SLOT(on_publishing_token_textChanged()));
	connect(ui->video_frame_rate, SIGNAL(currentIndexChanged(int)), this,
		SLOT(on_video_frame_rate_currentIndexChanged()));

	config_t *profile = obs_frontend_get_profile_config();
	if (!profile) {
		blog(LOG_ERROR, "No profile configuration file");
		return;
	}

	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	obs_service_t *service = main->GetService();
	// stream name
	stream_name_ = std::string(obs_service_get_username(service));
	ui->stream_name->setText(stream_name_.c_str());
	// publishing token
	publishing_token_ = obs_service_get_password(service);
	ui->publishing_token->setText(publishing_token_.c_str());

	// FPS
	fps_ = config_get_string(profile, "Video", "FPSCommon");
	int index = ui->video_frame_rate->findText(fps_.c_str());
	if (index >= 0) {
		ui->video_frame_rate->setCurrentIndex(index);
	}
}

SetCustomParametersPage::~SetCustomParametersPage()
{
	delete ui;
}

void SetCustomParametersPage::on_stream_name_textChanged()
{
	stream_name_ = QT_TO_UTF8(ui->stream_name->text());
}

void SetCustomParametersPage::on_publishing_token_textChanged()
{
	publishing_token_ = QT_TO_UTF8(ui->publishing_token->text());
}

void SetCustomParametersPage::on_video_frame_rate_currentIndexChanged()
{
	fps_ = QT_TO_UTF8(ui->video_frame_rate->currentText());
}

bool SetCustomParametersPage::validatePage()
{
	// if (!ui->frame_rate_button_group->checkedButton()) {
	// 	// FPS: one button must be checked
	// 	return false;
	// }

	return true;
}

/* ------------------------------------------------------------------------- */

SetCustomParameters::SetCustomParameters(QWidget *parent) : QWizard(parent)
{
	EnableThreadedMessageBoxes(true);

	proc_handler_t *ph = obs_get_proc_handler();

	OBSBasic *main = reinterpret_cast<OBSBasic *>(parent);
	main->EnableOutputs(false);

	installEventFilter(CreateShortcutFilter());

#if defined(_WIN32) || defined(__APPLE__)
	setWizardStyle(QWizard::ModernStyle);
#endif

	page_ = new SetCustomParametersPage();
	setPage(0, page_);
	setWindowTitle(QTStr("Set Custom Parameters"));
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	// obs_properties_t *props = obs_get_service_properties("Millicast-WebRTC");
	// obs_property_t *p = obs_properties_get(props, "server");
}

SetCustomParameters::~SetCustomParameters()
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	main->EnableOutputs(true);
	EnableThreadedMessageBoxes(false);
}

void SetCustomParameters::done(int result)
{
	QWizard::done(result);

	if (QDialog::Accepted == result) {
		SaveSettings();
	}
}

void SetCustomParameters::SaveSettings()
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	obs_service_t *service = main->GetService();

	obs_data_t *settings = obs_service_get_settings(service);
	const char *type = obs_service_get_type(service);
	if (strcmp(type, "webrtc_millicast") != 0) {
		blog(LOG_ERROR,
		     "Current streaming service is not set to WebRTC");
		return;
	}

	obs_service_t *oldService = main->GetService();
	OBSData hotkeyData = obs_hotkeys_save_service(oldService);
	obs_data_release(hotkeyData);

	obs_data_set_string(settings, "username", page_->stream_name_.c_str());
	obs_data_set_string(settings, "password",
			    page_->publishing_token_.c_str());
	const char *service_id = obs_data_get_string(settings, "service");
	OBSService newService = obs_service_create(type, "default_service",
						   settings, hotkeyData);
	obs_service_release(newService);

	if (!newService)
		return;

	main->SetService(newService);
	main->SaveService();

	config_t *profile = obs_frontend_get_profile_config();
	if (!profile) {
		blog(LOG_ERROR, "No profile configuration file");
		return;
	}
	config_set_string(main->Config(), "Video", "FPSCommon",
			  page_->fps_.c_str());
	config_save_safe(main->Config(), "tmp", nullptr);
}