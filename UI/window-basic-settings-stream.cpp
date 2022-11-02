// Copyright Dr. Alex. Gouaillard (2015, 2020)
#include <QMessageBox>
#include <QUrl>
#include <QToolTip>

#include "window-basic-settings.hpp"
#include "obs-frontend-api.h"
#include "obs-app.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"
#include "url-push-button.hpp"

#ifdef BROWSER_AVAILABLE
#include <browser-panel.hpp>
#endif

#include "auth-oauth.hpp"

#include "ui-config.h"

#if YOUTUBE_ENABLED
#include "youtube-api-wrappers.hpp"
#endif

struct QCef;
struct QCefCookieManager;

extern QCef *cef;
extern QCefCookieManager *panel_cookies;

// #289 service list of radio buttons
// 0 = Millicast-WebRTC (Millicast)
// 1 = Millicast-RTMP   (Custom)
enum class ListOpt : int { Millicast = 0, Custom }; // CustomWebrtc

enum class Section : int {
	Connect,
	StreamKey,
};

std::vector<std::string> webrtc_services = {"webrtc_millicast",
					    "webrtc_custom"};
std::vector<std::string>::size_type webrtc_count = webrtc_services.size();

inline bool OBSBasicSettings::IsCustomService() const
{
	// #289 service list of radio buttons
	return ui->serviceButtonGroup->checkedId() == (int)ListOpt::Custom;
}

inline bool OBSBasicSettings::IsWebRTC() const
{
	return ui->serviceButtonGroup->checkedId() == (int)ListOpt::Millicast;
}

// #289 service list of radio buttons
inline int OBSBasicSettings::GetServiceIndex() const
{
	if (-1 == ui->serviceButtonGroup->checkedId())
		return 0;
	return ui->serviceButtonGroup->checkedId();
}

void OBSBasicSettings::InitStreamPage()
{
	ui->connectAccount2->setVisible(false);
	ui->disconnectAccount->setVisible(false);
	ui->bandwidthTestEnable->setVisible(false);

	ui->twitchAddonDropdown->setVisible(false);
	ui->twitchAddonLabel->setVisible(false);

	ui->connectedAccountLabel->setVisible(false);
	ui->connectedAccountText->setVisible(false);

	int vertSpacing = ui->topStreamLayout->verticalSpacing();

	QMargins m = ui->topStreamLayout->contentsMargins();
	m.setBottom(vertSpacing / 2);
	ui->topStreamLayout->setContentsMargins(m);

	m = ui->loginPageLayout->contentsMargins();
	m.setTop(vertSpacing / 2);
	ui->loginPageLayout->setContentsMargins(m);

	m = ui->streamkeyPageLayout->contentsMargins();
	m.setTop(vertSpacing / 2);
	ui->streamkeyPageLayout->setContentsMargins(m);

	// #289 service list of radio buttons
	// LoadServices(false);

	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.None"));
	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.BTTV"));
	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.FFZ"));
	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.Both"));

	// #289 service list of radio buttons
	connect(ui->serviceButtonGroup, SIGNAL(clicked(bool)), this,
		SLOT(UpdateServerList()));
	connect(ui->serviceButtonGroup, SIGNAL(clicked(bool)), this,
		SLOT(UpdateKeyLink()));
	connect(ui->serviceButtonGroup, SIGNAL(clicked(bool)), this,
		SLOT(UpdateVodTrackSetting()));
	connect(ui->serviceButtonGroup, SIGNAL(clicked(bool)), this,
		SLOT(UpdateServiceRecommendations()));
	// #289 service list of radio buttons
	// connect(ui->serviceButtonGroup, SIGNAL(clicked(bool)), this,
	//   SLOT(UpdateResFPSLimits()));
	connect(ui->customServer, SIGNAL(textChanged(const QString &)), this,
		SLOT(UpdateKeyLink()));
	// #289 service list of radio buttons
	// connect(ui->ignoreRecommended, SIGNAL(clicked(bool)), this,
	// 	SLOT(DisplayEnforceWarning(bool)));
	// connect(ui->ignoreRecommended, SIGNAL(toggled(bool)), this,
	// 	SLOT(UpdateResFPSLimits()));
	connect(ui->customServer, SIGNAL(editingFinished(const QString &)),
		this, SLOT(UpdateKeyLink()));
	// #289 service list of radio buttons
	// connect(ui->serviceButtonGroup, SIGNAL(clicked(bool)), this,
	// 	SLOT(UpdateMoreInfoLink()));
	connect(ui->serviceGroupBox, SIGNAL(clicked(bool)), this,
		SLOT(UpdateMoreInfoLink()));
}

void OBSBasicSettings::LoadStream1Settings()
{
	// #289 service list of radio buttons
	// bool ignoreRecommended =
	// 	config_get_bool(main->Config(), "Stream1", "IgnoreRecommended");

	obs_service_t *service_obj = main->GetService();
	const char *type = obs_service_get_type(service_obj);

	loading = true;

	OBSDataAutoRelease settings = obs_service_get_settings(service_obj);

	// #289 service list of radio buttons
	const char *tmpString = nullptr;
	tmpString = obs_data_get_string(settings, "service");
	const char *service = strcmp("", tmpString) == 0 ? "Millicast-WebRTC"
							 : tmpString;

	const char *server = obs_data_get_string(settings, "server");
	const char *key = obs_data_get_string(settings, "key");

	if (strcmp(type, "rtmp_custom") == 0) {
		// #289 service list of radio buttons
		QList<QAbstractButton *> listButtons =
			ui->serviceButtonGroup->buttons();
		for (QList<QAbstractButton *>::iterator iter =
			     listButtons.begin();
		     iter != listButtons.end(); ++iter) {
			QRadioButton *radiobutton =
				reinterpret_cast<QRadioButton *>(*iter);
			if (strcmp(service,
				   radiobutton->text().toStdString().c_str()) ==
			    0) {
				radiobutton->setChecked(true);
				break;
			}
		}
		ui->customServer->setText(server);

		bool use_auth = obs_data_get_bool(settings, "use_auth");
		const char *username =
			obs_data_get_string(settings, "username");
		const char *password =
			obs_data_get_string(settings, "password");
		ui->authUsername->setText(QT_UTF8(username));
		ui->authPw->setText(QT_UTF8(password));
		ui->useAuth->setChecked(use_auth);

		// NOTE LUDO: #172 codecs list of radio buttons
		tmpString = obs_data_get_string(settings, "codec");
		const char *codec = strcmp("", tmpString) == 0 ? "vp9"
							       : tmpString;
		listButtons = ui->codecButtonGroup->buttons();
		for (QList<QAbstractButton *>::iterator iter =
			     listButtons.begin();
		     iter != listButtons.end(); ++iter) {
			QRadioButton *radiobutton =
				reinterpret_cast<QRadioButton *>(*iter);
			if (strcmp(codec,
				   radiobutton->text().toStdString().c_str()) ==
			    0) {
				radiobutton->setChecked(true);
				break;
			}
		}

		const char *room = obs_data_get_string(settings, "room");
		ui->room->setText(QT_UTF8(room));

		tmpString = obs_data_get_string(settings, "protocol");
		const char *protocol = strcmp("", tmpString) == 0 ? "Automatic"
								  : tmpString;
		int idxP = ui->streamProtocol->findText(protocol);
		ui->streamProtocol->setCurrentIndex(idxP);

		bool simulcast = obs_data_get_bool(settings, "simulcast");
		ui->simulcastEnable->setChecked(simulcast);

		bool multisource = obs_data_get_bool(settings, "multisource");
		ui->multisourceEnable->setChecked(multisource);

		const char *sourceId =
			obs_data_get_string(settings, "sourceId");
		ui->sourceId->setText(sourceId);

		const char *publish_api_url =
			obs_data_get_string(settings, "publish_api_url");
		ui->publishApiUrl->setText(publish_api_url);

	} else if (strcmp(type, "rtmp_common") == 0) {
		// #289 service list of radio buttons
		QList<QAbstractButton *> listButtons =
			ui->serviceButtonGroup->buttons();
		for (QList<QAbstractButton *>::iterator iter =
			     listButtons.begin();
		     iter != listButtons.end(); ++iter) {
			QRadioButton *radiobutton =
				reinterpret_cast<QRadioButton *>(*iter);
			if (strcmp(service,
				   radiobutton->text().toStdString().c_str()) ==
			    0) {
				radiobutton->setChecked(true);
				break;
			}
		}

		bool bw_test = obs_data_get_bool(settings, "bwtest");
		ui->bandwidthTestEnable->setChecked(bw_test);

		int idx =
			config_get_int(main->Config(), "Twitch", "AddonChoice");
		ui->twitchAddonDropdown->setCurrentIndex(idx);
	} else {
		const char *room = obs_data_get_string(settings, "room");
		ui->room->setText(QT_UTF8(room));

		const char *username =
			obs_data_get_string(settings, "username");
		ui->authUsername->setText(QT_UTF8(username));

		const char *password =
			obs_data_get_string(settings, "password");
		ui->authPw->setText(QT_UTF8(password));

		tmpString = obs_data_get_string(settings, "codec");
		// NOTE LUDO: #172 codecs list of radio buttons
		// const char *codec = strcmp("", tmpString) == 0 ? "Automatic"
		const char *codec = strcmp("", tmpString) == 0 ? "vp9"
							       : tmpString;
		// int idxC = ui->codec->findText(codec);
		// ui->codec->setCurrentIndex(idxC);
		QList<QAbstractButton *> listButtons =
			ui->codecButtonGroup->buttons();
		for (QList<QAbstractButton *>::iterator iter =
			     listButtons.begin();
		     iter != listButtons.end(); ++iter) {
			QRadioButton *radiobutton =
				reinterpret_cast<QRadioButton *>(*iter);
			if (strcmp(codec,
				   radiobutton->text().toStdString().c_str()) ==
			    0) {
				radiobutton->setChecked(true);
				break;
			}
		}

		tmpString = obs_data_get_string(settings, "protocol");
		const char *protocol = strcmp("", tmpString) == 0 ? "Automatic"
								  : tmpString;
		int idxP = ui->streamProtocol->findText(protocol);
		ui->streamProtocol->setCurrentIndex(idxP);

		bool simulcast = obs_data_get_bool(settings, "simulcast");
		ui->simulcastEnable->setChecked(simulcast);

		bool multisource = obs_data_get_bool(settings, "multisource");
		ui->multisourceEnable->setChecked(multisource);

		const char *sourceId =
			obs_data_get_string(settings, "sourceId");
		ui->sourceId->setText(sourceId);

		const char *publish_api_url =
			obs_data_get_string(settings, "publish_api_url");
		ui->publishApiUrl->setText(publish_api_url);

		// #289 service list of radio buttons
		listButtons = ui->serviceButtonGroup->buttons();
		for (QList<QAbstractButton *>::iterator iter =
			     listButtons.begin();
		     iter != listButtons.end(); ++iter) {
			QRadioButton *radiobutton =
				reinterpret_cast<QRadioButton *>(*iter);
			if (strcmp(service,
				   radiobutton->text().toStdString().c_str()) ==
			    0) {
				radiobutton->setChecked(true);
				break;
			}
		}

		ui->customServer->setText(server);
		bool use_auth = true;
		ui->useAuth->setChecked(use_auth);
	}

	UpdateServerList();

	if (strcmp(type, "rtmp_common") == 0) {
		int idx = ui->server->findData(server);
		if (idx == -1) {
			if (server && *server)
				ui->server->insertItem(0, server, server);
			idx = 0;
		}
		ui->server->setCurrentIndex(idx);
	}

	ui->key->setText(key);

	lastService.clear();
	on_service_currentIndexChanged(0);

	UpdateKeyLink();
	UpdateMoreInfoLink();
	UpdateVodTrackSetting();
	UpdateServiceRecommendations();

	bool streamActive = obs_frontend_streaming_active();
	ui->streamPage->setEnabled(!streamActive);

	// #289 service list of radio buttons
	// ui->ignoreRecommended->setChecked(ignoreRecommended);

	loading = false;

	// #289 service list of radio buttons
	// QMetaObject::invokeMethod(this, "UpdateResFPSLimits",
	// 			  Qt::QueuedConnection);
}

void OBSBasicSettings::SaveStream1Settings()
{
	bool customServer = IsCustomService();
	bool isWebrtc = IsWebRTC();

	// #289 service list of radio buttons
	const char *service_id =
		!isWebrtc ? customServer ? "rtmp_custom" : "rtmp_common"
			  : webrtc_services[GetServiceIndex()].c_str();

	obs_service_t *oldService = main->GetService();
	OBSDataAutoRelease hotkeyData = obs_hotkeys_save_service(oldService);

	OBSDataAutoRelease settings = obs_data_create();

	// #289 service list of radio buttons
	obs_data_set_string(
		settings, "service",
		QT_TO_UTF8(ui->serviceButtonGroup->checkedButton()->text()));
	if (!customServer && !isWebrtc) {
		obs_data_set_string(
			settings, "server",
			QT_TO_UTF8(ui->server->currentData().toString()));
	} else if (customServer && !isWebrtc) {
		obs_data_set_string(settings, "server",
				    QT_TO_UTF8(ui->customServer->text()));
		obs_data_set_bool(settings, "use_auth",
				  ui->useAuth->isChecked());
		obs_data_set_string(settings, "username",
				    QT_TO_UTF8(ui->authUsername->text()));
		obs_data_set_string(settings, "password",
				    QT_TO_UTF8(ui->authPw->text()));
		obs_data_set_string(settings, "room",
				    QT_TO_UTF8(ui->room->text()));
		obs_data_set_string(
			settings, "protocol",
			QT_TO_UTF8(ui->streamProtocol->currentText()));
		obs_data_set_bool(settings, "simulcast",
				  ui->simulcastEnable->isChecked());
		obs_data_set_bool(settings, "multisource",
				  ui->multisourceEnable->isChecked());
		obs_data_set_string(settings, "sourceId",
				    QT_TO_UTF8(ui->sourceId->text()));
		obs_data_set_string(settings, "publish_api_url",
				    QT_TO_UTF8(ui->publishApiUrl->text()));
	} else if (isWebrtc) {
		obs_data_set_string(settings, "server",
				    QT_TO_UTF8(ui->customServer->text()));
		obs_data_set_string(settings, "room",
				    QT_TO_UTF8(ui->room->text()));
		obs_data_set_bool(settings, "use_auth",
				  ui->useAuth->isChecked());
		obs_data_set_string(settings, "username",
				    QT_TO_UTF8(ui->authUsername->text()));
		obs_data_set_string(settings, "password",
				    QT_TO_UTF8(ui->authPw->text()));
		obs_data_set_string(
			settings, "protocol",
			QT_TO_UTF8(ui->streamProtocol->currentText()));
		obs_data_set_bool(settings, "simulcast",
				  ui->simulcastEnable->isChecked());
		obs_data_set_bool(settings, "multisource",
				  ui->multisourceEnable->isChecked());
		obs_data_set_string(settings, "sourceId",
				    QT_TO_UTF8(ui->sourceId->text()));
		obs_data_set_string(settings, "publish_api_url",
				    QT_TO_UTF8(ui->publishApiUrl->text()));
	}

	obs_data_set_string(
		settings, "codec",
		// NOTE LUDO: #172 codecs list of radio buttons
		// QT_TO_UTF8(ui->codec->currentText()));
		QT_TO_UTF8(ui->codecButtonGroup->checkedButton()->text()));

	if (!!auth && strcmp(auth->service(), "Twitch") == 0) {
		bool choiceExists = config_has_user_value(
			main->Config(), "Twitch", "AddonChoice");
		int currentChoice =
			config_get_int(main->Config(), "Twitch", "AddonChoice");
		int newChoice = ui->twitchAddonDropdown->currentIndex();

		config_set_int(main->Config(), "Twitch", "AddonChoice",
			       newChoice);

		if (choiceExists && currentChoice != newChoice)
			forceAuthReload = true;

		obs_data_set_bool(settings, "bwtest",
				  ui->bandwidthTestEnable->isChecked());
	} else {
		obs_data_set_bool(settings, "bwtest", false);
	}

	obs_data_set_string(settings, "key", QT_TO_UTF8(ui->key->text()));

	OBSServiceAutoRelease newService = obs_service_create(
		service_id, "default_service", settings, hotkeyData);

	if (!newService)
		return;

	main->SetService(newService);
	main->SaveService();
	main->auth = auth;
	if (!!main->auth) {
		main->auth->LoadUI();
		main->SetBroadcastFlowEnabled(main->auth->broadcastFlow());
	} else {
		main->SetBroadcastFlowEnabled(false);
	}

	// SaveCheckBox(ui->ignoreRecommended, "Stream1", "IgnoreRecommended");
}

void OBSBasicSettings::CheckSimulcastApplicableToCodec()
{
	std::string codec =
		QT_TO_UTF8(ui->codecButtonGroup->checkedButton()->text());
	if ("VP9" == codec || "AV1" == codec) {
		QString errorMessage =
			tr("Error: Simulcast is not applicable to %1\nDisabling simulcast")
				.arg(ui->codecButtonGroup->checkedButton()
					     ->text());
		QToolTip::showText(QCursor::pos(), errorMessage, this);
		ui->simulcastEnable->setCheckState(Qt::Unchecked);
	}
}

void OBSBasicSettings::on_video_codec_changed()
{
	if (Qt::Unchecked == ui->simulcastEnable->checkState()) {
		return;
	}

	CheckSimulcastApplicableToCodec();
}

void OBSBasicSettings::on_simulcast_box_checked()
{
	if (Qt::Unchecked == ui->simulcastEnable->checkState()) {
		// User has unchecked box
		return;
	}

	// User has checked box
	CheckSimulcastApplicableToCodec();
}

void OBSBasicSettings::UpdateMoreInfoLink()
{
	if (IsCustomService()) {
		ui->moreInfoButton->hide();
		return;
	}

	bool custom = IsCustomService();
	bool isWebrtc = IsWebRTC();
	// #289 service list of radio buttons
	QString serviceName;
	if (nullptr != ui->serviceButtonGroup->checkedButton()) {
		serviceName = ui->serviceButtonGroup->checkedButton()->text();
	} else {
		serviceName = "";
	}

	if (custom || isWebrtc)
		serviceName = "";

	obs_properties_t *props = obs_get_service_properties("rtmp_common");
	obs_property_t *services = obs_properties_get(props, "service");

	OBSDataAutoRelease settings = obs_data_create();

	obs_data_set_string(settings, "service", QT_TO_UTF8(serviceName));
	obs_property_modified(services, settings);

	const char *more_info_link =
		obs_data_get_string(settings, "more_info_link");

	if (!more_info_link || (*more_info_link == '\0')) {
		ui->moreInfoButton->hide();
	} else {
		ui->moreInfoButton->setTargetUrl(QUrl(more_info_link));
		ui->moreInfoButton->show();
	}
	obs_properties_destroy(props);
}

void OBSBasicSettings::UpdateKeyLink()
{
	// #289 service list of radio buttons
	QString serviceName;
	if (nullptr != ui->serviceButtonGroup->checkedButton()) {
		serviceName = ui->serviceButtonGroup->checkedButton()->text();
	} else {
		serviceName = "";
	}
	QString customServer = ui->customServer->text();
	QString streamKeyLink;

	obs_properties_t *props = obs_get_service_properties("rtmp_common");
	obs_property_t *services = obs_properties_get(props, "service");

	OBSDataAutoRelease settings = obs_data_create();

	obs_data_set_string(settings, "service", QT_TO_UTF8(serviceName));
	obs_property_modified(services, settings);

	streamKeyLink = obs_data_get_string(settings, "stream_key_link");

	if (customServer.contains("fbcdn.net") && IsCustomService()) {
		streamKeyLink =
			"https://www.facebook.com/live/producer?ref=OBS";
	}

	if (serviceName == "Dacast") {
		ui->streamKeyLabel->setText(
			QTStr("Basic.AutoConfig.StreamPage.EncoderKey"));
	} else {
		ui->streamKeyLabel->setText(
			QTStr("Basic.AutoConfig.StreamPage.StreamKey"));
	}

	if (QString(streamKeyLink).isNull() ||
	    QString(streamKeyLink).isEmpty()) {
		ui->getStreamKeyButton->hide();
	} else {
		ui->getStreamKeyButton->setTargetUrl(QUrl(streamKeyLink));
		ui->getStreamKeyButton->show();
	}
	obs_properties_destroy(props);
}

// #289 service list of radio buttons
// void OBSBasicSettings::LoadServices(bool showAll)
// {
// 	obs_properties_t *props = obs_get_service_properties("rtmp_common");

// 	OBSDataAutoRelease settings = obs_data_create();

// 	obs_data_set_bool(settings, "show_all", showAll);

// 	obs_property_t *prop = obs_properties_get(props, "show_all");
// 	obs_property_modified(prop, settings);

// 	ui->service->blockSignals(true);
// 	ui->service->clear();

// 	QStringList names;

// 	obs_property_t *services = obs_properties_get(props, "service");
// 	size_t services_count = obs_property_list_item_count(services);
// 	for (size_t i = 0; i < services_count; i++) {
// 		const char *name = obs_property_list_item_string(services, i);
// 		names.push_back(name);
// 	}

// 	if (showAll)
// 		names.sort(Qt::CaseInsensitive);

// 	for (QString &name : names)
// 		ui->service->addItem(name);

// 	if (!showAll) {
// 		ui->service->addItem(
// 			QTStr("Basic.AutoConfig.StreamPage.Service.ShowAll"),
// 			QVariant((int)ListOpt::ShowAll));
// 	}

// 	for (std::vector<std::string>::size_type i = webrtc_count; i-- > 0;) {
// 		ui->service->insertItem(0,
// 					obs_service_get_display_name(
// 						webrtc_services[i].c_str()),
// 					QVariant((int)i + 3));
// 	}

// 	ui->service->insertItem(
// 		0, QTStr("Basic.AutoConfig.StreamPage.Service.Custom"),
// 		QVariant((int)ListOpt::Custom));

// 	if (!lastService.isEmpty()) {
// 		int idx = ui->service->findText(lastService);
// 		if (idx != -1)
// 			ui->service->setCurrentIndex(idx);
// 	}

// 	obs_properties_destroy(props);

// 	ui->service->blockSignals(false);
// }

static inline bool is_auth_service(const std::string &service)
{
	return Auth::AuthType(service) != Auth::Type::None;
}

static inline bool is_external_oauth(const std::string &service)
{
	return Auth::External(service);
}

static void reset_service_ui_fields(Ui::OBSBasicSettings *ui,
				    std::string &service, bool loading)
{
	bool external_oauth = is_external_oauth(service);
	if (external_oauth) {
		ui->streamKeyWidget->setVisible(false);
		ui->streamKeyLabel->setVisible(false);
		ui->connectAccount2->setVisible(false);
		ui->useStreamKeyAdv->setVisible(false);
		ui->streamStackWidget->setCurrentIndex((int)Section::StreamKey);
	} else if (cef) {
		QString key = ui->key->text();
		bool can_auth = is_auth_service(service);
		int page = can_auth && (!loading || key.isEmpty())
				   ? (int)Section::Connect
				   : (int)Section::StreamKey;

		ui->streamStackWidget->setCurrentIndex(page);
		ui->streamKeyWidget->setVisible(true);
		ui->streamKeyLabel->setVisible(true);
		ui->connectAccount2->setVisible(can_auth);
		ui->useStreamKeyAdv->setVisible(false);
	} else {
		ui->connectAccount2->setVisible(false);
		ui->useStreamKeyAdv->setVisible(false);
	}

	ui->connectedAccountLabel->setVisible(false);
	ui->connectedAccountText->setVisible(false);
	ui->disconnectAccount->setVisible(false);
}

#if YOUTUBE_ENABLED
static void get_yt_ch_title(Ui::OBSBasicSettings *ui)
{
	const char *name = config_get_string(OBSBasic::Get()->Config(),
					     "YouTube", "ChannelName");
	if (name) {
		ui->connectedAccountText->setText(name);
	} else {
		// if we still not changed the service page
		if (IsYouTubeService(QT_TO_UTF8(ui->service->currentText()))) {
			ui->connectedAccountText->setText(
				QTStr("Auth.LoadingChannel.Error"));
		}
	}
}
#endif

void OBSBasicSettings::UseStreamKeyAdvClicked()
{
	ui->streamKeyWidget->setVisible(true);
}

void OBSBasicSettings::on_service_currentIndexChanged(int)
{
	// #289 service list of radio buttons
	// bool showMore = ui->service->currentData().toInt() ==
	// 		(int)ListOpt::ShowAll;
	// if (showMore)
	// 	return;

	// #289 service list of radio buttons
	std::string service =
		QT_TO_UTF8(ui->serviceButtonGroup->checkedButton()->text());
	bool custom = IsCustomService();
	bool isWebrtc = IsWebRTC();

	ui->disconnectAccount->setVisible(false);
	ui->bandwidthTestEnable->setVisible(false);
	ui->twitchAddonDropdown->setVisible(false);
	ui->twitchAddonLabel->setVisible(false);

	if (lastService != service.c_str()) {
		reset_service_ui_fields(ui.get(), service, loading);
	}

	// #289 service list of radio buttons
	ui->useAuth->setVisible(false);
	ui->authUsernameLabel->setVisible(isWebrtc);
	ui->authUsername->setVisible(isWebrtc);
	ui->authPwLabel->setVisible(false);
	ui->authPwWidget->setVisible(false);

	if (custom && !isWebrtc) {
		ui->streamkeyPageLayout->insertRow(1, ui->serverLabel,
						   ui->serverStackedWidget);

		ui->serverLabel->setVisible(true);
		ui->serverLabel->setText("Server");
		ui->serverStackedWidget->setCurrentIndex(1);
		ui->serverStackedWidget->setVisible(true);
		ui->streamKeyLabel->setVisible(true);
		ui->streamKeyWidget->setVisible(true);
		ui->roomLabel->setVisible(false);
		ui->room->setVisible(false);
		// on_useAuth_toggled();
		ui->codecLabel->setVisible(false);
		// NOTE LUDO: #172 codecs list of radio buttons
		// ui->codec->setVisible(true);
		QList<QAbstractButton *> listButtons =
			ui->codecButtonGroup->buttons();
		for (QList<QAbstractButton *>::iterator iter =
			     listButtons.begin();
		     iter != listButtons.end(); ++iter) {
			QRadioButton *radiobutton =
				reinterpret_cast<QRadioButton *>(*iter);
			radiobutton->setVisible(false);
		}
		ui->codecGroupBox->setVisible(false);
		ui->streamProtocolLabel->setVisible(false);
		ui->streamProtocol->setVisible(false);
		ui->streamingAdvancedSettingsButton->setVisible(false);
		ui->simulcastEnable->setVisible(false);
		ui->multisourceLabel->setVisible(false);
		ui->multisourceEnable->setVisible(false);
		ui->sourceIdLabel->setVisible(false);
		ui->sourceId->setVisible(false);
		ui->publishApiUrlLabel->setVisible(false);
		ui->publishApiUrl->setVisible(false);
	} else if (isWebrtc) {
		ui->streamKeyLabel->setVisible(false);
		ui->streamKeyWidget->setVisible(false);
		ui->serverStackedWidget->setCurrentIndex(1);
		ui->serverLabel->setVisible(true);
		ui->serverStackedWidget->setVisible(true);
		obs_properties_t *props = obs_get_service_properties(
			webrtc_services[GetServiceIndex()].c_str());
		obs_property_t *server = obs_properties_get(props, "server");
		obs_property_t *room = obs_properties_get(props, "room");
		obs_property_t *username =
			obs_properties_get(props, "username");
		obs_property_t *password =
			obs_properties_get(props, "password");
		obs_property_t *codec = obs_properties_get(props, "codec");
		obs_property_t *streamingAdvancedSettings = obs_properties_get(
			props, "streaming_advanced_settings");
		obs_property_t *simulcast =
			obs_properties_get(props, "simulcast");
		obs_property_t *multisource =
			obs_properties_get(props, "multisource");
		obs_property_t *sourceId =
			obs_properties_get(props, "sourceId");
		obs_property_t *publishApiUrl =
			obs_properties_get(props, "publish_api_url");
		obs_property_t *protocol =
			obs_properties_get(props, "protocol");
		ui->serverLabel->setText(obs_property_description(server));
		ui->roomLabel->setText(obs_property_description(room));
		ui->authUsernameLabel->setText(
			obs_property_description(username));
		ui->authPwLabel->setText(obs_property_description(password));
		int min_idx = 1;
		if (obs_property_visible(server)) {
			ui->streamkeyPageLayout->insertRow(
				min_idx, ui->serverLabel,
				ui->serverStackedWidget);
			min_idx++;
		}
		if (obs_property_visible(room)) {
			ui->streamkeyPageLayout->insertRow(
				min_idx, ui->roomLabel, ui->room);
			min_idx++;
		}
		if (obs_property_visible(username)) {
			ui->streamkeyPageLayout->insertRow(
				min_idx, ui->authUsernameLabel,
				ui->authUsername);
			min_idx++;
		}
		if (obs_property_visible(password)) {
			ui->streamkeyPageLayout->insertRow(
				min_idx, ui->authPwLabel, ui->authPwWidget);
			min_idx++;
		}
		// NOTE LUDO: #172 codecs list of radio buttons
		// if (obs_property_visible(codec)) {
		// 	ui->streamkeyPageLayout->insertRow(
		// 		min_idx, ui->codecLabel, ui->codec);
		// 	min_idx++;
		// }
		if (obs_property_visible(protocol)) {
			ui->streamkeyPageLayout->insertRow(
				min_idx, ui->streamProtocolLabel,
				ui->streamProtocol);
			min_idx++;
		}
		ui->serverLabel->setVisible(obs_property_visible(server));
		ui->serverStackedWidget->setVisible(
			obs_property_visible(server));
		ui->roomLabel->setVisible(obs_property_visible(room));
		ui->room->setVisible(obs_property_visible(room));
		ui->authUsernameLabel->setVisible(
			obs_property_visible(username));
		ui->authUsername->setVisible(obs_property_visible(username));
		ui->authPwLabel->setVisible(obs_property_visible(password));
		ui->authPwWidget->setVisible(obs_property_visible(password));
		ui->codecLabel->setVisible(obs_property_visible(codec));
		// NOTE LUDO: #172 codecs list of radio buttons
		// ui->codec->setVisible(obs_property_visible(codec));
		QList<QAbstractButton *> listButtons =
			ui->codecButtonGroup->buttons();
		for (QList<QAbstractButton *>::iterator iter =
			     listButtons.begin();
		     iter != listButtons.end(); ++iter) {
			QRadioButton *radiobutton =
				reinterpret_cast<QRadioButton *>(*iter);
			radiobutton->setVisible(obs_property_visible(codec));
		}
		ui->codecGroupBox->setVisible(true);
		ui->streamProtocolLabel->setVisible(
			obs_property_visible(protocol));
		ui->streamProtocol->setVisible(obs_property_visible(protocol));
		ui->streamingAdvancedSettingsButton->setVisible(true);
		ui->simulcastEnable->setVisible(false);
		ui->multisourceLabel->setVisible(true);
		ui->multisourceEnable->setVisible(true);
		ui->sourceIdLabel->setVisible(true);
		ui->sourceId->setVisible(true);
		ui->publishApiUrlLabel->setVisible(false);
		ui->publishApiUrl->setVisible(false);
		obs_properties_destroy(props);
	} else if (!custom && !isWebrtc) {
		ui->authUsernameLabel->setText("Username");
		ui->authPwLabel->setText("Password");
		ui->streamkeyPageLayout->insertRow(1, ui->serverLabel,
						   ui->serverStackedWidget);
		ui->streamkeyPageLayout->insertRow(2, ui->streamKeyLabel,
						   ui->streamKeyWidget);
		ui->streamkeyPageLayout->insertRow(3, NULL, ui->useAuth);
		ui->streamkeyPageLayout->insertRow(4, ui->authUsernameLabel,
						   ui->authUsername);
		ui->streamkeyPageLayout->insertRow(5, ui->authPwLabel,
						   ui->authPwWidget);
		// NOTE LUDO: #172 codecs list of radio buttons
		// ui->streamkeyPageLayout->insertRow(6, ui->codecLabel,
		// 				   ui->codec);
		ui->streamkeyPageLayout->insertRow(7, ui->streamProtocolLabel,
						   ui->streamProtocol);
		ui->serverStackedWidget->setCurrentIndex(0);
		ui->serverLabel->setVisible(true);
		ui->serverStackedWidget->setVisible(true);
		ui->streamKeyLabel->setVisible(true);
		ui->streamKeyWidget->setVisible(true);
		ui->roomLabel->setVisible(false);
		ui->room->setVisible(false);
		ui->streamProtocolLabel->setVisible(false);
		ui->streamProtocol->setVisible(false);
		ui->codecLabel->setVisible(false);
		// NOTE LUDO: #172 codecs list of radio buttons
		// ui->codec->setVisible(false);
		QList<QAbstractButton *> listButtons =
			ui->codecButtonGroup->buttons();
		for (QList<QAbstractButton *>::iterator iter =
			     listButtons.begin();
		     iter != listButtons.end(); ++iter) {
			QRadioButton *radiobutton =
				reinterpret_cast<QRadioButton *>(*iter);
			radiobutton->setVisible(false);
		}
		ui->codecGroupBox->setVisible(false);
		ui->streamingAdvancedSettingsButton->setVisible(false);
		ui->simulcastEnable->setVisible(false);
		ui->multisourceLabel->setVisible(false);
		ui->multisourceEnable->setVisible(false);
		ui->sourceIdLabel->setVisible(false);
		ui->sourceId->setVisible(false);
		ui->publishApiUrlLabel->setVisible(false);
		ui->publishApiUrl->setVisible(false);
	}

	auth.reset();

	if (!main->auth) {
		return;
	}

	auto system_auth_service = main->auth->service();
	bool service_check = service.find(system_auth_service) !=
			     std::string::npos;
#if YOUTUBE_ENABLED
	service_check = service_check ? service_check
				      : IsYouTubeService(system_auth_service) &&
						IsYouTubeService(service);
#endif
	if (service_check) {
		auth = main->auth;
		OnAuthConnected();
	}
}

void OBSBasicSettings::UpdateServerList()
{
	// #289 service list of radio buttons
	QString serviceName = ui->serviceButtonGroup->checkedButton()->text();
	lastService = serviceName;

	obs_properties_t *props = obs_get_service_properties("rtmp_common");
	obs_property_t *services = obs_properties_get(props, "service");

	OBSDataAutoRelease settings = obs_data_create();

	obs_data_set_string(settings, "service", QT_TO_UTF8(serviceName));
	obs_property_modified(services, settings);

	obs_property_t *servers = obs_properties_get(props, "server");

	ui->server->clear();

	size_t servers_count = obs_property_list_item_count(servers);
	for (size_t i = 0; i < servers_count; i++) {
		const char *name = obs_property_list_item_name(servers, i);
		const char *server = obs_property_list_item_string(servers, i);
		ui->server->addItem(name, server);
	}

	obs_properties_destroy(props);
}

void OBSBasicSettings::on_show_clicked()
{
	if (ui->key->echoMode() == QLineEdit::Password) {
		ui->key->setEchoMode(QLineEdit::Normal);
		ui->show->setText(QTStr("Hide"));
	} else {
		ui->key->setEchoMode(QLineEdit::Password);
		ui->show->setText(QTStr("Show"));
	}
}

void OBSBasicSettings::on_authPwShow_clicked()
{
	if (ui->authPw->echoMode() == QLineEdit::Password) {
		ui->authPw->setEchoMode(QLineEdit::Normal);
		ui->authPwShow->setText(QTStr("Hide"));
	} else {
		ui->authPw->setEchoMode(QLineEdit::Password);
		ui->authPwShow->setText(QTStr("Show"));
	}
}

OBSService OBSBasicSettings::SpawnTempService()
{
	bool custom = IsCustomService();
	// #289 service list of radio buttons
	bool isWebrtc = IsWebRTC();

	const char *service_id =
		!isWebrtc ? custom ? "rtmp_custom" : "rtmp_common"
			  : webrtc_services[GetServiceIndex()].c_str();

	OBSDataAutoRelease settings = obs_data_create();

	if (!custom && !isWebrtc) {
		// #289 service list of radio buttons
		obs_data_set_string(
			settings, "service",
			QT_TO_UTF8(ui->serviceButtonGroup->checkedButton()
					   ->text()));
		obs_data_set_string(
			settings, "server",
			QT_TO_UTF8(ui->server->currentData().toString()));
	} else {
		obs_data_set_string(
			settings, "server",
			QT_TO_UTF8(ui->customServer->text().trimmed()));
	}
	obs_data_set_string(settings, "key", QT_TO_UTF8(ui->key->text()));

	OBSServiceAutoRelease newService = obs_service_create(
		service_id, "temp_service", settings, nullptr);
	return newService.Get();
}

void OBSBasicSettings::OnOAuthStreamKeyConnected()
{
	OAuthStreamKey *a = reinterpret_cast<OAuthStreamKey *>(auth.get());

	if (a) {
		bool validKey = !a->key().empty();

		if (validKey)
			ui->key->setText(QT_UTF8(a->key().c_str()));

		ui->streamKeyWidget->setVisible(false);
		ui->streamKeyLabel->setVisible(false);
		ui->connectAccount2->setVisible(false);
		ui->disconnectAccount->setVisible(true);
		ui->useStreamKeyAdv->setVisible(false);

		ui->connectedAccountLabel->setVisible(false);
		ui->connectedAccountText->setVisible(false);

		if (strcmp(a->service(), "Twitch") == 0) {
			ui->bandwidthTestEnable->setVisible(true);
			ui->twitchAddonLabel->setVisible(true);
			ui->twitchAddonDropdown->setVisible(true);
		} else {
			ui->bandwidthTestEnable->setChecked(false);
		}
#if YOUTUBE_ENABLED
		if (IsYouTubeService(a->service())) {
			ui->key->clear();

			ui->connectedAccountLabel->setVisible(true);
			ui->connectedAccountText->setVisible(true);

			ui->connectedAccountText->setText(
				QTStr("Auth.LoadingChannel.Title"));

			get_yt_ch_title(ui.get());
		}
#endif
	}

	ui->streamStackWidget->setCurrentIndex((int)Section::StreamKey);
}

void OBSBasicSettings::OnAuthConnected()
{
	// #289 service list of radio buttons
	std::string service =
		QT_TO_UTF8(ui->serviceButtonGroup->checkedButton()->text());
	Auth::Type type = Auth::AuthType(service);

	if (type == Auth::Type::OAuth_StreamKey ||
	    type == Auth::Type::OAuth_LinkedAccount) {
		OnOAuthStreamKeyConnected();
	}

	if (!loading) {
		stream1Changed = true;
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::on_connectAccount_clicked()
{
	// #289 service list of radio buttons
	std::string service =
		QT_TO_UTF8(ui->serviceButtonGroup->checkedButton()->text());

	OAuth::DeleteCookies(service);

	auth = OAuthStreamKey::Login(this, service);
	if (!!auth) {
		OnAuthConnected();

		ui->useStreamKeyAdv->setVisible(false);
	}
}

#define DISCONNECT_COMFIRM_TITLE \
	"Basic.AutoConfig.StreamPage.DisconnectAccount.Confirm.Title"
#define DISCONNECT_COMFIRM_TEXT \
	"Basic.AutoConfig.StreamPage.DisconnectAccount.Confirm.Text"

void OBSBasicSettings::on_disconnectAccount_clicked()
{
	QMessageBox::StandardButton button;

	button = OBSMessageBox::question(this, QTStr(DISCONNECT_COMFIRM_TITLE),
					 QTStr(DISCONNECT_COMFIRM_TEXT));

	if (button == QMessageBox::No) {
		return;
	}

	main->auth.reset();
	auth.reset();
	main->SetBroadcastFlowEnabled(false);

	// #289 service list of radio buttons
	std::string service =
		QT_TO_UTF8(ui->serviceButtonGroup->checkedButton()->text());

#ifdef BROWSER_AVAILABLE
	OAuth::DeleteCookies(service);
#endif

	bool isWebrtc = IsWebRTC();

	if (isWebrtc) {
		ui->streamKeyWidget->setVisible(false);
		ui->streamKeyLabel->setVisible(false);
	} else {
		ui->streamKeyWidget->setVisible(true);
		ui->streamKeyLabel->setVisible(true);
	}

	ui->bandwidthTestEnable->setChecked(false);

	ui->streamKeyWidget->setVisible(true);
	ui->streamKeyLabel->setVisible(true);
	ui->connectAccount2->setVisible(true);
	ui->disconnectAccount->setVisible(false);
	ui->bandwidthTestEnable->setVisible(false);
	ui->twitchAddonDropdown->setVisible(false);
	ui->twitchAddonLabel->setVisible(false);
	ui->key->setText("");

	ui->connectedAccountLabel->setVisible(false);
	ui->connectedAccountText->setVisible(false);
}

void OBSBasicSettings::on_useStreamKey_clicked()
{
	ui->streamStackWidget->setCurrentIndex((int)Section::StreamKey);
}

void OBSBasicSettings::on_useAuth_toggled()
{
	if (!IsCustomService())
		return;

	bool use_auth = ui->useAuth->isChecked();

	ui->authUsernameLabel->setVisible(use_auth);
	ui->authUsername->setVisible(use_auth);
	ui->authPwLabel->setVisible(use_auth);
	ui->authPwWidget->setVisible(use_auth);
}

void OBSBasicSettings::UpdateVodTrackSetting()
{
	bool enableForCustomServer = config_get_bool(
		GetGlobalConfig(), "General", "EnableCustomServerVodTrack");
	// #289 service list of radio buttons
	bool enableVodTrack = ui->serviceButtonGroup->checkedButton()->text() ==
			      "Twitch";
	bool wasEnabled = !!vodTrackCheckbox;

	if (enableForCustomServer && IsCustomService())
		enableVodTrack = true;

	if (enableVodTrack == wasEnabled)
		return;

	if (!enableVodTrack) {
		delete vodTrackCheckbox;
		delete vodTrackContainer;
		delete simpleVodTrack;
		return;
	}

	/* -------------------------------------- */
	/* simple output mode vod track widgets   */

	bool simpleAdv = ui->simpleOutAdvanced->isChecked();
	bool vodTrackEnabled = config_get_bool(main->Config(), "SimpleOutput",
					       "VodTrackEnabled");

	simpleVodTrack = new QCheckBox(this);
	simpleVodTrack->setText(
		QTStr("Basic.Settings.Output.Simple.TwitchVodTrack"));
	simpleVodTrack->setVisible(simpleAdv);
	simpleVodTrack->setChecked(vodTrackEnabled);

	int pos;
	ui->simpleStreamingLayout->getWidgetPosition(ui->simpleOutAdvanced,
						     &pos, nullptr);
	ui->simpleStreamingLayout->insertRow(pos + 1, nullptr, simpleVodTrack);

	HookWidget(simpleVodTrack, SIGNAL(clicked(bool)),
		   SLOT(OutputsChanged()));
	connect(ui->simpleOutAdvanced, SIGNAL(toggled(bool)),
		simpleVodTrack.data(), SLOT(setVisible(bool)));

	/* -------------------------------------- */
	/* advanced output mode vod track widgets */

	vodTrackCheckbox = new QCheckBox(this);
	vodTrackCheckbox->setText(
		QTStr("Basic.Settings.Output.Adv.TwitchVodTrack"));
	vodTrackCheckbox->setLayoutDirection(Qt::RightToLeft);

	vodTrackContainer = new QWidget(this);
	QHBoxLayout *vodTrackLayout = new QHBoxLayout();
	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		vodTrack[i] = new QRadioButton(QString::number(i + 1));
		vodTrackLayout->addWidget(vodTrack[i]);

		HookWidget(vodTrack[i], SIGNAL(clicked(bool)),
			   SLOT(OutputsChanged()));
	}

	HookWidget(vodTrackCheckbox, SIGNAL(clicked(bool)),
		   SLOT(OutputsChanged()));

	vodTrackLayout->addStretch();
	vodTrackLayout->setContentsMargins(0, 0, 0, 0);

	vodTrackContainer->setLayout(vodTrackLayout);

	ui->advOutTopLayout->insertRow(2, vodTrackCheckbox, vodTrackContainer);

	vodTrackEnabled =
		config_get_bool(main->Config(), "AdvOut", "VodTrackEnabled");
	vodTrackCheckbox->setChecked(vodTrackEnabled);
	vodTrackContainer->setEnabled(vodTrackEnabled);

	connect(vodTrackCheckbox, SIGNAL(clicked(bool)), vodTrackContainer,
		SLOT(setEnabled(bool)));

	int trackIndex =
		config_get_int(main->Config(), "AdvOut", "VodTrackIndex");
	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		vodTrack[i]->setChecked((i + 1) == trackIndex);
	}
}

OBSService OBSBasicSettings::GetStream1Service()
{
	return stream1Changed ? SpawnTempService()
			      : OBSService(main->GetService());
}

void OBSBasicSettings::UpdateServiceRecommendations()
{
	bool customServer = IsCustomService();
	// ui->ignoreRecommended->setVisible(!customServer);
	// ui->enforceSettingsLabel->setVisible(!customServer);

	OBSService service = GetStream1Service();

	int vbitrate, abitrate;
	BPtr<obs_service_resolution> res_list;
	size_t res_count;
	int fps;

	obs_service_get_max_bitrate(service, &vbitrate, &abitrate);
	obs_service_get_supported_resolutions(service, &res_list, &res_count);
	obs_service_get_max_fps(service, &fps);

	QString text;

#define ENFORCE_TEXT(x) QTStr("Basic.Settings.Stream.Recommended." x)
	if (vbitrate)
		text += ENFORCE_TEXT("MaxVideoBitrate")
				.arg(QString::number(vbitrate));
	if (abitrate) {
		if (!text.isEmpty())
			text += "<br>";
		text += ENFORCE_TEXT("MaxAudioBitrate")
				.arg(QString::number(abitrate));
	}
	if (res_count) {
		if (!text.isEmpty())
			text += "<br>";

		obs_service_resolution best_res = {};
		int best_res_pixels = 0;

		for (size_t i = 0; i < res_count; i++) {
			obs_service_resolution res = res_list[i];
			int res_pixels = res.cx + res.cy;
			if (res_pixels > best_res_pixels) {
				best_res = res;
				best_res_pixels = res_pixels;
			}
		}

		QString res_str =
			QString("%1x%2").arg(QString::number(best_res.cx),
					     QString::number(best_res.cy));
		text += ENFORCE_TEXT("MaxResolution").arg(res_str);
	}
	if (fps) {
		if (!text.isEmpty())
			text += "<br>";

		text += ENFORCE_TEXT("MaxFPS").arg(QString::number(fps));
	}
#undef ENFORCE_TEXT

#if YOUTUBE_ENABLED
	if (IsYouTubeService(QT_TO_UTF8(ui->service->currentText()))) {
		if (!text.isEmpty())
			text += "<br><br>";

		text += "<a href=\"https://www.youtube.com/t/terms\">"
			"YouTube Terms of Service</a><br>"
			"<a href=\"http://www.google.com/policies/privacy\">"
			"Google Privacy Policy</a><br>"
			"<a href=\"https://security.google.com/settings/security/permissions\">"
			"Google Third-Party Permissions</a>";
	}
#endif
	// ui->enforceSettingsLabel->setText(text);
}

void OBSBasicSettings::DisplayEnforceWarning(bool checked)
{
	if (IsCustomService())
		return;

	if (!checked) {
		SimpleRecordingEncoderChanged();
		return;
	}

	QMessageBox::StandardButton button;

#define ENFORCE_WARNING(x) \
	QTStr("Basic.Settings.Stream.IgnoreRecommended.Warn." x)

	button = OBSMessageBox::question(this, ENFORCE_WARNING("Title"),
					 ENFORCE_WARNING("Text"));
#undef ENFORCE_WARNING

	if (button == QMessageBox::No) {
		// QMetaObject::invokeMethod(ui->ignoreRecommended, "setChecked",
		// 			  Qt::QueuedConnection,
		// 			  Q_ARG(bool, false));
		return;
	}

	SimpleRecordingEncoderChanged();
}

bool OBSBasicSettings::ResFPSValid(obs_service_resolution *res_list,
				   size_t res_count, int max_fps)
{
	if (!res_count && !max_fps)
		return true;

	if (res_count) {
		QString res = ui->outputResolution->currentText();
		bool found_res = false;

		int cx, cy;
		if (sscanf(QT_TO_UTF8(res), "%dx%d", &cx, &cy) != 2)
			return false;

		for (size_t i = 0; i < res_count; i++) {
			if (res_list[i].cx == cx && res_list[i].cy == cy) {
				found_res = true;
				break;
			}
		}

		if (!found_res)
			return false;
	}

	if (max_fps) {
		int fpsType = ui->fpsType->currentIndex();
		if (fpsType != 0)
			return false;

		std::string fps_str = QT_TO_UTF8(ui->fpsCommon->currentText());
		float fps;
		sscanf(fps_str.c_str(), "%f", &fps);
		if (fps > (float)max_fps)
			return false;
	}

	return true;
}

extern void set_closest_res(int &cx, int &cy,
			    struct obs_service_resolution *res_list,
			    size_t count);

/* Checks for and updates the resolution and FPS limits of a service, if any.
 *
 * If the service has a resolution and/or FPS limit, this will enforce those
 * limitations in the UI itself, preventing the user from selecting a
 * resolution or FPS that's not supported.
 *
 * This is an unpleasant thing to have to do to users, but there is no other
 * way to ensure that a service's restricted resolution/framerate values are
 * properly enforced, otherwise users will just be confused when things aren't
 * working correctly. The user can turn it off if they're partner (or if they
 * want to risk getting in trouble with their service) by selecting the "Ignore
 * recommended settings" option in the stream section of settings.
 *
 * This only affects services that have a resolution and/or framerate limit, of
 * which as of this writing, and hopefully for the foreseeable future, there is
 * only one.
 */
void OBSBasicSettings::UpdateResFPSLimits()
{
	if (loading)
		return;

	// #289 service list of radio buttons
	// int idx = ui->service->currentIndex();
	int idx = GetServiceIndex();
	if (idx == -1)
		return;

	// #289 service list of radio buttons
	// bool ignoreRecommended = ui->ignoreRecommended->isChecked();
	BPtr<obs_service_resolution> res_list;
	size_t res_count = 0;
	int max_fps = 0;

	// #289 service list of radio buttons
	// if (!IsCustomService() && !ignoreRecommended) {
	if (!IsCustomService()) {
		OBSService service = GetStream1Service();
		obs_service_get_supported_resolutions(service, &res_list,
						      &res_count);
		obs_service_get_max_fps(service, &max_fps);
	}

	/* ------------------------------------ */
	/* Check for enforced res/FPS           */

	QString res = ui->outputResolution->currentText();
	QString fps_str;
	int cx = 0, cy = 0;
	double max_fpsd = (double)max_fps;
	int closest_fps_index = -1;
	double fpsd;

	sscanf(QT_TO_UTF8(res), "%dx%d", &cx, &cy);

	if (res_count)
		set_closest_res(cx, cy, res_list, res_count);

	if (max_fps) {
		int fpsType = ui->fpsType->currentIndex();

		if (fpsType == 1) { //Integer
			fpsd = (double)ui->fpsInteger->value();
		} else if (fpsType == 2) { //Fractional
			fpsd = (double)ui->fpsNumerator->value() /
			       (double)ui->fpsDenominator->value();
		} else { //Common
			sscanf(QT_TO_UTF8(ui->fpsCommon->currentText()), "%lf",
			       &fpsd);
		}

		double closest_diff = 1000000000000.0;

		for (int i = 0; i < ui->fpsCommon->count(); i++) {
			double com_fpsd;
			sscanf(QT_TO_UTF8(ui->fpsCommon->itemText(i)), "%lf",
			       &com_fpsd);

			if (com_fpsd > max_fpsd) {
				continue;
			}

			double diff = fabs(com_fpsd - fpsd);
			if (diff < closest_diff) {
				closest_diff = diff;
				closest_fps_index = i;
				fps_str = ui->fpsCommon->itemText(i);
			}
		}
	}

	QString res_str =
		QString("%1x%2").arg(QString::number(cx), QString::number(cy));

	/* ------------------------------------ */
	/* Display message box if res/FPS bad   */

	bool valid = ResFPSValid(res_list, res_count, max_fps);

	if (!valid) {
		/* if the user was already on facebook with an incompatible
		 * resolution, assume it's an upgrade */
		// #289 service list of radio buttons
		// if (lastServiceIdx == -1 && lastIgnoreRecommended == -1) {
		if (lastServiceIdx == -1) {
			// ui->ignoreRecommended->setChecked(true);
			// ui->ignoreRecommended->setProperty("changed", true);
			stream1Changed = true;
			EnableApplyButton(true);
			UpdateResFPSLimits();
			return;
		}

		QMessageBox::StandardButton button;

#define WARNING_VAL(x) \
	QTStr("Basic.Settings.Output.Warn.EnforceResolutionFPS." x)

		QString str;
		if (res_count)
			str += WARNING_VAL("Resolution").arg(res_str);
		if (max_fps) {
			if (!str.isEmpty())
				str += "\n";
			str += WARNING_VAL("FPS").arg(fps_str);
		}

		button = OBSMessageBox::question(this, WARNING_VAL("Title"),
						 WARNING_VAL("Msg").arg(str));
#undef WARNING_VAL

		if (button == QMessageBox::No) {
			// #289 service list of radio buttons
			// if (idx != lastServiceIdx)
			// 	QMetaObject::invokeMethod(
			// 		ui->service, "setCurrentIndex",
			// 		Qt::QueuedConnection,
			// 		Q_ARG(int, lastServiceIdx));
			// else
			// 	QMetaObject::invokeMethod(ui->ignoreRecommended,
			// 				  "setChecked",
			// 				  Qt::QueuedConnection,
			// 				  Q_ARG(bool, true));
			return;
		}
	}

	/* ------------------------------------ */
	/* Update widgets/values if switching   */
	/* to/from enforced resolution/FPS      */

	ui->outputResolution->blockSignals(true);
	if (res_count) {
		ui->outputResolution->clear();
		ui->outputResolution->setEditable(false);
		HookWidget(ui->outputResolution,
			   SIGNAL(currentIndexChanged(int)),
			   SLOT(VideoChangedResolution()));

		int new_res_index = -1;

		for (size_t i = 0; i < res_count; i++) {
			obs_service_resolution val = res_list[i];
			QString str =
				QString("%1x%2").arg(QString::number(val.cx),
						     QString::number(val.cy));
			ui->outputResolution->addItem(str);

			if (val.cx == cx && val.cy == cy)
				new_res_index = (int)i;
		}

		ui->outputResolution->setCurrentIndex(new_res_index);
		if (!valid) {
			ui->outputResolution->setProperty("changed", true);
			videoChanged = true;
			EnableApplyButton(true);
		}
	} else {
		QString baseRes = ui->baseResolution->currentText();
		int baseCX, baseCY;
		sscanf(QT_TO_UTF8(baseRes), "%dx%d", &baseCX, &baseCY);

		if (!ui->outputResolution->isEditable()) {
			RecreateOutputResolutionWidget();
			ui->outputResolution->blockSignals(true);
			ResetDownscales((uint32_t)baseCX, (uint32_t)baseCY,
					true);
			ui->outputResolution->setCurrentText(res);
		}
	}
	ui->outputResolution->blockSignals(false);

	if (max_fps) {
		for (int i = 0; i < ui->fpsCommon->count(); i++) {
			double com_fpsd;
			sscanf(QT_TO_UTF8(ui->fpsCommon->itemText(i)), "%lf",
			       &com_fpsd);

			if (com_fpsd > max_fpsd) {
				SetComboItemEnabled(ui->fpsCommon, i, false);
				continue;
			}
		}

		ui->fpsType->setCurrentIndex(0);
		ui->fpsCommon->setCurrentIndex(closest_fps_index);
		if (!valid) {
			ui->fpsType->setProperty("changed", true);
			ui->fpsCommon->setProperty("changed", true);
			videoChanged = true;
			EnableApplyButton(true);
		}
	} else {
		for (int i = 0; i < ui->fpsCommon->count(); i++)
			SetComboItemEnabled(ui->fpsCommon, i, true);
	}

	SetComboItemEnabled(ui->fpsType, 1, !max_fps);
	SetComboItemEnabled(ui->fpsType, 2, !max_fps);

	/* ------------------------------------ */

	// #289 service list of radio buttons
	// lastIgnoreRecommended = (int)ignoreRecommended;
	lastServiceIdx = idx;
}

bool OBSBasicSettings::IsServiceOutputHasNetworkFeatures()
{
	if (IsCustomService())
		return ui->customServer->text().startsWith("rtmp");

	OBSServiceAutoRelease service = SpawnTempService();
	const char *output = obs_service_get_output_type(service);

	if (!output)
		return true;

	if (strcmp(output, "rtmp_output") == 0)
		return true;

	return false;
}

static bool service_supports_codec(const char **codecs, const char *codec)
{
	if (!codecs)
		return true;

	while (*codecs) {
		if (strcmp(*codecs, codec) == 0)
			return true;
		codecs++;
	}

	return false;
}

extern bool EncoderAvailable(const char *encoder);
extern const char *get_simple_output_encoder(const char *name);

static inline bool service_supports_encoder(const char **codecs,
					    const char *encoder)
{
	if (!EncoderAvailable(encoder))
		return false;

	const char *codec = obs_get_encoder_codec(encoder);
	return service_supports_codec(codecs, codec);
}

bool OBSBasicSettings::ServiceAndCodecCompatible()
{
	if (IsCustomService())
		return true;
	// #289 service list of radio buttons
	// if (ui->service->currentData().toInt() == (int)ListOpt::ShowAll)
	// 	return true;

	bool simple = (ui->outputMode->currentIndex() == 0);

	OBSService service = SpawnTempService();
	const char **codecs = obs_service_get_supported_video_codecs(service);
	const char *codec;

	if (simple) {
		QString encoder =
			ui->simpleOutStrEncoder->currentData().toString();
		const char *id = get_simple_output_encoder(QT_TO_UTF8(encoder));
		codec = obs_get_encoder_codec(id);
	} else {
		QString encoder = ui->advOutEncoder->currentData().toString();
		codec = obs_get_encoder_codec(QT_TO_UTF8(encoder));
	}

	return service_supports_codec(codecs, codec);
}

/* we really need a way to find fallbacks in a less hardcoded way. maybe. */
static QString get_adv_fallback(const QString &enc)
{
	if (enc == "jim_hevc_nvenc")
		return "jim_nvenc";
	if (enc == "h265_texture_amf")
		return "h264_texture_amf";
	return "obs_x264";
}

static QString get_simple_fallback(const QString &enc)
{
	if (enc == SIMPLE_ENCODER_NVENC_HEVC)
		return SIMPLE_ENCODER_NVENC;
	if (enc == SIMPLE_ENCODER_AMD_HEVC)
		return SIMPLE_ENCODER_AMD;
	return SIMPLE_ENCODER_X264;
}

bool OBSBasicSettings::ServiceSupportsCodecCheck()
{
	if (ServiceAndCodecCompatible()) {
		// #289 service list of radio buttons
		if (lastServiceIdx != GetServiceIndex())
			ResetEncoders(true);
		return true;
	}

	// #289 service list of radio buttons
	// QString service = ui->service->currentText();
	QString service = ui->millicastWebrtcRadioButton->isChecked()
																? ui->millicastWebrtcRadioButton->text()
																: ui->millicastRtmpRadioButton->text();
	QString cur_name;
	QString fb_name;
	bool simple = (ui->outputMode->currentIndex() == 0);

	/* ------------------------------------------------- */
	/* get current codec                                 */

	if (simple) {
		QString cur_enc =
			ui->simpleOutStrEncoder->currentData().toString();
		QString fb_enc = get_simple_fallback(cur_enc);

		int cur_idx = ui->simpleOutStrEncoder->findData(cur_enc);
		int fb_idx = ui->simpleOutStrEncoder->findData(fb_enc);

		cur_name = ui->simpleOutStrEncoder->itemText(cur_idx);
		fb_name = ui->simpleOutStrEncoder->itemText(fb_idx);
	} else {
		QString cur_enc = ui->advOutEncoder->currentData().toString();
		QString fb_enc = get_adv_fallback(cur_enc);

		cur_name = obs_encoder_get_display_name(QT_TO_UTF8(cur_enc));
		fb_name = obs_encoder_get_display_name(QT_TO_UTF8(fb_enc));
	}

#define WARNING_VAL(x) \
	QTStr("Basic.Settings.Output.Warn.ServiceCodecCompatibility." x)

	QString msg = WARNING_VAL("Msg").arg(service, cur_name, fb_name);
	auto button = OBSMessageBox::question(this, WARNING_VAL("Title"), msg);
#undef WARNING_VAL

	// #289 service list of radio buttons
	if (button == QMessageBox::No) {
		// QMetaObject::invokeMethod(ui->service, "setCurrentIndex",
		// 			  Qt::QueuedConnection,
		// 			  Q_ARG(int, lastServiceIdx));
		return false;
	}

	ResetEncoders(true);
	return true;
}

#define TEXT_USE_STREAM_ENC \
	QTStr("Basic.Settings.Output.Adv.Recording.UseStreamEncoder")

void OBSBasicSettings::ResetEncoders(bool streamOnly)
{
	QString lastAdvEnc = ui->advOutRecEncoder->currentData().toString();
	QString lastEnc = ui->simpleOutStrEncoder->currentData().toString();
	OBSService service = SpawnTempService();
	const char **codecs = obs_service_get_supported_video_codecs(service);
	const char *type;
	size_t idx = 0;

	QSignalBlocker s1(ui->simpleOutStrEncoder);
	QSignalBlocker s2(ui->advOutEncoder);

	/* ------------------------------------------------- */
	/* clear encoder lists                               */

	ui->simpleOutStrEncoder->clear();
	ui->advOutEncoder->clear();

	if (!streamOnly) {
		ui->advOutRecEncoder->clear();
		ui->advOutRecEncoder->addItem(TEXT_USE_STREAM_ENC, "none");
	}

	/* ------------------------------------------------- */
	/* load advanced stream/recording encoders           */

	while (obs_enum_encoder_types(idx++, &type)) {
		const char *name = obs_encoder_get_display_name(type);
		const char *codec = obs_get_encoder_codec(type);
		uint32_t caps = obs_get_encoder_caps(type);

		if (obs_get_encoder_type(type) != OBS_ENCODER_VIDEO)
			continue;

		const char *streaming_codecs[] = {
			"h264",
#ifdef ENABLE_HEVC
			"hevc",
#endif
		};

		bool is_streaming_codec = false;
		for (const char *test_codec : streaming_codecs) {
			if (strcmp(codec, test_codec) == 0) {
				is_streaming_codec = true;
				break;
			}
		}
		if ((caps & ENCODER_HIDE_FLAGS) != 0)
			continue;

		QString qName = QT_UTF8(name);
		QString qType = QT_UTF8(type);

		if (is_streaming_codec && service_supports_codec(codecs, codec))
			ui->advOutEncoder->addItem(qName, qType);
		if (!streamOnly)
			ui->advOutRecEncoder->addItem(qName, qType);
	}

	/* ------------------------------------------------- */
	/* load simple stream encoders                       */

#define ENCODER_STR(str) QTStr("Basic.Settings.Output.Simple.Encoder." str)

	ui->simpleOutStrEncoder->addItem(ENCODER_STR("Software"),
					 QString(SIMPLE_ENCODER_X264));
	if (service_supports_encoder(codecs, "obs_qsv11"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.QSV.H264"),
			QString(SIMPLE_ENCODER_QSV));
	if (service_supports_encoder(codecs, "ffmpeg_nvenc"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.NVENC.H264"),
			QString(SIMPLE_ENCODER_NVENC));
#ifdef ENABLE_HEVC
	if (service_supports_encoder(codecs, "h265_texture_amf"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.AMD.HEVC"),
			QString(SIMPLE_ENCODER_AMD_HEVC));
	if (service_supports_encoder(codecs, "ffmpeg_hevc_nvenc"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.NVENC.HEVC"),
			QString(SIMPLE_ENCODER_NVENC_HEVC));
#endif
	if (service_supports_encoder(codecs, "h264_texture_amf"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.AMD.H264"),
			QString(SIMPLE_ENCODER_AMD));
/* Preprocessor guard required for the macOS version check */
#ifdef __APPLE__
	if (service_supports_encoder(
		    codecs, "com.apple.videotoolbox.videoencoder.ave.avc")
#ifndef __aarch64__
	    && os_get_emulation_status() == true
#endif
	) {
		if (__builtin_available(macOS 13.0, *)) {
			ui->simpleOutStrEncoder->addItem(
				ENCODER_STR("Hardware.Apple.H264"),
				QString(SIMPLE_ENCODER_APPLE_H264));
		}
	}
#endif
#undef ENCODER_STR

	/* ------------------------------------------------- */
	/* Find fallback encoders                            */

	if (!lastAdvEnc.isEmpty()) {
		int idx = ui->advOutEncoder->findData(lastAdvEnc);
		if (idx == -1) {
			lastAdvEnc = get_adv_fallback(lastAdvEnc);
			ui->advOutEncoder->setProperty("changed",
						       QVariant(true));
			OutputsChanged();
		}

		idx = ui->advOutEncoder->findData(lastAdvEnc);
		ui->advOutEncoder->setCurrentIndex(idx);
	}

	if (!lastEnc.isEmpty()) {
		int idx = ui->simpleOutStrEncoder->findData(lastEnc);
		if (idx == -1) {
			lastEnc = get_simple_fallback(lastEnc);
			ui->simpleOutStrEncoder->setProperty("changed",
							     QVariant(true));
			OutputsChanged();
		}

		idx = ui->simpleOutStrEncoder->findData(lastEnc);
		ui->simpleOutStrEncoder->setCurrentIndex(idx);
	}
}
