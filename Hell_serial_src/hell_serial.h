#ifndef HELL_SERIAL_H
#define HELL_SERIAL_H

#include <QDialog>
#include <QMap>
#include <QTimer>
#include <QMenu>
#include <QSettings>
#include <QList>
#include <QtSerialPort/QtSerialPort>
#include <QSerialPortInfo>
#include "qhexedit2/qhexedit.h"
#include "custom_cmd_item.h"
#include "lua_plugin.h"

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
    void readSerialData();
    bool hex_send(const QString &_cmd);
    void ascii_send(const QString &_cmd);
    void cmdToolBoxTopLevelChanged(bool top);
    void cmdToolBox_send_custom_cmd(bool is_hex_send, QString cmd);
    void cmdToolBox_remove_custom_cmd(int id);
    void autorepeat_hex_send();
    void autorepeat_ascii_send();
    void plugin_SerialPortWrite(QByteArray wr_data);

    void on_pb_port_ctrl_clicked();
    void on_pb_clear_clicked();
    void on_cb_buffer_len_currentIndexChanged(const QString &arg1);
    void on_pb_save_raw_data_clicked();
    void on_pb_send_file_clicked();
    void on_pb_record_raw_data_clicked();
    void on_pb_about_clicked();
    void on_pb_always_visible_clicked();
    void on_pb_mode_switch_clicked();
    void on_chb_AutoRepeat_clicked(bool checked);
    void on_pb_hex_send_clicked(bool checked);
    void on_pb_ascii_send_clicked(bool checked);
    void on_pb_home_page_clicked();
    void on_pb_make_cmd_list_clicked();
    void on_cb_autorepeat_interval_currentIndexChanged(const QString &arg1);
    void on_pb_script_send_clicked(bool checked);
    void on_pb_plugin_dlg_clicked();
    void on_cb_baudrate_activated(const QString &text);

private:
    Ui::hell_serial *ui;

    void ui_init();
    void hex_edit_init();

    QSerialPort *m_qserial_port;

    QHexEdit *m_hex_edit;

    int buffer_len;

    QTimer m_hex_send_autorepeat_timer;
    QTimer m_ascii_send_autorepeat_timer;

    QFile record_file;
    QString m_record_file_name;

    QMap<QString, int> m_baudrate_map;
    //QMap<QString, int> m_data_bits_map;
    QMap<QString, int> m_parity_map;
    QMap<QString, int> m_stop_bits_map;

    QSettings *m_setting_file;
    QStringList m_custom_cmd_string_list;

    bool is_ascii_mode;
    QString m_last_script;

    QList<custom_cmd_item *> m_custom_cmd_item_list;

    LuaPlugin *m_LuaPlugin;

    void add_custom_cmd_to_list(QString cmd);
    int  get_sampling_time(QString time_str);
    void stop_autorepeat();
    void new_custom_cmd_item(int id, const QString &command);
    void resize_custom_cmd_tools_box();
    bool sleep_process(QString &line_data);
    bool loop_process(QString &line_data, QFile &file);
    void detect_serial_port();

protected:
    void keyPressEvent ( QKeyEvent * event );
    void moveEvent(QMoveEvent * e);
    bool nativeEvent(const QByteArray & eventType, void * message, long * result);
};

#endif // HELL_SERIAL_H
