#ifndef HELL_SERIAL_H
#define HELL_SERIAL_H

#include <QDialog>
#include <QMap>
#include <QTimer>
#include <QSettings>

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
    void hex_send();
    void ascii_send();

    void on_pb_port_ctrl_clicked();
    void on_pb_clear_clicked();
    void on_cb_buffer_len_currentIndexChanged(const QString &arg1);
    void on_pb_save_raw_data_clicked();
    void on_pb_send_file_clicked();
    void on_pb_record_raw_data_clicked();
    void on_pte_out_hex_selectionChanged();
    void on_pte_out_ascii_selectionChanged();
    void on_pb_about_clicked();
    void on_pb_always_visible_clicked();
    void on_pb_mode_switch_clicked();
    void on_chb_AutoRepeat_clicked(bool checked);
    void on_pb_hex_send_clicked(bool checked);
    void on_pb_ascii_send_clicked(bool checked);

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

    QTimer m_hex_send_autorepeat_timer;
    QTimer m_ascii_send_autorepeat_timer;

    QFile record_file;

    QMap<QString, int> m_baudrate_map;
    //QMap<QString, int> m_data_bits_map;
    QMap<QString, int> m_parity_map;
    QMap<QString, int> m_stop_bits_map;

    QSettings *m_setting_file;
    QStringList m_custom_cmd_string_list;

    bool is_ascii_mode;

    void add_custom_cmd_to_list(QString cmd);
    int  get_sampling_time(QString time_str);

protected:
    void keyPressEvent ( QKeyEvent * event );
};

#endif // HELL_SERIAL_H
