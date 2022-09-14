#include <QFile>
#include <QTextStream>
#include <QScrollBar>
#include <QFont>
#include <QFontDatabase>
#include <QPushButton>
#include <QCheckBox>
#include <QLayout>
#include <QDesktopServices>
#include <string>

#include "log-viewer.hpp"
#include "qt-wrappers.hpp"

OBSLogViewer::OBSLogViewer(QWidget *parent)
	: QDialog(parent), ui(new Ui::OBSLogViewer)
{
	setWindowFlags(windowFlags() & Qt::WindowMaximizeButtonHint &
		       ~Qt::WindowContextHelpButtonHint);
	setAttribute(Qt::WA_DeleteOnClose);

	ui->setupUi(this);

	bool showLogViewerOnStartup = config_get_bool(
		App()->GlobalConfig(), "LogViewer", "ShowLogStartup");

	ui->showStartup->setChecked(showLogViewerOnStartup);

	const char *geom = config_get_string(App()->GlobalConfig(), "LogViewer",
					     "geometry");

	if (geom != nullptr) {
		QByteArray ba = QByteArray::fromBase64(QByteArray(geom));
		restoreGeometry(ba);
	}

	InitLog();
}

OBSLogViewer::~OBSLogViewer()
{
	config_set_string(App()->GlobalConfig(), "LogViewer", "geometry",
			  saveGeometry().toBase64().constData());
}

void OBSLogViewer::on_showStartup_clicked(bool checked)
{
	config_set_bool(App()->GlobalConfig(), "LogViewer", "ShowLogStartup",
			checked);
}

extern QPointer<OBSLogViewer> obsLogViewer;

void OBSLogViewer::InitLog()
{
	char logDir[512];
	std::string path;

	if (GetConfigPath(logDir, sizeof(logDir), "obs-studio/logs")) {
		path += logDir;
		path += "/";
		path += App()->GetCurrentLog();
	}

	QFile file(QT_UTF8(path.c_str()));

	if (file.open(QIODevice::ReadOnly)) {
		QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
		in.setCodec("UTF-8");
#endif

		QTextDocument *doc = ui->textArea->document();
		QTextCursor cursor(doc);
		cursor.movePosition(QTextCursor::End);
		cursor.beginEditBlock();
		while (!in.atEnd()) {
			QString line = in.readLine();
			cursor.insertText(line);
			cursor.insertBlock();
		}
		cursor.endEditBlock();

		file.close();
	}
	QScrollBar *scroll = ui->textArea->verticalScrollBar();
	scroll->setValue(scroll->maximum());

	obsLogViewer = this;
}

void OBSLogViewer::AddLine(int type, const QString &str)
{
	QString msg = str.toHtmlEscaped();

	switch (type) {
	case LOG_WARNING:
		msg = QString("<font color=\"#c08000\">%1</font>").arg(msg);
		break;
	case LOG_ERROR:
		msg = QString("<font color=\"#c00000\">%1</font>").arg(msg);
		break;
	default:
		msg = QString("<font>%1</font>").arg(msg);
		break;
	}

	QScrollBar *scroll = ui->textArea->verticalScrollBar();
	bool bottomScrolled = scroll->value() >= scroll->maximum() - 10;

	if (bottomScrolled)
		scroll->setValue(scroll->maximum());

	QTextDocument *doc = ui->textArea->document();
	QTextCursor cursor(doc);
	cursor.movePosition(QTextCursor::End);
	cursor.beginEditBlock();
	cursor.insertHtml(msg);
	cursor.insertBlock();
	cursor.endEditBlock();

	if (bottomScrolled)
		scroll->setValue(scroll->maximum());
}

void OBSLogViewer::on_openButton_clicked()
{
	char logDir[512];
	if (GetConfigPath(logDir, sizeof(logDir), "obs-studio/logs") <= 0)
		return;

	const char *log = App()->GetCurrentLog();

	std::string path = logDir;
	path += "/";
	path += log;

	QUrl url = QUrl::fromLocalFile(QT_UTF8(path.c_str()));
	QDesktopServices::openUrl(url);
}
