#ifndef CUSTOM_CMD_ITEM_H
#define CUSTOM_CMD_ITEM_H

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QLabel>

class custom_cmd_item : public QWidget
{
    Q_OBJECT
public:
    explicit custom_cmd_item(int id = 0, QWidget *parent = 0);
    ~custom_cmd_item();

    void set_cmd_id(int id);
    void set_cmd_text(const QString &cmd);
    QString get_cmd_text();

protected:
    QPushButton *m_hex_send;
    QPushButton *m_ascii_send;
    QPushButton *m_remove_cmd;
    QLineEdit   *m_cmd_text;

    QHBoxLayout *m_hboxLayout;

signals:
    void send_custom_cmd(bool is_hex_send, QString cmd);
    void remove_custom_cmd(int id);

public slots:
    void on_pb_hex_send_clicked();
    void on_pb_ascii_send_clicked();
    void on_pb_remove_cmd_clicked();
};

#endif // CUSTOM_CMD_ITEM_H
