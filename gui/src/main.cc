#include <QApplication>
#include "mainwin.h"

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	MainWin w;
	w.show();

	return app.exec();
}
