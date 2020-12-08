#include "window-basic-about.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"
#include "remote-text.hpp"
#include <util/util.hpp>
#include <util/platform.h>
#include <platform.hpp>
#include <json11.hpp>

using namespace json11;

OBSAbout::OBSAbout(QWidget *parent) : QDialog(parent), ui(new Ui::OBSAbout)
{
	ui->setupUi(this);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	QString bitness;
	QString ver;

	if (sizeof(void *) == 4)
		bitness = " (32 bit)";
	else if (sizeof(void *) == 8)
		bitness = " (64 bit)";

#ifdef HAVE_OBSCONFIG_H
	ver += OBS_VERSION;
#else
	ver += LIBOBS_API_MAJOR_VER + "." + LIBOBS_API_MINOR_VER + "." +
	       LIBOBS_API_PATCH_VER;
#endif

	ui->version->setText(ver + bitness);

	ui->about->setText("<a href='#'>" + QTStr("About") + "</a>");
	ui->authors->setText("<a href='#'>" + QTStr("About.Authors") + "</a>");
	ui->license->setText("<a href='#'>" + QTStr("About.License") + "</a>");

	ui->name->setProperty("themeID", "aboutName");
	ui->version->setProperty("themeID", "aboutVersion");
	ui->about->setProperty("themeID", "aboutHLayout");
	ui->authors->setProperty("themeID", "aboutHLayout");
	ui->license->setProperty("themeID", "aboutHLayout");

	connect(ui->about, SIGNAL(clicked()), this, SLOT(ShowAbout()));
	connect(ui->authors, SIGNAL(clicked()), this, SLOT(ShowAuthors()));
	connect(ui->license, SIGNAL(clicked()), this, SLOT(ShowLicense()));

	QPointer<OBSAbout> about(this);

	OBSBasic *main = OBSBasic::Get();
	if (main->patronJson.empty() && !main->patronJsonThread) {
		RemoteTextThread *thread = new RemoteTextThread(
			"https://obsproject.com/patreon/about-box.json",
			"application/json");
		QObject::connect(thread, &RemoteTextThread::Result, main,
				 &OBSBasic::UpdatePatronJson);
		QObject::connect(
			thread,
			SIGNAL(Result(const QString &, const QString &)), this,
			SLOT(ShowAbout()));
		main->patronJsonThread.reset(thread);
		thread->start();
	} else {
		ShowAbout();
	}
}

void OBSAbout::ShowAbout()
{
	OBSBasic *main = OBSBasic::Get();
	QString text;
	text += "<h3>Remote Filming Studio is for live single and multi camera film shoot streaming</h3>";
	ui->textBrowser->setHtml(text);
}

void OBSAbout::ShowAuthors()
{
	std::string path;
	QString error = "Error! File could not be read.\n\n \
		Go to: https://github.com/obsproject/obs-studio/blob/master/AUTHORS";

	if (!GetDataFilePath("authors/AUTHORS", path)) {
		ui->textBrowser->setPlainText(error);
		return;
	}

	ui->textBrowser->setPlainText(QString::fromStdString(path));

	BPtr<char> text = os_quick_read_utf8_file(path.c_str());

	if (!text || !*text) {
		ui->textBrowser->setPlainText(error);
		return;
	}

	ui->textBrowser->setPlainText(QT_UTF8(text));
}

void OBSAbout::ShowLicense()
{
	std::string path;
	QString error = "Error! File could not be read.\n\n \
		Go to: https://github.com/obsproject/obs-studio/blob/master/COPYING";

	if (!GetDataFilePath("license/gplv2.txt", path)) {
		ui->textBrowser->setPlainText(error);
		return;
	}

	BPtr<char> text = os_quick_read_utf8_file(path.c_str());

	if (!text || !*text) {
		ui->textBrowser->setPlainText(error);
		return;
	}

	ui->textBrowser->setPlainText(QT_UTF8(text));
}
