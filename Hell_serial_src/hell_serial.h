#ifndef HELL_SERIAL_H
#define HELL_SERIAL_H

#include <QDialog>
#include <QMap>
#include <QTimer>

#include "qextserialport.h"
#include "receive_thread.h"

namespace Ui {
    class hell_serial;
}

class hell_serial : public QDialog
{
    Q_OBJECT

public:
    explicit hell_serial(QWidget *parent = 0);
    ~hell_serial();


private slots:
    void dataReceived(const QByteArray &dataReceived);

    void on_pb_port_ctrl_clicked();
    void on_pb_clear_clicked();
    void on_cb_buffer_len_currentIndexChanged(const QString &arg1);
    void on_pb_save_raw_data_clicked();
    void on_pb_send_file_clicked();
    void on_pb_record_raw_data_clicked();
    void on_pte_out_hex_selectionChanged();
    void on_pte_out_ascii_selectionChanged();

private:
    Ui::hell_serial *ui;

    void ui_init();

    QextSerialPort  m_serial_port;
    receive_thread *rec_thread;
    QString receive_buffer_hex;
    QString receive_buffer_ascii;
    QByteArray receive_buffer_raw;
    int line_feed;
    int buffer_len;
    int select_changed;

    QFile record_file;

    QMap<QString, int> m_baudrate_map;
    QMap<QString, int> m_data_bits_map;
    QMap<QString, int> m_parity_map;
    QMap<QString, int> m_stop_bits_map;

protected:
    void keyPressEvent ( QKeyEvent * event );
};

#endif // HELL_SERIAL_H
