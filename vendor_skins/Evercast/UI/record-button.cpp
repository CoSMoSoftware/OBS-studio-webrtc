/* Copyright Dr. Alex. Gouaillard (2015, 2020) */

#include "record-button.hpp"
#include "window-basic-main.hpp"

void RecordButton::resizeEvent(QResizeEvent *event)
{
	OBSBasic *main = OBSBasic::Get();
	if (!main->pause)
		return;

	QSize newSize = event->size();
	QSize pauseSize = main->pause->size();
  // NOTE LUDO: #165 Remove button recording
	// int height = main->ui->recordButton->size().height();

	// if (pauseSize.height() != height || pauseSize.width() != height) {
	// 	main->pause->setMinimumSize(height, height);
	// 	main->pause->setMaximumSize(height, height);
	// }
}
