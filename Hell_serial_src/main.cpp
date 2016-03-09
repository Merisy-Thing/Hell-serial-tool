#include <QApplication>
#include "hell_serial.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    hell_serial w;
    w.show();

    return a.exec();
}
