// Copyright Dr. Alex. Gouaillard (2015, 2020)
#include <QMessageBox>
#include <QUrl>

#include "window-basic-settings.hpp"
#include "obs-frontend-api.h"
#include "obs-app.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"
#include "url-push-button.hpp"

#ifdef BROWSER_AVAILABLE
#include <browser-panel.hpp>
#include "auth-oauth.hpp"
#endif

struct QCef;
struct QCefCookieManager;

extern QCef *cef;
extern QCefCookieManager *panel_cookies;

// #289 service list of radio buttons
// 0 = Remote Filming (Millicast)
// 1 = Remote Filming RTMP   (Custom)
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
	connect(ui->ignoreRecommended, SIGNAL(clicked(bool)), this,
		SLOT(DisplayEnforceWarning(bool)));
	// #289 service list of radio buttons
	// connect(ui->ignoreRecommended, SIGNAL(toggled(bool)), this,
	// 	SLOT(UpdateResFPSLimits()));
	connect(ui->customServer, SIGNAL(editingFinished(const QString &)),
		this, SLOT(UpdateKeyLink()));
	// #289 service list of radio buttons
	connect(ui->serviceButtonGroup, SIGNAL(clicked(bool)), this,
		SLOT(UpdateMoreInfoLink()));
}

void OBSBasicSettings::LoadStream1Settings()
{
	bool ignoreRecommended =
		config_get_bool(main->Config(), "Stream1", "IgnoreRecommended");

	obs_service_t *service_obj = main->GetService();
	const char *type = obs_service_get_type(service_obj);

	loading = true;

	obs_data_t *settings = obs_service_get_settings(service_obj);

	// #289 service list of radio buttons
	const char *tmpString = nullptr;
	tmpString = obs_data_get_string(settings, "service");
	const char *service = strcmp("", tmpString) == 0
				      ? "Remote Filming"
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
		ui->customServer->setText("rtmp://live-rtmp-pub.millicast.com:1935/v2/pub/");
		ui->customServer->setVisible(false);
		ui->serverLabel->setVisible(false);

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
		ui->customServer->setVisible(true);
		ui->serverLabel->setVisible(true);
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

	obs_data_release(settings);

	UpdateKeyLink();
	UpdateMoreInfoLink();
	UpdateVodTrackSetting();
	UpdateServiceRecommendations();

	bool streamActive = obs_frontend_streaming_active();
	ui->streamPage->setEnabled(!streamActive);

	ui->ignoreRecommended->setChecked(ignoreRecommended);

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
	OBSData hotkeyData = obs_hotkeys_save_service(oldService);
	obs_data_release(hotkeyData);

	OBSData settings = obs_data_create();
	obs_data_release(settings);

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

	OBSService newService = obs_service_create(
		service_id, "default_service", settings, hotkeyData);
	obs_service_release(newService);

	if (!newService)
		return;

	main->SetService(newService);
	main->SaveService();
	main->auth = auth;
	if (!!main->auth)
		main->auth->LoadUI();

	SaveCheckBox(ui->ignoreRecommended, "Stream1", "IgnoreRecommended");
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

	OBSData settings = obs_data_create();
	obs_data_release(settings);

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
	if (serviceName == "Twitch") {
		streamKeyLink = "https://dashboard.twitch.tv/settings/stream";
	} else if (serviceName.startsWith("YouTube")) {
		streamKeyLink = "https://www.youtube.com/live_dashboard";
	} else if (serviceName.startsWith("Restream.io")) {
		streamKeyLink =
			"https://restream.io/settings/streaming-setup?from=OBS";
	} else if (serviceName == "Luzento.com - RTMP") {
		streamKeyLink =
			"https://cms.luzento.com/dashboard/stream-key?from=OBS";
	} else if (serviceName == "Facebook Live" ||
		   (customServer.contains("fbcdn.net") && IsCustomService())) {
		streamKeyLink =
			"https://www.facebook.com/live/producer?ref=OBS";
	} else if (serviceName.startsWith("Twitter")) {
		streamKeyLink = "https://studio.twitter.com/producer/sources";
	} else if (serviceName.startsWith("YouStreamer")) {
		streamKeyLink = "https://app.youstreamer.com/stream/";
	} else if (serviceName == "Trovo") {
		streamKeyLink = "https://studio.trovo.live/mychannel/stream";
	} else if (serviceName == "Glimesh") {
		streamKeyLink = "https://glimesh.tv/users/settings/stream";
	} else if (serviceName.startsWith("OPENREC.tv")) {
		streamKeyLink =
			"https://www.openrec.tv/login?keep_login=true&url=https://www.openrec.tv/dashboard/live?from=obs";
	} else if (serviceName == "Brime Live") {
		streamKeyLink = "https://brimelive.com/obs-stream-key-link";
	}

	if (serviceName == "Dacast") {
		ui->streamKeyLabel->setText(
			QTStr("Basic.AutoConfig.StreamPage.EncoderKey"));
	} else {
		ui->streamKeyLabel->setText(
			QTStr("Basic.AutoConfig.StreamPage.StreamKey"));
	}

	if (QString(streamKeyLink).isNull()) {
		ui->getStreamKeyButton->hide();
	} else {
		ui->getStreamKeyButton->setTargetUrl(QUrl(streamKeyLink));
		ui->getStreamKeyButton->show();
	}
}

// #289 service list of radio buttons
// void OBSBasicSettings::LoadServices(bool showAll)
// {
// 	obs_properties_t *props = obs_get_service_properties("rtmp_common");

// 	OBSData settings = obs_data_create();
// 	obs_data_release(settings);

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

#ifdef BROWSER_AVAILABLE
	if (cef && !isWebrtc) {
		if (lastService != service.c_str()) {
			QString key = ui->key->text();
			bool can_auth = is_auth_service(service);
			int page = can_auth && (!loading || key.isEmpty())
					   ? (int)Section::Connect
					   : (int)Section::StreamKey;

			ui->streamStackWidget->setCurrentIndex(page);
			ui->streamKeyWidget->setVisible(true);
			ui->streamKeyLabel->setVisible(true);
			ui->connectAccount2->setVisible(can_auth);
		}
	} else {
		ui->connectAccount2->setVisible(false);
	}
#else
	ui->connectAccount2->setVisible(false);
#endif

	// #289 service list of radio buttons
	ui->useAuth->setVisible(false);
	ui->authUsernameLabel->setVisible(isWebrtc);
	ui->authUsername->setVisible(isWebrtc);
	ui->authPwLabel->setVisible(false);
	ui->authPwWidget->setVisible(false);

	if (custom && !isWebrtc) {
		// ui->streamkeyPageLayout->insertRow(1, ui->serverLabel,
		// 				   ui->serverStackedWidget);
		ui->streamkeyPageLayout->insertRow(2, ui->streamKeyLabel,
						   ui->streamKeyWidget);
		// ui->streamkeyPageLayout->insertRow(3, nullptr, ui->useAuth);
		// ui->streamkeyPageLayout->insertRow(4, ui->authUsernameLabel,
		// 				   ui->authUsername);
		// ui->streamkeyPageLayout->insertRow(5, ui->authPwLabel,
		// 				   ui->authPwWidget);
		// NOTE LUDO: #172 codecs list of radio buttons
		// ui->streamkeyPageLayout->insertRow(6, ui->codecLabel,
		//				   ui->codec);
		// ui->streamkeyPageLayout->insertRow(7, ui->streamProtocolLabel,
		// 				   ui->streamProtocol);

		ui->serverLabel->setVisible(false);
		ui->serverLabel->setText("Server");
		ui->serverStackedWidget->setCurrentIndex(1);
		ui->serverStackedWidget->setVisible(false);
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
		ui->publishApiUrlLabel->setVisible(false);
		ui->publishApiUrl->setVisible(false);
	}

#ifdef BROWSER_AVAILABLE
	auth.reset();

	if (!!main->auth &&
	    service.find(main->auth->service()) != std::string::npos) {
		auth = main->auth;
		OnAuthConnected();
	}
#endif
}

void OBSBasicSettings::UpdateServerList()
{
	// #289 service list of radio buttons
	QString serviceName = ui->serviceButtonGroup->checkedButton()->text();
	lastService = serviceName;

	obs_properties_t *props = obs_get_service_properties("rtmp_common");
	obs_property_t *services = obs_properties_get(props, "service");

	OBSData settings = obs_data_create();
	obs_data_release(settings);

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

	OBSData settings = obs_data_create();
	obs_data_release(settings);

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
		obs_data_set_string(settings, "server",
				    QT_TO_UTF8(ui->customServer->text()));
	}
	obs_data_set_string(settings, "key", QT_TO_UTF8(ui->key->text()));

	OBSService newService = obs_service_create(service_id, "temp_service",
						   settings, nullptr);
	obs_service_release(newService);

	return newService;
}

void OBSBasicSettings::OnOAuthStreamKeyConnected()
{
#ifdef BROWSER_AVAILABLE
	OAuthStreamKey *a = reinterpret_cast<OAuthStreamKey *>(auth.get());

	if (a) {
		bool validKey = !a->key().empty();

		if (validKey)
			ui->key->setText(QT_UTF8(a->key().c_str()));

		ui->streamKeyWidget->setVisible(false);
		ui->streamKeyLabel->setVisible(false);
		ui->connectAccount2->setVisible(false);
		ui->disconnectAccount->setVisible(true);

		if (strcmp(a->service(), "Twitch") == 0) {
			ui->bandwidthTestEnable->setVisible(true);
			ui->twitchAddonLabel->setVisible(true);
			ui->twitchAddonDropdown->setVisible(true);
		} else {
			ui->bandwidthTestEnable->setChecked(false);
		}
	}

	ui->streamStackWidget->setCurrentIndex((int)Section::StreamKey);
#endif
}

void OBSBasicSettings::OnAuthConnected()
{
	// #289 service list of radio buttons
	std::string service =
		QT_TO_UTF8(ui->serviceButtonGroup->checkedButton()->text());
	Auth::Type type = Auth::AuthType(service);

	if (type == Auth::Type::OAuth_StreamKey) {
		OnOAuthStreamKeyConnected();
	}

	if (!loading) {
		stream1Changed = true;
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::on_connectAccount_clicked()
{
#ifdef BROWSER_AVAILABLE
	// #289 service list of radio buttons
	std::string service =
		QT_TO_UTF8(ui->serviceButtonGroup->checkedButton()->text());

	OAuth::DeleteCookies(service);

	auth = OAuthStreamKey::Login(this, service);
	if (!!auth)
		OnAuthConnected();
#endif
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
	ui->ignoreRecommended->setVisible(!customServer);
	ui->enforceSettingsLabel->setVisible(!customServer);

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
			text += "\n";
		text += ENFORCE_TEXT("MaxAudioBitrate")
				.arg(QString::number(abitrate));
	}
	if (res_count) {
		if (!text.isEmpty())
			text += "\n";

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
			text += "\n";

		text += ENFORCE_TEXT("MaxFPS").arg(QString::number(fps));
	}
#undef ENFORCE_TEXT

	ui->enforceSettingsLabel->setText(text);
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
		QMetaObject::invokeMethod(ui->ignoreRecommended, "setChecked",
					  Qt::QueuedConnection,
					  Q_ARG(bool, false));
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
// #289 service list of radio buttons
// void OBSBasicSettings::UpdateResFPSLimits()
// {
// 	if (loading)
// 		return;

// 	int idx = ui->service->currentIndex();
// 	if (idx == -1)
// 		return;

// 	bool ignoreRecommended = ui->ignoreRecommended->isChecked();
// 	BPtr<obs_service_resolution> res_list;
// 	size_t res_count = 0;
// 	int max_fps = 0;

// 	if (!IsCustomService() && !ignoreRecommended) {
// 		OBSService service = GetStream1Service();
// 		obs_service_get_supported_resolutions(service, &res_list,
// 						      &res_count);
// 		obs_service_get_max_fps(service, &max_fps);
// 	}

// 	/* ------------------------------------ */
// 	/* Check for enforced res/FPS           */

// 	QString res = ui->outputResolution->currentText();
// 	QString fps_str;
// 	int cx = 0, cy = 0;
// 	double max_fpsd = (double)max_fps;
// 	int closest_fps_index = -1;
// 	double fpsd;

// 	sscanf(QT_TO_UTF8(res), "%dx%d", &cx, &cy);

// 	if (res_count)
// 		set_closest_res(cx, cy, res_list, res_count);

// 	if (max_fps) {
// 		int fpsType = ui->fpsType->currentIndex();

// 		if (fpsType == 1) { //Integer
// 			fpsd = (double)ui->fpsInteger->value();
// 		} else if (fpsType == 2) { //Fractional
// 			fpsd = (double)ui->fpsNumerator->value() /
// 			       (double)ui->fpsDenominator->value();
// 		} else { //Common
// 			sscanf(QT_TO_UTF8(ui->fpsCommon->currentText()), "%lf",
// 			       &fpsd);
// 		}

// 		double closest_diff = 1000000000000.0;

// 		for (int i = 0; i < ui->fpsCommon->count(); i++) {
// 			double com_fpsd;
// 			sscanf(QT_TO_UTF8(ui->fpsCommon->itemText(i)), "%lf",
// 			       &com_fpsd);

// 			if (com_fpsd > max_fpsd) {
// 				continue;
// 			}

// 			double diff = fabs(com_fpsd - fpsd);
// 			if (diff < closest_diff) {
// 				closest_diff = diff;
// 				closest_fps_index = i;
// 				fps_str = ui->fpsCommon->itemText(i);
// 			}
// 		}
// 	}

// 	QString res_str =
// 		QString("%1x%2").arg(QString::number(cx), QString::number(cy));

// 	/* ------------------------------------ */
// 	/* Display message box if res/FPS bad   */

// 	bool valid = ResFPSValid(res_list, res_count, max_fps);

// 	if (!valid) {
// 		/* if the user was already on facebook with an incompatible
// 		 * resolution, assume it's an upgrade */
// 		if (lastServiceIdx == -1 && lastIgnoreRecommended == -1) {
// 			ui->ignoreRecommended->setChecked(true);
// 			ui->ignoreRecommended->setProperty("changed", true);
// 			stream1Changed = true;
// 			EnableApplyButton(true);
// 			UpdateResFPSLimits();
// 			return;
// 		}

// 		QMessageBox::StandardButton button;

// #define WARNING_VAL(x) \
// 	QTStr("Basic.Settings.Output.Warn.EnforceResolutionFPS." x)

// 		QString str;
// 		if (res_count)
// 			str += WARNING_VAL("Resolution").arg(res_str);
// 		if (max_fps) {
// 			if (!str.isEmpty())
// 				str += "\n";
// 			str += WARNING_VAL("FPS").arg(fps_str);
// 		}

// 		button = OBSMessageBox::question(this, WARNING_VAL("Title"),
// 						 WARNING_VAL("Msg").arg(str));
// #undef WARNING_VAL

// 		if (button == QMessageBox::No) {
// 			if (idx != lastServiceIdx)
// 				QMetaObject::invokeMethod(
// 					ui->service, "setCurrentIndex",
// 					Qt::QueuedConnection,
// 					Q_ARG(int, lastServiceIdx));
// 			else
// 				QMetaObject::invokeMethod(ui->ignoreRecommended,
// 							  "setChecked",
// 							  Qt::QueuedConnection,
// 							  Q_ARG(bool, true));
// 			return;
// 		}
// 	}

// 	/* ------------------------------------ */
// 	/* Update widgets/values if switching   */
// 	/* to/from enforced resolution/FPS      */

// 	ui->outputResolution->blockSignals(true);
// 	if (res_count) {
// 		ui->outputResolution->clear();
// 		ui->outputResolution->setEditable(false);

// 		int new_res_index = -1;

// 		for (size_t i = 0; i < res_count; i++) {
// 			obs_service_resolution val = res_list[i];
// 			QString str =
// 				QString("%1x%2").arg(QString::number(val.cx),
// 						     QString::number(val.cy));
// 			ui->outputResolution->addItem(str);

// 			if (val.cx == cx && val.cy == cy)
// 				new_res_index = (int)i;
// 		}

// 		ui->outputResolution->setCurrentIndex(new_res_index);
// 		if (!valid) {
// 			ui->outputResolution->setProperty("changed", true);
// 			videoChanged = true;
// 			EnableApplyButton(true);
// 		}
// 	} else {
// 		QString baseRes = ui->baseResolution->currentText();
// 		int baseCX, baseCY;
// 		sscanf(QT_TO_UTF8(baseRes), "%dx%d", &baseCX, &baseCY);

// 		if (!ui->outputResolution->isEditable()) {
// 			RecreateOutputResolutionWidget();
// 			ui->outputResolution->blockSignals(true);
// 			ResetDownscales((uint32_t)baseCX, (uint32_t)baseCY,
// 					true);
// 			ui->outputResolution->setCurrentText(res);
// 		}
// 	}
// 	ui->outputResolution->blockSignals(false);

// 	if (max_fps) {
// 		for (int i = 0; i < ui->fpsCommon->count(); i++) {
// 			double com_fpsd;
// 			sscanf(QT_TO_UTF8(ui->fpsCommon->itemText(i)), "%lf",
// 			       &com_fpsd);

// 			if (com_fpsd > max_fpsd) {
// 				SetComboItemEnabled(ui->fpsCommon, i, false);
// 				continue;
// 			}
// 		}

// 		ui->fpsType->setCurrentIndex(0);
// 		ui->fpsCommon->setCurrentIndex(closest_fps_index);
// 		if (!valid) {
// 			ui->fpsType->setProperty("changed", true);
// 			ui->fpsCommon->setProperty("changed", true);
// 			videoChanged = true;
// 			EnableApplyButton(true);
// 		}
// 	} else {
// 		for (int i = 0; i < ui->fpsCommon->count(); i++)
// 			SetComboItemEnabled(ui->fpsCommon, i, true);
// 	}

// 	SetComboItemEnabled(ui->fpsType, 1, !max_fps);
// 	SetComboItemEnabled(ui->fpsType, 2, !max_fps);

// 	/* ------------------------------------ */

// 	lastIgnoreRecommended = (int)ignoreRecommended;
// 	lastServiceIdx = idx;
// }
