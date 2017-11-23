#include "OpenALWave.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	OpenALWave w;
	w.show();
	return a.exec();
}
