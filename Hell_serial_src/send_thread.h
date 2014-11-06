#ifndef SEND_THREAD_H
#define SEND_THREAD_H

#include <QObject>
#include "qextserialport.h"

class send_thread : public QThread
{
    Q_OBJECT
public:
    send_thread(QextSerialPort &adrPort);
    ~send_thread();

    bool send_data(const QByteArray &send_data);
protected:
    void run();

signals:
    void dataSended(int count);

private :
    QextSerialPort &port;
    QByteArray send_data_buffer;

    int send_byte_count;
};

#endif // RECEIVE_THREAD_H
