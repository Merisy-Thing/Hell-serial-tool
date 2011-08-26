#include "hell_serial.h"
#include "ui_hell_serial.h"

#include <QFileDialog>
#include <QMessageBox>

#define POLL_INTERVAL       200  /*Unit: ms*/
#define TIME_OUT            10   /*Unit: ms*/
#define MAX_FILE_SIZE       64*1024

#define SELECT_FILTED               0
#define SELECT_CHANGED_FROM_HEX     1
#define SELECT_CHANGED_FROM_ASCII   2

hell_serial::hell_serial(QWidget *parent) :
    QDialog(parent), ui(new Ui::hell_serial)
{
    ui->setupUi(this);

    ui_init();

    m_serial_port.setDtr(false);
    m_serial_port.setRts(false);

    line_feed = 0;
    select_changed = SELECT_FILTED;

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
        m_data_bits_map[string.arg(i)] = i;
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

    //buffer_len
    string = "%1";
    for(int i=512; i<= 4096; i+=512) {
        ui->cb_buffer_len->insertItem(i/512-1, string.arg(i));
    }
    ui->cb_buffer_len->setCurrentIndex(0);
    buffer_len = 512;

    ui->pb_save_raw_data->setEnabled(false);
    ui->pb_send_file->setEnabled(false);
    ui->pb_record_raw_data->setEnabled(false);
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
            m_serial_port.setDataBits((DataBitsType)m_data_bits_map[ui->cb_data_bits->currentText()]);
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

    QTextCursor cursor = ui->pte_out_hex->textCursor();
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
        default :
            c.push_back(event->text().toAscii());
            //qDebug("%s",event->text().toAscii().data());
            break ;
        }
        m_serial_port.write(c,1);
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
}

void hell_serial::on_cb_buffer_len_currentIndexChanged(const QString &len)
{
    //qDebug("%s", arg1.toAscii().data());
    buffer_len = len.toInt();
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
