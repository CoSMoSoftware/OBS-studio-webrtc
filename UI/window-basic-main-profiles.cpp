/******************************************************************************
    Copyright (C) 2015 by Hugh Bailey <obs.jim@gmail.com>

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

#include <obs.hpp>
#include <util/platform.h>
#include <util/util.hpp>
#include <QMessageBox>
#include <QVariant>
#include <QFileDialog>
#include "window-basic-main.hpp"
#include "window-basic-auto-config.hpp"
#include "window-namedialog.hpp"
#include "qt-wrappers.hpp"

extern void DestroyPanelCookieManager();
extern void DuplicateCurrentCookieProfile(ConfigFile &config);
extern void CheckExistingCookieId();
extern void DeleteCookies();

void EnumProfiles(std::function<bool(const char *, const char *)> &&cb)
{
	char path[512];
	os_glob_t *glob;

	int ret = GetConfigPath(
		path, sizeof(path),
		(std::string(CONFIG_DIR) + "/basic/profiles/*").c_str());
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get profiles config path");
		return;
	}

	if (os_glob(path, 0, &glob) != 0) {
		blog(LOG_WARNING, "Failed to glob profiles");
		return;
	}

	for (size_t i = 0; i < glob->gl_pathc; i++) {
		const char *filePath = glob->gl_pathv[i].path;
		const char *dirName = strrchr(filePath, '/') + 1;

		if (!glob->gl_pathv[i].directory)
			continue;

		if (strcmp(dirName, ".") == 0 || strcmp(dirName, "..") == 0)
			continue;

		std::string file = filePath;
		file += "/basic.ini";

		ConfigFile config;
		int ret = config.Open(file.c_str(), CONFIG_OPEN_EXISTING);
		if (ret != CONFIG_SUCCESS)
			continue;

		const char *name = config_get_string(config, "General", "Name");
		if (!name)
			name = strrchr(filePath, '/') + 1;

		if (!cb(name, filePath))
			break;
	}

	os_globfree(glob);
}

static bool GetProfileDir(const char *findName, const char *&profileDir)
{
	bool found = false;
	auto func = [&](const char *name, const char *path) {
		if (strcmp(name, findName) == 0) {
			found = true;
			profileDir = strrchr(path, '/') + 1;
			return false;
		}
		return true;
	};

	EnumProfiles(func);
	return found;
}

static bool ProfileExists(const char *findName)
{
	const char *profileDir = nullptr;
	return GetProfileDir(findName, profileDir);
}

static bool AskForProfileName(QWidget *parent, std::string &name,
			      const char *title, const char *text,
			      const bool showWizard, bool &wizardChecked,
			      const char *oldName = nullptr)
{
	for (;;) {
		bool success = false;

		if (showWizard) {
			success = NameDialog::AskForNameWithOption(
				parent, title, text, name,
				QTStr("AddProfile.WizardCheckbox"),
				wizardChecked, QT_UTF8(oldName));
		} else {
			success = NameDialog::AskForName(
				parent, title, text, name, QT_UTF8(oldName));
		}

		if (!success) {
			return false;
		}
		if (name.empty()) {
			OBSMessageBox::warning(parent,
					       QTStr("NoNameEntered.Title"),
					       QTStr("NoNameEntered.Text"));
			continue;
		}
		if (ProfileExists(name.c_str())) {
			OBSMessageBox::warning(parent,
					       QTStr("NameExists.Title"),
					       QTStr("NameExists.Text"));
			continue;
		}
		break;
	}
	return true;
}

static bool FindSafeProfileDirName(const std::string &profileName,
				   std::string &dirName)
{
	char path[512];
	int ret;

	if (ProfileExists(profileName.c_str())) {
		blog(LOG_WARNING, "Profile '%s' exists", profileName.c_str());
		return false;
	}

	if (!GetFileSafeName(profileName.c_str(), dirName)) {
		blog(LOG_WARNING, "Failed to create safe file name for '%s'",
		     profileName.c_str());
		return false;
	}

	ret = GetConfigPath(
		path, sizeof(path),
		(std::string(CONFIG_DIR) + "/basic/profiles/").c_str());
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get profiles config path");
		return false;
	}

	dirName.insert(0, path);

	if (!GetClosestUnusedFileName(dirName, nullptr)) {
		blog(LOG_WARNING, "Failed to get closest file name for %s",
		     dirName.c_str());
		return false;
	}

	dirName.erase(0, ret);
	return true;
}

static bool CopyProfile(const char *fromPartial, const char *to)
{
	os_glob_t *glob;
	char path[514];
	char dir[512];
	int ret;

	ret = GetConfigPath(
		dir, sizeof(dir),
		(std::string(CONFIG_DIR) + "/basic/profiles/").c_str());
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get profiles config path");
		return false;
	}

	snprintf(path, sizeof(path), "%s%s/*", dir, fromPartial);

	if (os_glob(path, 0, &glob) != 0) {
		blog(LOG_WARNING, "Failed to glob profile '%s'", fromPartial);
		return false;
	}

	for (size_t i = 0; i < glob->gl_pathc; i++) {
		const char *filePath = glob->gl_pathv[i].path;
		if (glob->gl_pathv[i].directory)
			continue;

		ret = snprintf(path, sizeof(path), "%s/%s", to,
			       strrchr(filePath, '/') + 1);
		if (ret > 0) {
			if (os_copyfile(filePath, path) != 0) {
				blog(LOG_WARNING,
				     "CopyProfile: Failed to "
				     "copy file %s to %s",
				     filePath, path);
			}
		}
	}

	os_globfree(glob);

	return true;
}

static bool ProfileNeedsRestart(config_t *newConfig, QString &settings)
{
	OBSBasic *main = OBSBasic::Get();

	const char *oldSpeakers =
		config_get_string(main->Config(), "Audio", "ChannelSetup");
	uint oldSampleRate =
		config_get_uint(main->Config(), "Audio", "SampleRate");

	const char *newSpeakers =
		config_get_string(newConfig, "Audio", "ChannelSetup");
	uint newSampleRate = config_get_uint(newConfig, "Audio", "SampleRate");

	auto appendSetting = [&settings](const char *name) {
		settings += QStringLiteral("\n") + QTStr(name);
	};

	bool result = false;
	if (oldSpeakers != NULL && newSpeakers != NULL) {
		result = strcmp(oldSpeakers, newSpeakers) != 0;
		appendSetting("Basic.Settings.Audio.Channels");
	}
	if (oldSampleRate != 0 && newSampleRate != 0) {
		result |= oldSampleRate != newSampleRate;
		appendSetting("Basic.Settings.Audio.SampleRate");
	}

	return result;
}

bool OBSBasic::AddProfile(bool create_new, const char *title, const char *text,
			  const char *init_text, bool rename)
{
	std::string name;

	bool showWizardChecked = config_get_bool(App()->GlobalConfig(), "Basic",
						 "ConfigOnNewProfile");
	// #277 Disable auto-configuration
	create_new = false;
	showWizardChecked = false;

	if (!AskForProfileName(this, name, title, text, create_new,
			       showWizardChecked, init_text))
		return false;

	return CreateProfile(name, create_new, showWizardChecked, rename);
}

bool OBSBasic::CreateProfile(const std::string &newName, bool create_new,
			     bool showWizardChecked, bool rename)
{
	std::string newDir;
	std::string newPath;
	ConfigFile config;

	if (!FindSafeProfileDirName(newName, newDir))
		return false;

	if (create_new) {
		config_set_bool(App()->GlobalConfig(), "Basic",
				"ConfigOnNewProfile", showWizardChecked);
	}

	std::string curDir =
		config_get_string(App()->GlobalConfig(), "Basic", "ProfileDir");

	char baseDir[512];
	int ret = GetConfigPath(
		baseDir, sizeof(baseDir),
		(std::string(CONFIG_DIR) + "/basic/profiles/").c_str());
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get profiles config path");
		return false;
	}

	newPath = baseDir;
	newPath += newDir;

	if (os_mkdir(newPath.c_str()) < 0) {
		blog(LOG_WARNING, "Failed to create profile directory '%s'",
		     newDir.c_str());
		return false;
	}

	if (!create_new)
		CopyProfile(curDir.c_str(), newPath.c_str());

	newPath += "/basic.ini";

	if (config.Open(newPath.c_str(), CONFIG_OPEN_ALWAYS) != 0) {
		blog(LOG_ERROR, "Failed to open new config file '%s'",
		     newDir.c_str());
		return false;
	}

	if (api && !rename)
		api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGING);

	config_set_string(App()->GlobalConfig(), "Basic", "Profile",
			  newName.c_str());
	config_set_string(App()->GlobalConfig(), "Basic", "ProfileDir",
			  newDir.c_str());

	Auth::Save();
	if (create_new) {
		auth.reset();
		DestroyPanelCookieManager();
	} else if (!rename) {
		DuplicateCurrentCookieProfile(config);
	}

	config_set_string(config, "General", "Name", newName.c_str());
	basicConfig.SaveSafe("tmp");
	config.SaveSafe("tmp");
	config.Swap(basicConfig);
	InitBasicConfigDefaults();
	InitBasicConfigDefaults2();
	RefreshProfiles();

	if (create_new)
		ResetProfileData();

	blog(LOG_INFO, "Created profile '%s' (%s, %s)", newName.c_str(),
	     create_new ? "clean" : "duplicate", newDir.c_str());
	blog(LOG_INFO, "------------------------------------------------");

	config_save_safe(App()->GlobalConfig(), "tmp", nullptr);
	UpdateTitleBar();
	UpdateVolumeControlsDecayRate();

	Auth::Load();

	// Run auto configuration setup wizard when a new profile is made to assist
	// setting up blank settings
	if (create_new && showWizardChecked) {
		AutoConfig wizard(this);
		wizard.setModal(true);
		wizard.show();
		wizard.exec();
	}

	if (api && !rename) {
		api->on_event(OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED);
		api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGED);
	}
	return true;
}

bool OBSBasic::NewProfile(const QString &name)
{
	return CreateProfile(name.toStdString(), true, false, false);
}

bool OBSBasic::DuplicateProfile(const QString &name)
{
	return CreateProfile(name.toStdString(), false, false, false);
}

void OBSBasic::DeleteProfile(const char *profileName, const char *profileDir)
{
	char profilePath[512];
	char basePath[512];

	int ret = GetConfigPath(basePath, sizeof(basePath),
				(std::string(CONFIG_DIR) + "/basic/profiles").c_str());
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get profiles config path");
		return;
	}

	ret = snprintf(profilePath, sizeof(profilePath), "%s/%s/*", basePath, profileDir);
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get path for profile dir '%s'",
		     profileDir);
		return;
	}

	os_glob_t *glob;
	if (os_glob(profilePath, 0, &glob) != 0) {
		blog(LOG_WARNING, "Failed to glob profile dir '%s'",
		     profileDir);
		return;
	}

	for (size_t i = 0; i < glob->gl_pathc; i++) {
		const char *filePath = glob->gl_pathv[i].path;

		if (glob->gl_pathv[i].directory)
			continue;

		os_unlink(filePath);
	}

	os_globfree(glob);

	ret = snprintf(profilePath, sizeof(profilePath), "%s/%s", basePath, profileDir);
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get path for profile dir '%s'",
		     profileDir);
		return;
	}

	os_rmdir(profilePath);

	blog(LOG_INFO, "------------------------------------------------");
	blog(LOG_INFO, "Removed profile '%s' (%s)", profileName, profileDir);
	blog(LOG_INFO, "------------------------------------------------");
}

void OBSBasic::DeleteProfile(const QString &profileName)
{
	std::string name = profileName.toStdString();
	const char *curName =
		config_get_string(App()->GlobalConfig(), "Basic", "Profile");

	if (strcmp(curName, name.c_str()) == 0) {
		on_actionRemoveProfile_triggered(true);
		return;
	}

	const char *profileDir = nullptr;
	if (!GetProfileDir(name.c_str(), profileDir)) {
		blog(LOG_WARNING, "Profile '%s' not found", name.c_str());
		return;
	}

	if (!profileDir) {
		blog(LOG_WARNING, "Failed to get profile dir for profile '%s'",
		     name.c_str());
		return;
	}

	DeleteProfile(name.c_str(), profileDir);
	RefreshProfiles();
	config_save_safe(App()->GlobalConfig(), "tmp", nullptr);
	if (api)
		api->on_event(OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED);
}

void OBSBasic::RefreshProfiles()
{
	QList<QAction *> menuActions = ui->profileMenu->actions();
	int count = 0;

	for (int i = 0; i < menuActions.count(); i++) {
		QVariant v = menuActions[i]->property("file_name");
		if (v.typeName() != nullptr)
			delete menuActions[i];
	}

	const char *curName =
		config_get_string(App()->GlobalConfig(), "Basic", "Profile");

	auto addProfile = [&](const char *name, const char *path) {
		std::string file = strrchr(path, '/') + 1;

		QAction *action = new QAction(QT_UTF8(name), this);
		action->setProperty("file_name", QT_UTF8(path));
		connect(action, &QAction::triggered, this,
			&OBSBasic::ChangeProfile);
		action->setCheckable(true);

		action->setChecked(strcmp(name, curName) == 0);

		ui->profileMenu->addAction(action);
		count++;
		return true;
	};

	EnumProfiles(addProfile);

	ui->actionRemoveProfile->setEnabled(count > 1);
}

void OBSBasic::ResetProfileData()
{
	ResetVideo();
	service = nullptr;
	InitService();
	ResetOutputs();
	ClearHotkeys();
	CreateHotkeys();

	/* load audio monitoring */
	if (obs_audio_monitoring_available()) {
		const char *device_name = config_get_string(
			basicConfig, "Audio", "MonitoringDeviceName");
		const char *device_id = config_get_string(basicConfig, "Audio",
							  "MonitoringDeviceId");

		obs_set_audio_monitoring_device(device_name, device_id);

		blog(LOG_INFO, "Audio monitoring device:\n\tname: %s\n\tid: %s",
		     device_name, device_id);
	}
}

void OBSBasic::on_actionNewProfile_triggered()
{
	AddProfile(true, Str("AddProfile.Title"), Str("AddProfile.Text"));
}

void OBSBasic::on_actionDupProfile_triggered()
{
	AddProfile(false, Str("AddProfile.Title"), Str("AddProfile.Text"));
}

void OBSBasic::on_actionRenameProfile_triggered()
{
	std::string curDir =
		config_get_string(App()->GlobalConfig(), "Basic", "ProfileDir");
	std::string curName =
		config_get_string(App()->GlobalConfig(), "Basic", "Profile");

	/* Duplicate and delete in case there are any issues in the process */
	bool success = AddProfile(false, Str("RenameProfile.Title"),
				  Str("AddProfile.Text"), curName.c_str(),
				  true);
	if (success) {
		DeleteProfile(curName.c_str(), curDir.c_str());
		RefreshProfiles();
	}

	if (api)
		api->on_event(OBS_FRONTEND_EVENT_PROFILE_RENAMED);
}

void OBSBasic::on_actionRemoveProfile_triggered(bool skipConfirmation)
{
	std::string newName;
	std::string newPath;
	ConfigFile config;

	std::string oldDir =
		config_get_string(App()->GlobalConfig(), "Basic", "ProfileDir");
	std::string oldName =
		config_get_string(App()->GlobalConfig(), "Basic", "Profile");

	auto cb = [&](const char *name, const char *filePath) {
		if (strcmp(oldName.c_str(), name) != 0) {
			newName = name;
			newPath = filePath;
			return false;
		}

		return true;
	};

	EnumProfiles(cb);

	/* this should never be true due to menu item being grayed out */
	if (newPath.empty())
		return;

	if (!skipConfirmation) {
		QString text = QTStr("ConfirmRemove.Text")
				       .arg(QT_UTF8(oldName.c_str()));

		QMessageBox::StandardButton button = OBSMessageBox::question(
			this, QTStr("ConfirmRemove.Title"), text);
		if (button == QMessageBox::No)
			return;
	}

	size_t newPath_len = newPath.size();
	newPath += "/basic.ini";

	if (config.Open(newPath.c_str(), CONFIG_OPEN_ALWAYS) != 0) {
		blog(LOG_ERROR, "ChangeProfile: Failed to load file '%s'",
		     newPath.c_str());
		return;
	}

	if (api)
		api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGING);

	newPath.resize(newPath_len);

	const char *newDir = strrchr(newPath.c_str(), '/') + 1;

	config_set_string(App()->GlobalConfig(), "Basic", "Profile",
			  newName.c_str());
	config_set_string(App()->GlobalConfig(), "Basic", "ProfileDir", newDir);

	QString settingsRequiringRestart;
	bool needsRestart =
		ProfileNeedsRestart(config, settingsRequiringRestart);

	Auth::Save();
	auth.reset();
	DeleteCookies();
	DestroyPanelCookieManager();

	config.Swap(basicConfig);
	InitBasicConfigDefaults();
	InitBasicConfigDefaults2();
	ResetProfileData();
	DeleteProfile(oldName.c_str(), oldDir.c_str());
	RefreshProfiles();
	config_save_safe(App()->GlobalConfig(), "tmp", nullptr);

	blog(LOG_INFO, "Switched to profile '%s' (%s)", newName.c_str(),
	     newDir);
	blog(LOG_INFO, "------------------------------------------------");

	UpdateTitleBar();
	UpdateVolumeControlsDecayRate();

	Auth::Load();

	if (api) {
		api->on_event(OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED);
		api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGED);
	}

	if (needsRestart) {
		QMessageBox::StandardButton button = OBSMessageBox::question(
			this, QTStr("Restart"),
			QTStr("LoadProfileNeedsRestart")
				.arg(settingsRequiringRestart));

		if (button == QMessageBox::Yes) {
			restart = true;
			close();
		}
	}
}

void OBSBasic::on_actionImportProfile_triggered()
{
	char path[512];

	QString home = QDir::homePath();

	int ret = GetConfigPath(path, 512,
		(std::string(CONFIG_DIR) + "/basic/profiles/").c_str());
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get profile config path");
		return;
	}

	QString dir = SelectDirectory(
		this, QTStr("Basic.MainMenu.Profile.Import"), home);

	if (!dir.isEmpty() && !dir.isNull()) {
		QString inputPath = QString::fromUtf8(path);
		QFileInfo finfo(dir);
		QString directory = finfo.fileName();
		QString profileDir = inputPath + directory;

		if (ProfileExists(directory.toStdString().c_str())) {
			OBSMessageBox::warning(
				this, QTStr("Basic.MainMenu.Profile.Import"),
				QTStr("Basic.MainMenu.Profile.Exists"));
		} else if (os_mkdir(profileDir.toStdString().c_str()) < 0) {
			blog(LOG_WARNING,
			     "Failed to create profile directory '%s'",
			     directory.toStdString().c_str());
		} else {
			QFile::copy(dir + "/basic.ini",
				    profileDir + "/basic.ini");
			QFile::copy(dir + "/service.json",
				    profileDir + "/service.json");
			QFile::copy(dir + "/streamEncoder.json",
				    profileDir + "/streamEncoder.json");
			QFile::copy(dir + "/recordEncoder.json",
				    profileDir + "/recordEncoder.json");
			RefreshProfiles();
		}
	}
}

void OBSBasic::on_actionExportProfile_triggered()
{
	char path[512];

	QString home = QDir::homePath();

	QString currentProfile = QString::fromUtf8(config_get_string(
		App()->GlobalConfig(), "Basic", "ProfileDir"));

	int ret = GetConfigPath(path, 512,
		(std::string(CONFIG_DIR) + "/basic/profiles/").c_str());
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get profile config path");
		return;
	}

	QString dir = SelectDirectory(
		this, QTStr("Basic.MainMenu.Profile.Export"), home);

	if (!dir.isEmpty() && !dir.isNull()) {
		QString outputDir = dir + "/" + currentProfile;
		QString inputPath = QString::fromUtf8(path);
		QDir folder(outputDir);

		if (!folder.exists()) {
			folder.mkpath(outputDir);
		} else {
			if (QFile::exists(outputDir + "/basic.ini"))
				QFile::remove(outputDir + "/basic.ini");

			if (QFile::exists(outputDir + "/service.json"))
				QFile::remove(outputDir + "/service.json");

			if (QFile::exists(outputDir + "/streamEncoder.json"))
				QFile::remove(outputDir +
					      "/streamEncoder.json");

			if (QFile::exists(outputDir + "/recordEncoder.json"))
				QFile::remove(outputDir +
					      "/recordEncoder.json");
		}

		QFile::copy(inputPath + currentProfile + "/basic.ini",
			    outputDir + "/basic.ini");
		QFile::copy(inputPath + currentProfile + "/service.json",
			    outputDir + "/service.json");
		QFile::copy(inputPath + currentProfile + "/streamEncoder.json",
			    outputDir + "/streamEncoder.json");
		QFile::copy(inputPath + currentProfile + "/recordEncoder.json",
			    outputDir + "/recordEncoder.json");
	}
}

void OBSBasic::ChangeProfile()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	ConfigFile config;
	std::string path;

	if (!action)
		return;

	path = QT_TO_UTF8(action->property("file_name").value<QString>());
	if (path.empty())
		return;

	const char *oldName =
		config_get_string(App()->GlobalConfig(), "Basic", "Profile");
	if (action->text().compare(QT_UTF8(oldName)) == 0) {
		action->setChecked(true);
		return;
	}

	size_t path_len = path.size();
	path += "/basic.ini";

	if (config.Open(path.c_str(), CONFIG_OPEN_ALWAYS) != 0) {
		blog(LOG_ERROR, "ChangeProfile: Failed to load file '%s'",
		     path.c_str());
		return;
	}

	if (api)
		api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGING);

	path.resize(path_len);

	const char *newName = config_get_string(config, "General", "Name");
	const char *newDir = strrchr(path.c_str(), '/') + 1;

	QString settingsRequiringRestart;
	bool needsRestart =
		ProfileNeedsRestart(config, settingsRequiringRestart);

	config_set_string(App()->GlobalConfig(), "Basic", "Profile", newName);
	config_set_string(App()->GlobalConfig(), "Basic", "ProfileDir", newDir);

	Auth::Save();
	auth.reset();
	DestroyPanelCookieManager();

	config.Swap(basicConfig);
	InitBasicConfigDefaults();
	InitBasicConfigDefaults2();
	ResetProfileData();
	RefreshProfiles();
	config_save_safe(App()->GlobalConfig(), "tmp", nullptr);
	UpdateTitleBar();
	UpdateVolumeControlsDecayRate();

	Auth::Load();

	CheckForSimpleModeX264Fallback();

	blog(LOG_INFO, "Switched to profile '%s' (%s)", newName, newDir);
	blog(LOG_INFO, "------------------------------------------------");

	if (api)
		api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGED);

	if (needsRestart) {
		QMessageBox::StandardButton button = OBSMessageBox::question(
			this, QTStr("Restart"),
			QTStr("LoadProfileNeedsRestart")
				.arg(settingsRequiringRestart));

		if (button == QMessageBox::Yes) {
			restart = true;
			close();
		}
	}
}

void OBSBasic::CheckForSimpleModeX264Fallback()
{
	const char *curStreamEncoder =
		config_get_string(basicConfig, "SimpleOutput", "StreamEncoder");
	const char *curRecEncoder =
		config_get_string(basicConfig, "SimpleOutput", "RecEncoder");
	bool qsv_supported = false;
	bool qsv_av1_supported = false;
	bool amd_supported = false;
	bool nve_supported = false;
#ifdef ENABLE_HEVC
	bool amd_hevc_supported = false;
	bool nve_hevc_supported = false;
	bool apple_hevc_supported = false;
#endif
	bool amd_av1_supported = false;
	bool apple_supported = false;
	bool changed = false;
	size_t idx = 0;
	const char *id;

	while (obs_enum_encoder_types(idx++, &id)) {
		if (strcmp(id, "amd_amf_h264") == 0)
			amd_supported = true;
		else if (strcmp(id, "obs_qsv11") == 0)
			qsv_supported = true;
		else if (strcmp(id, "obs_qsv11_av1") == 0)
			qsv_av1_supported = true;
		else if (strcmp(id, "ffmpeg_nvenc") == 0)
			nve_supported = true;
#ifdef ENABLE_HEVC
		else if (strcmp(id, "h265_texture_amf") == 0)
			amd_hevc_supported = true;
		else if (strcmp(id, "ffmpeg_hevc_nvenc") == 0)
			nve_hevc_supported = true;
#endif
		else if (strcmp(id, "av1_texture_amf") == 0)
			amd_av1_supported = true;
		else if (strcmp(id,
				"com.apple.videotoolbox.videoencoder.ave.avc") ==
			 0)
			apple_supported = true;
#ifdef ENABLE_HEVC
		else if (strcmp(id,
				"com.apple.videotoolbox.videoencoder.ave.hevc") ==
			 0)
			apple_hevc_supported = true;
#endif
	}

	auto CheckEncoder = [&](const char *&name) {
		if (strcmp(name, SIMPLE_ENCODER_QSV) == 0) {
			if (!qsv_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
		} else if (strcmp(name, SIMPLE_ENCODER_QSV_AV1) == 0) {
			if (!qsv_av1_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
		} else if (strcmp(name, SIMPLE_ENCODER_NVENC) == 0) {
			if (!nve_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
		} else if (strcmp(name, SIMPLE_ENCODER_NVENC_AV1) == 0) {
			if (!nve_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
#ifdef ENABLE_HEVC
		} else if (strcmp(name, SIMPLE_ENCODER_AMD_HEVC) == 0) {
			if (!amd_hevc_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
		} else if (strcmp(name, SIMPLE_ENCODER_NVENC_HEVC) == 0) {
			if (!nve_hevc_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
#endif
		} else if (strcmp(name, SIMPLE_ENCODER_AMD) == 0) {
			if (!amd_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
		} else if (strcmp(name, SIMPLE_ENCODER_AMD_AV1) == 0) {
			if (!amd_av1_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
		} else if (strcmp(name, SIMPLE_ENCODER_APPLE_H264) == 0) {
			if (!apple_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
#ifdef ENABLE_HEVC
		} else if (strcmp(name, SIMPLE_ENCODER_APPLE_HEVC) == 0) {
			if (!apple_hevc_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
#endif
		}

		return true;
	};

	if (!CheckEncoder(curStreamEncoder))
		config_set_string(basicConfig, "SimpleOutput", "StreamEncoder",
				  curStreamEncoder);
	if (!CheckEncoder(curRecEncoder))
		config_set_string(basicConfig, "SimpleOutput", "RecEncoder",
				  curRecEncoder);
	if (changed)
		config_save_safe(basicConfig, "tmp", nullptr);
}
