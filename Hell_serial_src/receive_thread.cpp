#include "receive_thread.h"

receive_thread::receive_thread(QextSerialPort &adrPort) : port(adrPort)
{
    stopped = false;
}

receive_thread::~receive_thread()
{
    if (isRunning()) {
        stopReceiving();
        wait();
    }
}

void receive_thread::stopReceiving()
{
    stopped = true;
}

// Thread Receive Loop
void receive_thread::run()
{
    int numBytes = 0;
    QByteArray data;

    forever {
        if (stopped) {
            stopped = false;
            break;
        }
        numBytes = port.bytesAvailable();
        if (numBytes>0) {
            mutexReceive.lock();
            data = port.read(numBytes);
            mutexReceive.unlock();

            emit dataReceived(data);
        }
    }
}
