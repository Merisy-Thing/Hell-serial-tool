#include <QtGui/QApplication>
#include "hell_serial.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    hell_serial w;
    w.setWindowFlags(Qt::WindowMinimizeButtonHint);
    w.show();

    return a.exec();
}
