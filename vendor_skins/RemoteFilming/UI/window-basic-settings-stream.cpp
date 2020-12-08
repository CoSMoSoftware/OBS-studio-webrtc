// Copyright Dr. Alex. Gouaillard (2015, 2020)
#include <QMessageBox>

#include "window-basic-settings.hpp"
#include "obs-frontend-api.h"
#include "obs-app.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"

#ifdef BROWSER_AVAILABLE
#include <browser-panel.hpp>
#include "auth-oauth.hpp"
#endif

struct QCef;
struct QCefCookieManager;

extern QCef *cef;
extern QCefCookieManager *panel_cookies;

enum class ListOpt : int { ShowAll = 1, Custom, Millicast };

enum class Section : int {
	Connect,
	StreamKey,
};

// NOTE LUDO: #167 Settings/Stream: only one service displayed: Millicast
std::vector<std::string> webrtc_services = {"webrtc_millicast"};
std::vector<std::string>::size_type webrtc_count = webrtc_services.size();

inline bool OBSBasicSettings::IsCustomService() const
{
	return false;
}

inline int OBSBasicSettings::IsWebRTC() const
{
	return 1;
}

void OBSBasicSettings::InitStreamPage()
{
	ui->connectAccount2->setVisible(false);
	ui->disconnectAccount->setVisible(false);
	ui->bandwidthTestEnable->setVisible(false);

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

	LoadServices(false);

	connect(ui->serviceButtonGroup, SIGNAL(buttonClicked(int)), this,
		SLOT(UpdateServerList()));
	connect(ui->serviceButtonGroup, SIGNAL(buttonClicked(int)), this,
		SLOT(UpdateKeyLink()));
}

void OBSBasicSettings::LoadStream1Settings()
{
	obs_service_t *service_obj = main->GetService();
	const char *type = obs_service_get_type(service_obj);

	loading = true;

	obs_data_t *settings = obs_service_get_settings(service_obj);

	// NOTE LUDO: #173 replace Settings/Stream service Millicast combo box by a radio button
	// const char *service = obs_data_get_string(settings, "service");
	const char *server = obs_data_get_string(settings, "server");
	const char *key = obs_data_get_string(settings, "key");

	if (strcmp(type, "rtmp_custom") == 0) {
		bool use_auth = obs_data_get_bool(settings, "use_auth");
		const char *username =
			obs_data_get_string(settings, "username");
		const char *password =
			obs_data_get_string(settings, "password");
		ui->authUsername->setText(QT_UTF8(username));
		ui->authPw->setText(QT_UTF8(password));
		ui->useAuth->setChecked(use_auth);
	} else if (strcmp(type, "rtmp_common") == 0) {
		bool bw_test = obs_data_get_bool(settings, "bwtest");
		ui->bandwidthTestEnable->setChecked(bw_test);
	} else {
		const char *room = obs_data_get_string(settings, "room");
		const char *username =
			obs_data_get_string(settings, "username");
		const char *password =
			obs_data_get_string(settings, "password");

		const char *tmpString = nullptr;

		tmpString = obs_data_get_string(settings, "codec");
		const char *codec =
			// NOTE LUDO: #172 codecs list of radio buttons
			// 	strcmp("", tmpString) == 0 ? "Automatic" : tmpString;
			strcmp("", tmpString) == 0 ? "vp9" : tmpString;

		tmpString = obs_data_get_string(settings, "protocol");
		const char *protocol = strcmp("", tmpString) == 0 ? "Automatic"
								  : tmpString;

		int idx = 0;
		for (std::vector<std::string>::size_type i = 0;
		     i < webrtc_count; ++i)
			if (std::string(type) == webrtc_services[i]) {
				idx = i + 1;
				break;
			}

		// NOTE LUDO: #173 replace Settings/Stream service Millicast combo box by a radio button
		// ui->service->setCurrentIndex(idx);
		// NOTE LUDO: #185 Settings/Stream replace server name QStackedWidget by a QLineEdit
		// ui->customServer->setText(server);
		ui->serverName->setText(server);
		ui->room->setText(QT_UTF8(room));
		ui->authUsername->setText(QT_UTF8(username));
		ui->authPw->setText(QT_UTF8(password));
		bool use_auth = true;
		ui->useAuth->setChecked(use_auth);

		// NOTE LUDO: #172 codecs list of radio buttons
		// int idxC = ui->codec->findText(codec);
		// ui->codec->setCurrentIndex(idxC);
		QList<QAbstractButton *> listButtons =
			ui->codecButtonGroup->buttons();
		QList<QAbstractButton *>::iterator iter;
		for (iter = listButtons.begin(); iter != listButtons.end();
		     ++iter) {
			QRadioButton *radiobutton =
				reinterpret_cast<QRadioButton *>(*iter);
			if (strcmp(codec,
				   radiobutton->text().toStdString().c_str()) ==
			    0) {
				radiobutton->setChecked(true);
				break;
			}
		}

		int idxP = ui->streamProtocol->findText(protocol);
		ui->streamProtocol->setCurrentIndex(idxP);
	}

	UpdateServerList();

	if (strcmp(type, "rtmp_common") == 0) {
		// NOTE LUDO: #185 Settings/Stream replace server name QStackedWidget by a QLineEdit
		// int idx = ui->server->findData(server);
		// if (idx == -1) {
		// 	if (server && *server)
		// 		ui->server->insertItem(0, server, server);
		// 	idx = 0;
		// }
		// ui->server->setCurrentIndex(idx);
	}

	ui->key->setText(key);

	lastService.clear();
	on_service_currentIndexChanged(1);

	obs_data_release(settings);

	UpdateKeyLink();

	bool streamActive = obs_frontend_streaming_active();
	ui->streamPage->setEnabled(!streamActive);

	loading = false;
}

void OBSBasicSettings::SaveStream1Settings()
{
	bool customServer = IsCustomService();
	int webrtc = IsWebRTC();

	const char *service_id = "webrtc_millicast";

	obs_service_t *oldService = main->GetService();
	OBSData hotkeyData = obs_hotkeys_save_service(oldService);
	obs_data_release(hotkeyData);

	OBSData settings = obs_data_create();
	obs_data_release(settings);

	if (!customServer && webrtc == 0) {
		// NOTE LUDO: #173 replace Settings/Stream service Millicast combo box by a radio button
		// obs_data_set_string(settings, "service",
		// 		    QT_TO_UTF8(ui->service->currentText()));
		// NOTE LUDO: #185 Settings/Stream replace server name QStackedWidget by a QLineEdit
		// obs_data_set_string(
		// 	settings, "server",
		// 	QT_TO_UTF8(ui->server->currentData().toString()));
	} else if (customServer && webrtc == 0) {
		// NOTE LUDO: #185 Settings/Stream replace server name QStackedWidget by a QLineEdit
		// obs_data_set_string(settings, "server",
		// 		    QT_TO_UTF8(ui->customServer->text()));
		obs_data_set_bool(settings, "use_auth",
				  ui->useAuth->isChecked());
		if (ui->useAuth->isChecked()) {
			obs_data_set_string(
				settings, "username",
				QT_TO_UTF8(ui->authUsername->text()));
			obs_data_set_string(settings, "password",
					    QT_TO_UTF8(ui->authPw->text()));
		}
	} else if (webrtc > 0) {
		obs_data_set_string(
			settings, "server",
			// NOTE LUDO: #185 Settings/Stream replace server name QStackedWidget
			// by a QLineEdit
			// QT_TO_UTF8(ui->customServer->text()));
			QT_TO_UTF8(ui->serverName->text()));
		obs_data_set_string(settings, "room",
				    QT_TO_UTF8(ui->room->text()));
		obs_data_set_string(settings, "username",
				    QT_TO_UTF8(ui->authUsername->text()));
		obs_data_set_string(settings, "password",
				    QT_TO_UTF8(ui->authPw->text()));
		// NOTE LUDO: #172 codecs list of radio buttons
		obs_data_set_string(
			settings, "codec",
			QT_TO_UTF8(
				ui->codecButtonGroup->checkedButton()->text()));
		obs_data_set_string(
			settings, "protocol",
			QT_TO_UTF8(ui->streamProtocol->currentText()));
	}

	obs_data_set_bool(settings, "bwtest",
			  ui->bandwidthTestEnable->isChecked());
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
}

void OBSBasicSettings::UpdateKeyLink()
{
	bool custom = IsCustomService();
	int webrtc = IsWebRTC();
	// NOTE LUDO: #173 replace Settings/Stream service Millicast combo box by a radio button
	// QString serviceName = ui->service->currentText();
	QString serviceName = ui->serviceButtonGroup->checkedButton()->text();

	if (custom || webrtc > 0)
		serviceName = "";

	QString text = QTStr("Basic.AutoConfig.StreamPage.StreamKey");
	if (serviceName == "Twitch") {
		text += " <a href=\"https://";
		text += "www.twitch.tv/broadcast/dashboard/streamkey";
		text += "\">";
		text += QTStr(
			"Basic.AutoConfig.StreamPage.StreamKey.LinkToSite");
		text += "</a>";
	} else if (serviceName == "YouTube / YouTube Gaming") {
		text += " <a href=\"https://";
		text += "www.youtube.com/live_dashboard";
		text += "\">";
		text += QTStr(
			"Basic.AutoConfig.StreamPage.StreamKey.LinkToSite");
		text += "</a>";
	} else if (serviceName.startsWith("Restream.io")) {
		text += " <a href=\"https://";
		text += "restream.io/settings/streaming-setup?from=OBS";
		text += "\">";
		text += QTStr(
			"Basic.AutoConfig.StreamPage.StreamKey.LinkToSite");
		text += "</a>";
	} else if (serviceName == "Facebook Live") {
		text += " <a href=\"https://";
		text += "www.facebook.com/live/create";
		text += "\">";
		text += QTStr(
			"Basic.AutoConfig.StreamPage.StreamKey.LinkToSite");
		text += "</a>";
	}

	ui->streamKeyLabel->setText(text);
}

void OBSBasicSettings::LoadServices(bool showAll)
{
	obs_properties_t *props =
		obs_get_service_properties("webrtc_millicast");

	OBSData settings = obs_data_create();
	obs_data_release(settings);

	obs_data_set_bool(settings, "show_all", showAll);

	obs_property_t *prop = obs_properties_get(props, "show_all");
	obs_property_modified(prop, settings);

	obs_properties_destroy(props);
}

static inline bool is_auth_service(const std::string &service)
{
	return Auth::AuthType(service) != Auth::Type::None;
}

void OBSBasicSettings::on_service_currentIndexChanged(int)
{
	std::string service =
		QT_TO_UTF8(ui->serviceButtonGroup->checkedButton()->text());
	bool custom = IsCustomService();
	int webrtc = IsWebRTC();

	ui->disconnectAccount->setVisible(false);
	ui->bandwidthTestEnable->setVisible(false);

#ifdef BROWSER_AVAILABLE
	if (cef && webrtc == 0) {
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

	ui->useAuth->setVisible(custom && webrtc == 0);
	ui->authUsernameLabel->setVisible(custom || webrtc > 0);
	ui->authUsername->setVisible(custom || webrtc > 0);
	ui->authPwLabel->setVisible(custom);
	ui->authPwWidget->setVisible(custom);

	if (custom && webrtc == 0) {
		ui->authUsernameLabel->setText("Username");
		ui->authPwLabel->setText("Password");
		// NOTE LUDO: #185 Settings/Stream replace server name QStackedWidget by a QLineEdit
		// ui->streamkeyPageLayout->insertRow(1, ui->serverLabel,
		// 				   ui->serverStackedWidget);
		ui->streamkeyPageLayout->insertRow(2, ui->streamKeyLabel,
						   ui->streamKeyWidget);
		ui->streamkeyPageLayout->insertRow(3, nullptr, ui->useAuth);
		ui->streamkeyPageLayout->insertRow(4, ui->authUsernameLabel,
						   ui->authUsername);
		ui->streamkeyPageLayout->insertRow(5, ui->authPwLabel,
						   ui->authPwWidget);
		ui->streamkeyPageLayout->insertRow(7, ui->streamProtocolLabel,
						   ui->streamProtocol);
		ui->serverLabel->setVisible(true);
		ui->streamKeyLabel->setVisible(true);
		ui->streamKeyWidget->setVisible(true);
		ui->roomLabel->setVisible(false);
		ui->room->setVisible(false);
		on_useAuth_toggled();
		ui->codecLabel->setVisible(false);
		ui->streamProtocolLabel->setVisible(false);
		ui->streamProtocol->setVisible(false);
	} else if (webrtc > 0) {
		ui->streamKeyLabel->setVisible(false);
		ui->streamKeyWidget->setVisible(false);
		ui->serverLabel->setVisible(true);
		ui->serverName->setVisible(true);
		obs_properties_t *props =
			obs_get_service_properties("webrtc_millicast");
		obs_property_t *server = obs_properties_get(props, "server");
		obs_property_t *room = obs_properties_get(props, "room");
		obs_property_t *username =
			obs_properties_get(props, "username");
		obs_property_t *password =
			obs_properties_get(props, "password");
		obs_property_t *codec = obs_properties_get(props, "codec");
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
				// NOTE LUDO: #185 Settings/Stream replace server name QStackedWidget by a QLineEdit
				//  ui->serverStackedWidget);
				ui->serverName);
			min_idx++;
		}
		if (obs_property_visible(room)) {
			ui->streamkeyPageLayout->insertRow(
				min_idx, ui->roomLabel, ui->room);
			min_idx++;
		}
		if (true) { // NOTE ALEX: FIXME obs_property_visible(username)) {
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
		if (obs_property_visible(codec)) {
			// NOTE LUDO: #172 codecs list of radio buttons
			// ui->streamkeyPageLayout->insertRow(min_idx, ui->codecLabel,
			// 				   ui->codec);
			// min_idx++;
		}
		if (obs_property_visible(protocol)) {
			ui->streamkeyPageLayout->insertRow(
				min_idx, ui->streamProtocolLabel,
				ui->streamProtocol);
			min_idx++;
		}
		ui->serverLabel->setVisible(obs_property_visible(server));
		// NOTE LUDO: #185 Settings/Stream replace server name QStackedWidget by a QLineEdit
		// ui->serverStackedWidget->setVisible(obs_property_visible(server));
		ui->serverName->setVisible(obs_property_visible(server));
		ui->roomLabel->setVisible(obs_property_visible(room));
		ui->room->setVisible(obs_property_visible(room));
		ui->authUsernameLabel->setVisible(
			true); // NOTE ALEX: FIXME obs_property_visible(username));
		ui->authUsername->setVisible(
			true); // NOTE ALEX: FIXME obs_property_visible(username));
		ui->authPwLabel->setVisible(obs_property_visible(password));
		ui->authPwWidget->setVisible(obs_property_visible(password));
		ui->codecLabel->setVisible(obs_property_visible(codec));
		// NOTE LUDO: #172 codecs list of radio buttons
		// ui->codec->setVisible(obs_property_visible(codec));
		ui->streamProtocolLabel->setVisible(
			obs_property_visible(protocol));
		ui->streamProtocol->setVisible(obs_property_visible(protocol));
		obs_properties_destroy(props);
	} else if (!custom && webrtc == 0) { // rtmp_common
		ui->authUsernameLabel->setText("Username");
		ui->authPwLabel->setText("Password");
		// NOTE LUDO: #185 Settings/Stream replace server name QStackedWidget by a QLineEdit
		// ui->streamkeyPageLayout->insertRow(1, ui->serverLabel,
		// 					ui->serverStackedWidget);
		ui->streamkeyPageLayout->insertRow(2, ui->streamKeyLabel,
						   ui->streamKeyWidget);
		ui->streamkeyPageLayout->insertRow(3, NULL, ui->useAuth);
		ui->streamkeyPageLayout->insertRow(4, ui->authUsernameLabel,
						   ui->authUsername);
		ui->streamkeyPageLayout->insertRow(5, ui->authPwLabel,
						   ui->authPwWidget);
		// NOTE LUDO: #172 codecs list of radio buttons
		// ui->streamkeyPageLayout->insertRow(6, ui->codecLabel,
		// 					ui->codec);
		ui->streamkeyPageLayout->insertRow(7, ui->streamProtocolLabel,
						   ui->streamProtocol);
		// NOTE LUDO: #185 Settings/Stream replace server name QStackedWidget by a QLineEdit
		// ui->serverStackedWidget->setCurrentIndex(0);
		ui->serverLabel->setVisible(true);
		// NOTE LUDO: #185 Settings/Stream replace server name QStackedWidget by a QLineEdit
		// ui->serverStackedWidget->setVisible(true);
		ui->streamKeyLabel->setVisible(true);
		ui->streamKeyWidget->setVisible(true);
		ui->roomLabel->setVisible(false);
		ui->room->setVisible(false);
		ui->streamProtocolLabel->setVisible(false);
		ui->streamProtocol->setVisible(false);
		ui->codecLabel->setVisible(false);
		// NOTE LUDO: #172 codecs list of radio buttons
		// ui->codec->setVisible(false);
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
	QString serviceName = ui->serviceButtonGroup->checkedButton()->text();
	bool showMore = false;

	if (showMore) {
		LoadServices(true);
		return;
	} else {
		lastService = serviceName;
	}

	obs_properties_t *props = obs_get_service_properties("rtmp_common");
	obs_property_t *services = obs_properties_get(props, "service");

	OBSData settings = obs_data_create();
	obs_data_release(settings);

	obs_data_set_string(settings, "service", QT_TO_UTF8(serviceName));
	obs_property_modified(services, settings);

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
	int webrtc = IsWebRTC();

	const char *service_id = "webrtc_millicast";

	OBSData settings = obs_data_create();
	obs_data_release(settings);

	if (!custom && webrtc == 0) {
		obs_data_set_string(
			settings, "service",
			QT_TO_UTF8(ui->serviceButtonGroup->checkedButton()
					   ->text()));
		obs_data_set_string(settings, "server",
				    QT_TO_UTF8(ui->serverName->text()));
	} else {
		obs_data_set_string(settings, "server",
				    QT_TO_UTF8(ui->serverName->text()));
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

		if (strcmp(a->service(), "Twitch") == 0)
			ui->bandwidthTestEnable->setVisible(true);
	}

	ui->streamStackWidget->setCurrentIndex((int)Section::StreamKey);
#endif
}

void OBSBasicSettings::OnAuthConnected()
{
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
	std::string service = QT_TO_UTF8(ui->service->currentText());

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

	std::string service =
		QT_TO_UTF8(ui->serviceButtonGroup->checkedButton()->text());

#ifdef BROWSER_AVAILABLE
	OAuth::DeleteCookies(service);
#endif

	int webrtc = IsWebRTC();

	if (webrtc > 0) {
		ui->streamKeyWidget->setVisible(false);
		ui->streamKeyLabel->setVisible(false);
	} else {
		ui->streamKeyWidget->setVisible(true);
		ui->streamKeyLabel->setVisible(true);
	}

	ui->connectAccount2->setVisible(true);
	ui->disconnectAccount->setVisible(false);
	ui->bandwidthTestEnable->setVisible(false);
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
