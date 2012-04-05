#include "hell_serial.h"
#include "ui_hell_serial.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QRegExp>

#define POLL_INTERVAL       200  /*Unit: ms*/
#define TIME_OUT            10   /*Unit: ms*/
#define MAX_FILE_SIZE       64*1024

#define SELECT_FILTED               0
#define SELECT_CHANGED_FROM_HEX     1
#define SELECT_CHANGED_FROM_ASCII   2

#define MAX_CUSTOM_CMD_NUM          64
#define CUSTOM_CMD_FILE_NAME        "custom_cmd_list.ini"

hell_serial::hell_serial(QWidget *parent) :
    QDialog(parent), ui(new Ui::hell_serial)
{
    ui->setupUi(this);

    ui_init();

    m_serial_port.setDtr(false);
    m_serial_port.setRts(false);

    line_feed = 0;
    select_changed = SELECT_FILTED;

    is_ascii_mode = false;
    ui->pte_out_ascii_mode->setVisible(false);
    this->setMaximumWidth(600);
    this->setWindowFlags(Qt::WindowMinimizeButtonHint);
    this->show();

    rec_thread = new receive_thread(m_serial_port);
    connect(rec_thread, SIGNAL(dataReceived(const QByteArray &)),
            this,       SLOT(dataReceived(const QByteArray &)));
}

hell_serial::~hell_serial()
{
    if(rec_thread->isRunning()) {
        rec_thread->stopReceiving();
        rec_thread->wait();
    }
    if(m_serial_port.isOpen()) {
        m_serial_port.close();
    }

    if(record_file.isOpen()) {
       record_file.close();
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
    int index;

    //Port name list
    string = "COM%1";
    for(int i=1; i<29; i++) {
        ui->cb_port_name->insertItem(i,string.arg(i));
    }
    ui->cb_port_name->setMaxVisibleItems(15);
    ui->cb_port_name->setCurrentIndex(0);

    //Baud Rate Type
    string_list <<  "50"     <<  "75"    <<  "110"   <<  "134"   <<  "150"
                <<  "200"    <<  "300"   <<  "600"   <<  "1200"  <<  "1800"
                <<  "2400"   <<  "4800"  <<  "9600"  <<  "14400" <<  "19200"
                <<  "38400"  <<  "56000" <<  "57600" <<  "76800" <<  "115200"
                <<  "128000" <<  "256000";
    index = BAUD50;
    for(int i=0; i<string_list.size(); i++) {
        ui->cb_baudrate->insertItem(i, string_list.at(i));
        m_baudrate_map[string_list.at(i)] = index;
        index++;
    }
    ui->cb_baudrate->setMaxVisibleItems(15);
    ui->cb_baudrate->setCurrentIndex(19);

    //Data bits
    string = "%1";
    for(int i=5; i<=8; i++) {
        ui->cb_data_bits->insertItem(i-5, string.arg(i));
        //m_data_bits_map[string.arg(i)] = i;
    }
    ui->cb_data_bits->setCurrentIndex(3);

    //Parity Type
    string_list.clear();
    string_list << tr("NONE") << tr("ODD") << tr("EVEN") << tr("SPACE");
    index = PAR_NONE;
    for(int i=0; i<string_list.size(); i++) {
        ui->cb_parity->insertItem(i, string_list.at(i));
        m_parity_map[string_list.at(i)] = index;
        index++;
    }
    ui->cb_parity->setMaxVisibleItems(4);
    ui->cb_parity->setCurrentIndex(0);

    //Stop bits
    string_list.clear();
    string_list << tr("1") << tr("1.5") << tr("2");
    index = STOP_1;
    for(int i=0; i<string_list.size(); i++) {
        ui->cb_stop_bits->insertItem(i, string_list.at(i));
        m_stop_bits_map[string_list.at(i)] = index;
        index++;
    }
    ui->cb_stop_bits->setMaxVisibleItems(3);
    ui->cb_stop_bits->setCurrentIndex(0);

    ui->pte_out_hex->setUndoRedoEnabled(false);
    ui->pte_out_ascii->setUndoRedoEnabled(false);

    ui->pte_out_hex->setStyleSheet(QString::fromUtf8("background-color:Black; color:green"));
    ui->pte_out_ascii->setStyleSheet(QString::fromUtf8("background-color:Black; color:red"));
    ui->pte_out_ascii_mode->setStyleSheet(QString::fromUtf8("background-color:Black; color:green"));
    //buffer_len
    string = "%1";
    for(int i=512; i<= 4096*4; i+=1024) {
        ui->cb_buffer_len->insertItem(i/512-1, string.arg(i));
    }
    ui->cb_buffer_len->setCurrentIndex(0);
    buffer_len = 512;
    ui->pte_out_ascii_mode->setMaximumBlockCount(512);

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

void hell_serial::on_pb_about_clicked()
{
    QMessageBox::about(this, tr("About "),
                       tr("A Simple Serial Tool (Qt 4.7.3)"
                          "\r\nCreat by: pajoke"
                          "\r\nE_Mail: pajoke@163.com"
                          "\r\n======================"
                          "\r\nCode base on \"QExtSerialPort\" Class"
                          ));
}

void hell_serial::on_pb_port_ctrl_clicked()
{
    if(m_serial_port.isOpen()) {
        //close
        rec_thread->stopReceiving();
        rec_thread->wait();

        m_serial_port.close();
        ui->pb_port_ctrl->setText(tr("Open"));

        if(record_file.isOpen()) {
           record_file.close();
        }

        if(receive_buffer_raw.size() > 0) {
            ui->pb_save_raw_data->setEnabled(true);
        }
        ui->pb_send_file->setEnabled(false);
        ui->pb_record_raw_data->setText(tr("Record"));
        ui->pb_record_raw_data->setEnabled(false);
        ui->gb_port_setting->setEnabled(true);
    } else {
        //open
        m_serial_port.setPortName(ui->cb_port_name->currentText());
        if(m_serial_port.open(QIODevice::ReadWrite)) {
            m_serial_port.setBaudRate((BaudRateType)m_baudrate_map[ui->cb_baudrate->currentText()]);
            m_serial_port.setDataBits((DataBitsType)ui->cb_data_bits->currentIndex());
            m_serial_port.setParity((ParityType)m_parity_map[ui->cb_parity->currentText()]);
            m_serial_port.setStopBits((StopBitsType)m_stop_bits_map[ui->cb_stop_bits->currentText()]);
            m_serial_port.setFlowControl(FLOW_OFF);
            m_serial_port.setTimeout(TIME_OUT);

            m_serial_port.flush();
            rec_thread->start();

            ui->pb_port_ctrl->setText(tr("Close"));
            ui->pb_save_raw_data->setEnabled(false);
            ui->pb_send_file->setEnabled(true);
            ui->pb_record_raw_data->setEnabled(true);
            ui->gb_port_setting->setEnabled(false);
        } else {
            qDebug("open fail");
        }
        //poll_timer->start(POLL_INTERVAL);
    }
}

void hell_serial::dataReceived(const QByteArray &dataReceived)
{
    const char bin2hex[] = "0123456789ABCDEF";
    uchar ch;

    QTextCursor cursor;

    cursor = ui->pte_out_ascii_mode->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->pte_out_ascii_mode->setTextCursor(cursor);
    ui->pte_out_ascii_mode->insertPlainText(QString(dataReceived));

    for(int i=0; i<dataReceived.size(); i++) {
        ch = dataReceived[i];

        if(ch >= ' ' && ch <= '~') {
            receive_buffer_ascii += ch;
        } else {
            receive_buffer_ascii += "?";
        }

        receive_buffer_hex += QString(bin2hex[ch>>4])+ QString(bin2hex[ch&0x0F]) + (" ");
        if(line_feed++ == 15) {
            line_feed = 0;
            receive_buffer_hex += "\r\n";
            receive_buffer_ascii += "\r\n";
        }
    }

    receive_buffer_raw += dataReceived;
    if(receive_buffer_raw.size() > buffer_len) {
        receive_buffer_raw.remove(0, receive_buffer_raw.size()%buffer_len);
    }

    if(record_file.isOpen()) {
        record_file.write(dataReceived);
    }

    if(receive_buffer_ascii.size()> buffer_len) {
        receive_buffer_hex.remove(0, 48+2);//16B*3 + \r\n
        receive_buffer_ascii.remove(0, 16+2);//16B + \r\n
    }
    ui->pte_out_hex->setPlainText(receive_buffer_hex);

    cursor = ui->pte_out_hex->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->pte_out_hex->setTextCursor(cursor);

    cursor = ui->pte_out_ascii->textCursor();
    ui->pte_out_ascii->setPlainText(receive_buffer_ascii);
    cursor.movePosition(QTextCursor::End);
    ui->pte_out_ascii->setTextCursor(cursor);

    ui->lb_dbg->setText(QString("%1").arg(receive_buffer_ascii.size()));

}

void hell_serial::keyPressEvent ( QKeyEvent * event )
{
    QByteArray c;

    if(m_serial_port.isOpen()) {
        switch (event->key()) {
        case Qt::Key_Space:
            qDebug("Key_Space");
            c.push_back(' ');
            break;
            break;
        default :
            QByteArray key_code = event->text().toAscii();
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
            //qDebug("%s",event->text().toAscii().data());
            break ;
        }
        m_serial_port.write(c);
    }
}

void hell_serial::on_pb_clear_clicked()
{
    receive_buffer_raw.clear();
    receive_buffer_hex.clear();
    receive_buffer_ascii.clear();
    line_feed = 0;
    ui->pb_save_raw_data->setEnabled(false);

    ui->pte_out_hex->clear();
    ui->pte_out_ascii->clear();
    ui->pte_out_ascii_mode->clear();
}

void hell_serial::on_cb_buffer_len_currentIndexChanged(const QString &len)
{
    //qDebug("%s", arg1.toAscii().data());
    buffer_len = len.toInt();
    ui->pte_out_ascii_mode->setMaximumBlockCount(len.toInt());
}

void hell_serial::on_pb_save_raw_data_clicked()
{
    if(receive_buffer_raw.size() == 0) {
        return;
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
    file.write(receive_buffer_raw);
    file.close();
}

void hell_serial::on_pb_send_file_clicked()
{
    QString input_file = QFileDialog::getOpenFileName(this, "send file",
                                    "", tr("All (*.*)"));
    QFile file(input_file);
    if(file.size() > MAX_FILE_SIZE ){
        if(QMessageBox::question(this, tr("Warning"), tr("file size greater than 64KB continue?"),
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
    rec_thread->stopReceiving();
    rec_thread->wait();

    m_serial_port.write(file_data);//Send data

    m_serial_port.flush();
    rec_thread->start();
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

void hell_serial::on_pte_out_hex_selectionChanged()
{
    if(select_changed == SELECT_CHANGED_FROM_ASCII || select_changed == SELECT_CHANGED_FROM_HEX) {
        select_changed = SELECT_FILTED;
        return;
    } else {
        select_changed = SELECT_CHANGED_FROM_HEX;
    }

    QTextCursor cursor_hex = ui->pte_out_hex->textCursor();
    QTextCursor cursor_ascii = ui->pte_out_ascii->textCursor();

    int hex_start, ascii_start, end, hex_len, ascii_len, section, start_section;

    hex_start = cursor_hex.selectionStart();
    end   = cursor_hex.selectionEnd();
    hex_len = end - hex_start + 2;
    start_section = hex_start/49;
    section  = hex_len/49;

    ascii_start = (hex_start - start_section)/3 + start_section;
    ascii_len = (hex_len - section)/3 + section;

    if(ascii_len == 0) {
        ascii_len = 1;
    }

    //qDebug("start = %d, end = %d", hex_start, end);

    cursor_ascii.setPosition(ascii_start);
    cursor_ascii.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, ascii_len);
    ui->pte_out_ascii->setTextCursor(cursor_ascii);

    if(hex_len == 2) {
        hex_start = hex_start - (hex_start - start_section)%3;
        cursor_hex.setPosition(hex_start);
        cursor_hex.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
        ui->pte_out_hex->setTextCursor(cursor_hex);
        select_changed = SELECT_CHANGED_FROM_HEX;
    }

}

void hell_serial::on_pte_out_ascii_selectionChanged()
{
    if(select_changed == SELECT_CHANGED_FROM_ASCII || select_changed == SELECT_CHANGED_FROM_HEX) {
        select_changed = SELECT_FILTED;
        return;
    } else {
        select_changed = SELECT_CHANGED_FROM_ASCII;
    }
    QTextCursor cursor_hex = ui->pte_out_hex->textCursor();
    QTextCursor cursor_ascii = ui->pte_out_ascii->textCursor();

    int hex_start, ascii_start, end, hex_len, ascii_len, section, start_section;

    ascii_start = cursor_ascii.selectionStart();
    end = cursor_ascii.selectionEnd();
    ascii_len = end - ascii_start;
    start_section = ascii_start/17;
    section = ascii_len/17;

    hex_start = (ascii_start - start_section)*3 + start_section;
    hex_len = (ascii_len - section)*3 + section;

    //qDebug("start = %d, end = %d", ascii_start, end);

    cursor_hex.setPosition(hex_start);
    cursor_hex.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, hex_len);
    ui->pte_out_hex->setTextCursor(cursor_hex);

    if(ascii_len == 0) {
        cursor_ascii.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
        ui->pte_out_ascii->setTextCursor(cursor_ascii);
        select_changed = SELECT_CHANGED_FROM_ASCII;
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
            qDebug("can't found text = %s", cmd.toAscii().data());
        }
    }
}

void hell_serial::on_pb_hex_send_clicked()
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

    if(m_serial_port.isOpen()) {
        add_custom_cmd_to_list(cmd_backup);
        m_serial_port.write(raw_data);
    }

    //qDebug("cmd = %s", cmd.toAscii().data());
}

void hell_serial::on_pb_ascii_send_clicked()
{
    QString cmd = ui->cb_custom_cmd_list->currentText();
    if(cmd.isEmpty()) {
        return;
    }

    if(m_serial_port.isOpen()) {
        add_custom_cmd_to_list(cmd);
        m_serial_port.write(cmd.toAscii().data());
    }
}

void hell_serial::on_pb_mode_switch_clicked()
{
    if(is_ascii_mode) {
        is_ascii_mode = false;
        ui->pb_mode_switch->setText("ASCII Mode");

        ui->pte_out_ascii->setVisible(true);
        ui->pte_out_hex->setVisible(true);
        ui->pte_out_ascii_mode->setVisible(false);

        this->setMaximumWidth(600);
        this->setWindowFlags(this->windowFlags() & (~Qt::WindowMaximizeButtonHint));
        this->show();
    } else {
        is_ascii_mode = true;
        ui->pb_mode_switch->setText("HEX Mode");

        ui->pte_out_ascii->setVisible(false);
        ui->pte_out_hex->setVisible(false);
        ui->pte_out_ascii_mode->setVisible(true);
        this->setMaximumWidth(QWIDGETSIZE_MAX);
        this->setWindowFlags(this->windowFlags() | Qt::WindowMaximizeButtonHint);
        this->show();
    }
    //this->windowFlags() & (~Qt::WindowStaysOnTopHint)
}
