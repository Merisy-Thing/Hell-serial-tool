#include "hell_serial.h"
#include "ui_hell_serial.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QRegExp>
#include <QDate>
#include <QDesktopServices>
#include <QUrl>
#include <QScrollBar>

#define MAX_FILE_SIZE       512*1024

#define MAX_CUSTOM_CMD_NUM          64
#define CUSTOM_CMD_FILE_NAME        "custom_cmd_list.ini"

hell_serial::hell_serial(QWidget *parent) :
    QDialog(parent), ui(new Ui::hell_serial)
{
    ui->setupUi(this);

    ui_init();
    hex_edit_init();

    m_qserial_port = new QSerialPort(this);

    is_ascii_mode = false;
    ui->pte_out_ascii_mode->setVisible(false);
    this->setMaximumWidth(600);
    this->setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
    this->show();

    connect(m_qserial_port, SIGNAL(readyRead()), this, SLOT(readSerialData()));

    connect(&m_hex_send_autorepeat_timer, SIGNAL(timeout ()),
            this,       SLOT(hex_send()));
    connect(&m_ascii_send_autorepeat_timer, SIGNAL(timeout ()),
            this,       SLOT(ascii_send()));

    this->setWindowTitle(QString("Hell Serial Tool - ") + __DATE__ + "  (By: Hell Prototypes)");
}

hell_serial::~hell_serial()
{
    if(record_file.isOpen()) {
       record_file.close();
    }

    if(m_qserial_port->isOpen()) {
        m_qserial_port->close();
    }

    if(m_setting_file != NULL) {
        m_setting_file->beginGroup("Cmd_List");
        QString cmd;
        int index = 0;
        for(int i=0; i<ui->cb_custom_cmd_list->count(); i++) {
            if(index >= MAX_CUSTOM_CMD_NUM) {
                break;
            }
            cmd = ui->cb_custom_cmd_list->itemText(i);
            if(cmd.isEmpty()) {
                continue;
            }
            m_setting_file->setValue("Cmd_"+QString::number(index++), cmd);
        }
        m_setting_file->endGroup();
        m_setting_file->sync();
    }

    delete ui;
}

void hell_serial::ui_init()
{
    QString string;
    QStringList string_list;

    //Port name list
    string = "COM%1";
    for(int i=1; i<29; i++) {
        ui->cb_port_name->insertItem(i,string.arg(i));
    }
    ui->cb_port_name->setMaxVisibleItems(15);
    ui->cb_port_name->setCurrentIndex(0);

    //Baud Rate Type
    string_list <<  "2400"   <<  "4800"  <<  "9600"  <<  "14400" <<  "19200"
                <<  "38400"  <<  "56000" <<  "57600" <<  "76800" <<  "115200"
                <<  "128000" <<  "256000";
    ui->cb_baudrate->insertItems(0, string_list);

    ui->cb_baudrate->setMaxVisibleItems(12);
    ui->cb_baudrate->setCurrentIndex(9);

    //Data bits
    string = "%1";
    for(int i=5; i<=8; i++) {
        ui->cb_data_bits->insertItem(i-5, string.arg(i));
        //m_data_bits_map[string.arg(i)] = i;
    }
    ui->cb_data_bits->setCurrentIndex(3);

    //Parity Type
    string_list.clear();
    string_list << tr("NONE") << tr("EVEN") << tr("ODD") << tr("SPACE") << tr("MARK");
    int parity_list[] = {QSerialPort::NoParity,
                         QSerialPort::EvenParity,
                         QSerialPort::OddParity,
                         QSerialPort::SpaceParity,
                         QSerialPort::MarkParity };
    for(int i=0; i<string_list.size(); i++) {
        ui->cb_parity->insertItem(i, string_list.at(i));
        m_parity_map[string_list.at(i)] = parity_list[i];
    }
    ui->cb_parity->setMaxVisibleItems(5);
    ui->cb_parity->setCurrentIndex(0);

    //Stop bits
    string_list.clear();
    string_list << tr("1") << tr("1.5") << tr("2");
    int stopBits_list[] = {QSerialPort::OneStop,
                           QSerialPort::OneAndHalfStop,
                           QSerialPort::TwoStop};
    for(int i=0; i<string_list.size(); i++) {
        ui->cb_stop_bits->insertItem(i, string_list.at(i));
        m_stop_bits_map[string_list.at(i)] = stopBits_list[i];
    }

    ui->cb_stop_bits->setMaxVisibleItems(3);
    ui->cb_stop_bits->setCurrentIndex(0);

    ui->pte_out_ascii_mode->setStyleSheet(QString::fromUtf8("background-color:Black; color:green"));
    //buffer_len
    string = "%1";
    for(int i=1; i<= 128; i++) {
        ui->cb_buffer_len->insertItem(i-1, string.arg(i*512));
    }
    ui->cb_buffer_len->setCurrentIndex(1);
    buffer_len = 1024;
    ui->pte_out_ascii_mode->setMaximumBlockCount(buffer_len);

    ui->pb_save_raw_data->setEnabled(false);
    ui->pb_send_file->setEnabled(false);
    ui->pb_record_raw_data->setEnabled(false);

    QFile setting_file(CUSTOM_CMD_FILE_NAME);
    if(setting_file.exists()) {
        m_setting_file = new QSettings(CUSTOM_CMD_FILE_NAME, QSettings::IniFormat);
        m_setting_file->beginGroup("Cmd_List");
        QString cmd;
        for(int i=0; i<MAX_CUSTOM_CMD_NUM; i++) {
            cmd = m_setting_file->value("Cmd_"+QString::number(i)).toString();
            if(cmd.isEmpty()) {
                break;
            }
            m_custom_cmd_string_list.push_back(cmd);
        }
        m_setting_file->endGroup();
    } else {
        if(setting_file.open(QIODevice::WriteOnly)) {
            setting_file.close();
            m_setting_file = new QSettings(CUSTOM_CMD_FILE_NAME, QSettings::IniFormat);
        } else {
            m_setting_file = NULL;
        }
        m_custom_cmd_string_list.clear();
    }
    if(m_custom_cmd_string_list.size() > 0) {
        ui->cb_custom_cmd_list->addItems(m_custom_cmd_string_list);
    }

    ui->cb_custom_cmd_list->setMaxVisibleItems(MAX_CUSTOM_CMD_NUM);
}

void hell_serial::hex_edit_init()
{
    m_hex_edit = new QHexEdit;
    m_hex_edit->setFocusPolicy(Qt::NoFocus);
    ui->horizontalLayout->addWidget(m_hex_edit);

    m_hex_edit->setReadOnly(true);
    m_hex_edit->setAddressArea(false);
}



void hell_serial::on_pb_about_clicked()
{
    QMessageBox::about(this, tr("About "),
                       tr("A Simple Serial Tool"
                          "\r\nCreat by: pajoke"
                          "\r\nE_Mail: pajoke@163.com"
                          ));
}

void hell_serial::stop_autorepeat()
{
    if(m_hex_send_autorepeat_timer.isActive()) {
        m_hex_send_autorepeat_timer.stop();
        ui->pb_hex_send->setChecked(false);
    }
    if(m_ascii_send_autorepeat_timer.isActive()) {
        m_ascii_send_autorepeat_timer.stop();
        ui->pb_ascii_send->setChecked(false);
    }
}
void hell_serial::on_pb_port_ctrl_clicked()
{
    if(m_qserial_port->isOpen()) {
        //close
        stop_autorepeat();
        m_qserial_port->close();
        ui->pb_port_ctrl->setText(tr("Open"));

        if(record_file.isOpen()) {
           record_file.close();
        }

        if(m_hex_edit->dataSize() > 0) {
            ui->pb_save_raw_data->setEnabled(true);
        }
        ui->pb_send_file->setEnabled(false);
        ui->pb_record_raw_data->setText(tr("Record"));
        ui->pb_record_raw_data->setEnabled(false);
        ui->gb_port_setting->setEnabled(true);
    } else {
        //open
        m_qserial_port->setPortName(ui->cb_port_name->currentText());
        if(m_qserial_port->open(QIODevice::ReadWrite)) {
            m_qserial_port->setBaudRate(ui->cb_baudrate->currentText().toInt());
            m_qserial_port->setDataBits((QSerialPort::DataBits)ui->cb_data_bits->currentText().toInt());
            m_qserial_port->setParity((QSerialPort::Parity)m_parity_map[ui->cb_parity->currentText()]);
            m_qserial_port->setStopBits((QSerialPort::StopBits)m_stop_bits_map[ui->cb_stop_bits->currentText()]);
            m_qserial_port->setFlowControl(QSerialPort::NoFlowControl);

            m_qserial_port->flush();

            ui->pb_port_ctrl->setText(tr("Close"));
            ui->pb_save_raw_data->setEnabled(false);
            ui->pb_send_file->setEnabled(true);
            ui->pb_record_raw_data->setEnabled(true);
            ui->gb_port_setting->setEnabled(false);
        } else {
            //qDebug("open fail");
        }
    }
}

void hell_serial::readSerialData()
{
    QByteArray data = m_qserial_port->readAll();

    if(is_ascii_mode) {
        QTextCursor cursor = ui->pte_out_ascii_mode->textCursor();
        cursor.movePosition(QTextCursor::End);
        ui->pte_out_ascii_mode->setTextCursor(cursor);
        ui->pte_out_ascii_mode->insertPlainText(QString(data));
        ui->pte_out_ascii_mode->verticalScrollBar()->setValue(ui->pte_out_ascii_mode->verticalScrollBar()->maximum());
    }
    m_hex_edit->insert(m_hex_edit->dataSize(), data);
    if(m_hex_edit->dataSize() > buffer_len) {
        int num = ((m_hex_edit->dataSize() - buffer_len)/16)*16;
        if(num > 0) {
            m_hex_edit->remove(0, num);
        }
    }
    m_hex_edit->scrollToBottom();

    if(record_file.isOpen()) {
        record_file.write(data);
    }

    //qDebug("ascii num=%d", ui->pte_out_ascii_mode->blockCount());

    ui->lb_dbg->setText(QString("%1").arg(m_hex_edit->dataSize()));
}

void hell_serial::keyPressEvent ( QKeyEvent * event )
{
    QByteArray c;

    if(m_qserial_port->isOpen()) {
        switch (event->key()) {
        case Qt::Key_Space:
            //qDebug("Key_Space");
            c.push_back(' ');
            break;
            break;
        default :
            QByteArray key_code = event->text().toLatin1();
            if(key_code.at(0) == '\r') {
                if(ui->cbx_line_feed_mode->isChecked()) {
                    c.push_back('\r');
                    c.push_back('\n');
                } else {
                    c.push_back('\r');
                }
            } else {
                c.push_back(key_code);
            }
            break ;
        }
        //qDebug("%s",c.data());
        m_qserial_port->write(c);
    }
}

void hell_serial::on_pb_clear_clicked()
{
    m_hex_edit->remove(0, m_hex_edit->dataSize());

    ui->pb_save_raw_data->setEnabled(false);

    ui->pte_out_ascii_mode->clear();
    ui->lb_dbg->setText("0");
}

void hell_serial::on_cb_buffer_len_currentIndexChanged(const QString &len)
{
    //qDebug("%s", arg1.toAscii().data());
    buffer_len = len.toInt();
    ui->pte_out_ascii_mode->setMaximumBlockCount(len.toInt());
}

void hell_serial::on_pb_save_raw_data_clicked()
{
    QByteArray raw_data;
    if(is_ascii_mode) {
        raw_data = ui->pte_out_ascii_mode->toPlainText().toLatin1();
        if(raw_data.size() == 0) {
            return;
        }
    } else {
        if(m_hex_edit->dataSize() == 0) {
            return;
        }
        raw_data = m_hex_edit->data();
    }

    QString output_file = QFileDialog::getSaveFileName(this, tr("save file"),
                                               "R:\\", tr("All (*.*)"));
    if(output_file.isEmpty() == true) {
        return;
    }

    QFile file(output_file);
    if(!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Warning"), tr("Open file error!"), QMessageBox::Yes );
        return;
    }
    file.write(raw_data);
    file.close();
}

void hell_serial::on_pb_send_file_clicked()
{
    QString input_file = QFileDialog::getOpenFileName(this, "send file",
                                    "", tr("All (*.*)"));
    QFile file(input_file);
    if(file.size() > MAX_FILE_SIZE ){
        if(QMessageBox::question(this, tr("Warning"), tr("file size greater than 512KB continue?"),
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::No){
            return;
        }
    }
    if(!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Warning"), tr("Open file error!"), QMessageBox::Yes );
        return;
    }
    QByteArray file_data = file.readAll();
    file.close();

    ui->pb_port_ctrl->setEnabled(false);

    m_qserial_port->write(file_data);

    ui->pb_port_ctrl->setEnabled(true);
}

void hell_serial::on_pb_record_raw_data_clicked()
{
    if(!record_file.isOpen()) {
        //record_file
        QString f = QFileDialog::getSaveFileName(this, tr("save file"),
                                                   "R:\\", tr("All (*.*)"));
        if(f.isEmpty() == true) {
            return;
        }
        record_file.setFileName(f);
        if(!record_file.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, tr("Warning"), tr("Open file error!"), QMessageBox::Yes );
            return;
        }
        ui->pb_record_raw_data->setText(tr("Stop"));
    } else {
        record_file.close();
        ui->pb_record_raw_data->setText(tr("Record"));
    }
}

void hell_serial::on_pb_always_visible_clicked()
{
    if(this->windowFlags() & Qt::WindowStaysOnTopHint) {
        //hide();
        setWindowFlags(this->windowFlags() & (~Qt::WindowStaysOnTopHint));
        show();
        ui->pb_always_visible->setText(tr("Always visible"));
    } else {
        //hide();
        setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);
        show();
        ui->pb_always_visible->setText(tr("unAlways"));
    }
}

void hell_serial::add_custom_cmd_to_list(QString cmd)
{
    if(!m_custom_cmd_string_list.contains(cmd)) {
        m_custom_cmd_string_list.push_back(cmd);
        ui->cb_custom_cmd_list->insertItem(0, cmd);
    } else {
        int index = ui->cb_custom_cmd_list->findText(cmd);
        if(index >=0 ) {
            ui->cb_custom_cmd_list->removeItem(index);
            ui->cb_custom_cmd_list->insertItem(0,cmd);
            ui->cb_custom_cmd_list->setCurrentIndex(0);
        } else {
            qDebug("can't found text = %s", cmd.toLatin1().data());
        }
    }
}

int hell_serial::get_sampling_time(QString time_str)
{
    int num = 0, factor = 0;
    bool ok = false;

    if(time_str.right(2) == "ms"){
        num = time_str.left(time_str.length()-2).toInt(&ok);
        factor = 1;//ms base
        if(!ok) {
            ui->cb_autorepeat_interval->setCurrentIndex(2);
            return 100;
        }
    } else if(time_str.right(1) == "S"){
        num = time_str.left(time_str.length()-1).toInt(&ok);
        factor = 1000;//S base
        if(!ok) {
            ui->cb_autorepeat_interval->setCurrentIndex(2);
            return 100;
        }
    } else {
        ui->cb_autorepeat_interval->setCurrentIndex(2);
        return 100;
    }

    if(ok) {
        //qDebug("get_sampling_time = %d", num * factor);
        return num * factor;
    } else {
        //ui->cb_autorepeat_interval->setCurrentIndex(2);
        qDebug("get_sampling_time faile 100 default");
        return 100;
    }
}
void hell_serial::on_pb_hex_send_clicked(bool checked)
{
    if(!m_qserial_port->isOpen()) {
        QMessageBox::warning(this, tr("Warning"), tr("Port not open!"), QMessageBox::Yes );
        if(ui->pb_hex_send->isCheckable()) {
            ui->pb_hex_send->setChecked(false);
        }
        return;
    }

    if(ui->pb_hex_send->isCheckable()) {
        if(checked){
            if(m_ascii_send_autorepeat_timer.isActive()) {
                ui->pb_hex_send->setChecked(false);
            } else {
                m_hex_send_autorepeat_timer.setInterval(get_sampling_time(ui->cb_autorepeat_interval->currentText()));
                m_hex_send_autorepeat_timer.start();
            }
        } else {
            m_hex_send_autorepeat_timer.stop();
        }
    } else {
        hex_send();
    }
}
void hell_serial::hex_send()
{
    QString cmd = ui->cb_custom_cmd_list->currentText();
    if(cmd.isEmpty()) {
        return;
    }
    QString cmd_backup = cmd;

    //qDebug("cmd = %s", cmd.toAscii().data());

    //eat comment
    if(cmd.contains("//")) {
        cmd = cmd.left(cmd.indexOf("//"));
    }
    if(cmd.isEmpty()) {
        return;
    }

    //remove space char
    cmd.remove(QChar(' '), Qt::CaseSensitive);
    if(cmd.isEmpty()) {
        return;
    }

    //odd number
    if(cmd.size() & 1) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid data length, [Odd] length!"), QMessageBox::Yes );
        return;
    }

    if(cmd.contains(QRegExp("[^0-9,A-F,a-f]"))) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid data format, 0-9,A-F,a-f chars need!"), QMessageBox::Yes );
        return;
    }

    QByteArray raw_data;
    for(int i=0; i<cmd.size(); i+=2) {
        bool ok;
        char hex = (char)cmd.mid(i, 2).toUInt(&ok, 16);
        if(ok) {
            raw_data.push_back(hex);
        }
    }

    if(m_qserial_port->isOpen()) {
        add_custom_cmd_to_list(cmd_backup);
        m_qserial_port->write(raw_data);
    }

    //qDebug("cmd = %s", cmd.toAscii().data());
}


void hell_serial::on_pb_ascii_send_clicked(bool checked)
{
    if(!m_qserial_port->isOpen()) {
        QMessageBox::warning(this, tr("Warning"), tr("Port not open!"), QMessageBox::Yes );
        if(ui->pb_ascii_send->isCheckable()) {
            ui->pb_ascii_send->setChecked(false);
        }
        return;
    }

    if(ui->pb_ascii_send->isCheckable()) {
        if(checked){
            if(m_hex_send_autorepeat_timer.isActive()) {
                ui->pb_ascii_send->setChecked(false);
            } else {
                m_ascii_send_autorepeat_timer.setInterval(get_sampling_time(ui->cb_autorepeat_interval->currentText()));
                m_ascii_send_autorepeat_timer.start();
            }
        } else {
            m_ascii_send_autorepeat_timer.stop();
        }
    } else {
        ascii_send();
    }
}
void hell_serial::ascii_send()
{
    QString cmd = ui->cb_custom_cmd_list->currentText();
    if(cmd.isEmpty()) {
        return;
    }

    if(m_qserial_port->isOpen()) {
        add_custom_cmd_to_list(cmd);
        m_qserial_port->write(cmd.toLatin1());
    }
}

void hell_serial::on_pb_mode_switch_clicked()
{
    if(is_ascii_mode) {
        is_ascii_mode = false;
        ui->pb_mode_switch->setText("ASCII Mode");
        m_hex_edit->setVisible(true);
        ui->pte_out_ascii_mode->setVisible(false);
        ui->pte_out_ascii_mode->clear();

        this->setMaximumWidth(600);
        this->setWindowFlags(windowFlags() & (~Qt::WindowMaximizeButtonHint));
    } else {
        is_ascii_mode = true;
        ui->pb_mode_switch->setText("HEX Mode");
        m_hex_edit->setVisible(false);
        ui->pte_out_ascii_mode->setPlainText(QString(m_hex_edit->data()));
        ui->pte_out_ascii_mode->verticalScrollBar()->setValue(ui->pte_out_ascii_mode->verticalScrollBar()->maximum());
        ui->pte_out_ascii_mode->setVisible(true);
        this->setMaximumWidth(QWIDGETSIZE_MAX);
        this->setWindowFlags(this->windowFlags() | Qt::WindowMaximizeButtonHint);
    }
    this->show();
}

void hell_serial::on_chb_AutoRepeat_clicked(bool checked)
{
    if(checked) {
        ui->pb_hex_send->setCheckable(true);
        ui->pb_ascii_send->setCheckable(true);
    } else {
        ui->pb_hex_send->setCheckable(false);
        ui->pb_ascii_send->setCheckable(false);
        stop_autorepeat();
    }
}


void hell_serial::on_pb_home_page_clicked()
{
    QDesktopServices::openUrl(QUrl("http://www.hellprototypes.com/", QUrl::TolerantMode));
}
