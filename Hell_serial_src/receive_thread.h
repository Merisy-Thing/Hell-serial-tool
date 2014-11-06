#ifndef RECEIVE_THREAD_H
#define RECEIVE_THREAD_H

#include <QObject>
#include "qextserialport.h"

class receive_thread : public QThread
{
    Q_OBJECT
public:
    receive_thread(QextSerialPort &adrPort);
    ~receive_thread();

    void stopReceiving();

protected:
    void run();

signals:
    void dataReceived(const QByteArray &dataReceived);

private :
    QextSerialPort &port;
    bool stopped;

};

#endif // RECEIVE_THREAD_H
