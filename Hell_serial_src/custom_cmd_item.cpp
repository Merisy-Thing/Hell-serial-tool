#include "custom_cmd_item.h"

custom_cmd_item::custom_cmd_item(int id, QWidget *parent) : QWidget(parent)
{
    m_remove_cmd = new QPushButton(QString::number(id));
    m_remove_cmd->setMaximumWidth(24);
    m_remove_cmd->setMinimumWidth(24);
    m_cmd_text = new QLineEdit();
    m_cmd_text->setMinimumWidth(256);
    m_hex_send = new QPushButton("Hex");
    m_hex_send->setMaximumWidth(46);
    m_ascii_send = new QPushButton("Ascii");
    m_ascii_send->setMaximumWidth(46);

    connect(m_hex_send,   SIGNAL(clicked()), this, SLOT(on_pb_hex_send_clicked()));
    connect(m_ascii_send, SIGNAL(clicked()), this, SLOT(on_pb_ascii_send_clicked()));
    connect(m_remove_cmd, SIGNAL(clicked()), this, SLOT(on_pb_remove_cmd_clicked()));

    m_hboxLayout = new QHBoxLayout;
    m_hboxLayout->addWidget(m_remove_cmd);
    m_hboxLayout->addWidget(m_cmd_text);
    m_hboxLayout->addWidget(m_hex_send);
    m_hboxLayout->addWidget(m_ascii_send);

    m_hboxLayout->setMargin(0);
    m_hboxLayout->setSpacing(3);
    this->setLayout(m_hboxLayout);
}

custom_cmd_item::~custom_cmd_item()
{

}

void custom_cmd_item::set_cmd_text(const QString &cmd)
{
    m_cmd_text->setText(cmd);
}

void custom_cmd_item::set_cmd_id(int id)
{
    m_remove_cmd->setText(QString::number(id));
}

QString custom_cmd_item::get_cmd_text()
{
    return m_cmd_text->text();
}

void custom_cmd_item::on_pb_hex_send_clicked()
{
    QString cmd = m_cmd_text->text();
    if(!cmd.isEmpty()) {
        emit send_custom_cmd(true, cmd);
    }
}

void custom_cmd_item::on_pb_ascii_send_clicked()
{
    QString cmd = m_cmd_text->text();
    if(!cmd.isEmpty()) {
        emit send_custom_cmd(false, cmd);
    }
}

void custom_cmd_item::on_pb_remove_cmd_clicked()
{
    emit remove_custom_cmd(m_remove_cmd->text().toInt());
}

