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

enum class ListOpt : int {
	ShowAll = 1,
	Custom,
};

enum class Section : int {
	Connect,
	StreamKey,
};

inline bool OBSBasicSettings::IsCustomService() const
{
	return ui->service->currentData().toInt() == (int)ListOpt::Custom;
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

	LoadServices(false);

	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.None"));
	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.BTTV"));
	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.FFZ"));
	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.Both"));

	connect(ui->service, SIGNAL(currentIndexChanged(int)), this,
		SLOT(UpdateServerList()));
	connect(ui->service, SIGNAL(currentIndexChanged(int)), this,
		SLOT(UpdateKeyLink()));
	connect(ui->customServer, SIGNAL(textChanged(const QString &)), this,
		SLOT(UpdateKeyLink()));
	connect(ui->customServer, SIGNAL(editingFinished(const QString &)),
		this, SLOT(UpdateKeyLink()));
}

void OBSBasicSettings::LoadStream1Settings()
{
	obs_service_t *service_obj = main->GetService();
	const char *type = obs_service_get_type(service_obj);

	loading = true;

	obs_data_t *settings = obs_service_get_settings(service_obj);

	const char *service = obs_data_get_string(settings, "service");
	const char *server = obs_data_get_string(settings, "server");
	const char *key = obs_data_get_string(settings, "key");

	if (strcmp(type, "rtmp_custom") == 0) {
		ui->service->setCurrentIndex(0);
		ui->customServer->setText(server);

		bool use_auth = obs_data_get_bool(settings, "use_auth");
		const char *username =
			obs_data_get_string(settings, "username");
		const char *password =
			obs_data_get_string(settings, "password");
		ui->authUsername->setText(QT_UTF8(username));
		ui->authPw->setText(QT_UTF8(password));
		ui->useAuth->setChecked(use_auth);
	} else {
		int idx = ui->service->findText(service);
		if (idx == -1) {
			if (service && *service)
				ui->service->insertItem(1, service);
			idx = 1;
		}
		ui->service->setCurrentIndex(idx);

		bool bw_test = obs_data_get_bool(settings, "bwtest");
		ui->bandwidthTestEnable->setChecked(bw_test);

		idx = config_get_int(main->Config(), "Twitch", "AddonChoice");
		ui->twitchAddonDropdown->setCurrentIndex(idx);
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

	bool streamActive = obs_frontend_streaming_active();
	ui->streamPage->setEnabled(!streamActive);

	loading = false;
}

void OBSBasicSettings::SaveStream1Settings()
{
	bool customServer = IsCustomService();
	const char *service_id = customServer ? "rtmp_custom" : "rtmp_common";

	obs_service_t *oldService = main->GetService();
	OBSData hotkeyData = obs_hotkeys_save_service(oldService);
	obs_data_release(hotkeyData);

	OBSData settings = obs_data_create();
	obs_data_release(settings);

	if (!customServer) {
		obs_data_set_string(settings, "service",
				    QT_TO_UTF8(ui->service->currentText()));
		obs_data_set_string(
			settings, "server",
			QT_TO_UTF8(ui->server->currentData().toString()));
	} else {
		obs_data_set_string(settings, "server",
				    QT_TO_UTF8(ui->customServer->text()));
		obs_data_set_bool(settings, "use_auth",
				  ui->useAuth->isChecked());
		if (ui->useAuth->isChecked()) {
			obs_data_set_string(
				settings, "username",
				QT_TO_UTF8(ui->authUsername->text()));
			obs_data_set_string(settings, "password",
					    QT_TO_UTF8(ui->authPw->text()));
		}
	}

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
}

void OBSBasicSettings::UpdateKeyLink()
{
	QString serviceName = ui->service->currentText();
	QString customServer = ui->customServer->text();
	QString streamKeyLink;
	if (serviceName == "Twitch") {
		streamKeyLink =
			"https://www.twitch.tv/broadcast/dashboard/streamkey";
	} else if (serviceName == "YouTube / YouTube Gaming") {
		streamKeyLink = "https://www.youtube.com/live_dashboard";
	} else if (serviceName.startsWith("Restream.io")) {
		streamKeyLink =
			"https://restream.io/settings/streaming-setup?from=OBS";
	} else if (serviceName == "Facebook Live" ||
		   (customServer.contains("fbcdn.net") && IsCustomService())) {
		streamKeyLink =
			"https://www.facebook.com/live/producer?ref=OBS";
	} else if (serviceName.startsWith("Twitter")) {
		streamKeyLink = "https://www.pscp.tv/account/producer";
	} else if (serviceName.startsWith("YouStreamer")) {
		streamKeyLink = "https://app.youstreamer.com/stream/";
	} else if (serviceName == "Trovo") {
		streamKeyLink = "https://studio.trovo.live/mychannel/stream";
	}

	if (QString(streamKeyLink).isNull()) {
		ui->getStreamKeyButton->hide();
	} else {
		ui->getStreamKeyButton->setTargetUrl(QUrl(streamKeyLink));
		ui->getStreamKeyButton->show();
	}
}

void OBSBasicSettings::LoadServices(bool showAll)
{
	obs_properties_t *props = obs_get_service_properties("rtmp_common");

	OBSData settings = obs_data_create();
	obs_data_release(settings);

	obs_data_set_bool(settings, "show_all", showAll);

	obs_property_t *prop = obs_properties_get(props, "show_all");
	obs_property_modified(prop, settings);

	ui->service->blockSignals(true);
	ui->service->clear();

	QStringList names;

	obs_property_t *services = obs_properties_get(props, "service");
	size_t services_count = obs_property_list_item_count(services);
	for (size_t i = 0; i < services_count; i++) {
		const char *name = obs_property_list_item_string(services, i);
		names.push_back(name);
	}

	if (showAll)
		names.sort(Qt::CaseInsensitive);

	for (QString &name : names)
		ui->service->addItem(name);

	if (!showAll) {
		ui->service->addItem(
			QTStr("Basic.AutoConfig.StreamPage.Service.ShowAll"),
			QVariant((int)ListOpt::ShowAll));
	}

	ui->service->insertItem(
		0, QTStr("Basic.AutoConfig.StreamPage.Service.Custom"),
		QVariant((int)ListOpt::Custom));

	if (!lastService.isEmpty()) {
		int idx = ui->service->findText(lastService);
		if (idx != -1)
			ui->service->setCurrentIndex(idx);
	}

	obs_properties_destroy(props);

	ui->service->blockSignals(false);
}

static inline bool is_auth_service(const std::string &service)
{
	return Auth::AuthType(service) != Auth::Type::None;
}

void OBSBasicSettings::on_service_currentIndexChanged(int)
{
	bool showMore = ui->service->currentData().toInt() ==
			(int)ListOpt::ShowAll;
	if (showMore)
		return;

	std::string service = QT_TO_UTF8(ui->service->currentText());
	bool custom = IsCustomService();

	ui->disconnectAccount->setVisible(false);
	ui->bandwidthTestEnable->setVisible(false);
	ui->twitchAddonDropdown->setVisible(false);
	ui->twitchAddonLabel->setVisible(false);

#ifdef BROWSER_AVAILABLE
	if (cef) {
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

	ui->useAuth->setVisible(custom);
	ui->authUsernameLabel->setVisible(custom);
	ui->authUsername->setVisible(custom);
	ui->authPwLabel->setVisible(custom);
	ui->authPwWidget->setVisible(custom);

	if (custom) {
		ui->streamkeyPageLayout->insertRow(1, ui->serverLabel,
						   ui->serverStackedWidget);

		ui->serverStackedWidget->setCurrentIndex(1);
		ui->serverStackedWidget->setVisible(true);
		ui->serverLabel->setVisible(true);
		on_useAuth_toggled();
	} else {
		ui->serverStackedWidget->setCurrentIndex(0);
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
	QString serviceName = ui->service->currentText();
	bool showMore = ui->service->currentData().toInt() ==
			(int)ListOpt::ShowAll;

	if (showMore) {
		LoadServices(true);
		ui->service->showPopup();
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
	const char *service_id = custom ? "rtmp_custom" : "rtmp_common";

	OBSData settings = obs_data_create();
	obs_data_release(settings);

	if (!custom) {
		obs_data_set_string(settings, "service",
				    QT_TO_UTF8(ui->service->currentText()));
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
	std::string service = QT_TO_UTF8(ui->service->currentText());
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

	std::string service = QT_TO_UTF8(ui->service->currentText());

#ifdef BROWSER_AVAILABLE
	OAuth::DeleteCookies(service);
#endif

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
