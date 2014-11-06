#include "send_thread.h"

send_thread::send_thread(QextSerialPort &adrPort) : port(adrPort)
{
    send_byte_count = 0;
}

send_thread::~send_thread()
{
    if (isRunning()) {
        wait();
    }
}

bool send_thread::send_data(const QByteArray &send_data)
{
    if(!port.isOpen()) {
        return false;
    }
    send_data_buffer.clear();
    send_data_buffer=send_data;
    if(send_data_buffer.size() > 0) {
        this->start();
    }
    return true;
}

// Thread Receive Loop
void send_thread::run()
{
    const char *p = send_data_buffer.constData();
    for(int i=0; i<send_data_buffer.size(); i++) {
        port.write(p++, 1);

        dataSended(++send_byte_count);
    }
}
