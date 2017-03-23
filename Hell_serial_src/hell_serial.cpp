#include "hell_serial.h"
#include "ui_hell_serial.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegExp>
#include <QDate>
#include <QDesktopServices>
#include <QUrl>
#include <QScrollBar>
#include <QToolTip>

#ifdef Q_OS_WIN32
#include <WTypes.h>
#include <dbt.h>
#include <Windows.h>
#endif

#define MAX_FILE_SIZE               512*1024

#define MAX_CUSTOM_CMD_NUM          64
#define CUSTOM_CMD_FILE_NAME        "Hell_Serial.ini"

#define WG_MAXIMUM_WIDTH            720

hell_serial::hell_serial(QWidget *parent) :
    QDialog(parent), ui(new Ui::hell_serial)
{
    ui->setupUi(this);

    ui_init();
    hex_edit_init();

    m_qserial_port = new QSerialPort(this);

    is_ascii_mode = false;
    ui->pte_out_ascii_mode->setVisible(false);
    this->setMaximumWidth(WG_MAXIMUM_WIDTH);
    this->setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
    this->show();

    connect(m_qserial_port, SIGNAL(readyRead()), this, SLOT(readSerialData()));
    connect(m_LuaPlugin->m_lua_bind, SIGNAL(SerialPortWrite(QByteArray)), this, SLOT(plugin_SerialPortWrite(QByteArray)));

    connect(&m_hex_send_autorepeat_timer, SIGNAL(timeout ()),
            this,       SLOT(autorepeat_hex_send()));
    connect(&m_ascii_send_autorepeat_timer, SIGNAL(timeout ()),
            this,       SLOT(autorepeat_ascii_send()));

    this->setWindowTitle(QString("Hell Serial Tool - ") + __DATE__ + "  (By: Hell Prototypes)");
    ui->pb_about->setVisible(false);
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

        if(m_custom_cmd_item_list.size() > 0) {
            m_setting_file->beginGroup("Box_List");
            QString cmd;
            int index = 0;
            for(int i=0; i<m_custom_cmd_item_list.size(); i++) {
                if(index >= MAX_CUSTOM_CMD_NUM) {
                    break;
                }
                cmd = m_custom_cmd_item_list.at(i)->get_cmd_text();
                if(cmd.isEmpty()) {
                    continue;
                }
                m_setting_file->setValue("Box_"+QString::number(index++), cmd);
            }
            m_setting_file->endGroup();
        }
        m_setting_file->beginGroup("Misc");
        m_setting_file->setValue("LastScriptFile", m_last_script);
        m_setting_file->setValue("LastRecordFile", m_record_file_name);
        m_setting_file->setValue("LastPortName", m_last_port_name);
        m_setting_file->setValue("LastDlgSize", m_last_dlg_size);
        m_setting_file->endGroup();

        m_setting_file->sync();
    }

    delete m_LuaPlugin;
    delete ui;
}

void hell_serial::ui_init()
{
    QString string;
    QStringList string_list;

    //Baud Rate Type
    string_list <<  "2400"   <<  "4800"  <<  "9600"  <<  "14400" <<  "19200"
                <<  "38400"  <<  "56000" <<  "57600" <<  "76800" <<  "115200"
                <<  "128000" <<  "256000" << "CUSTOM";
    ui->cb_baudrate->insertItems(0, string_list);

    ui->cb_baudrate->setMaxVisibleItems(ui->cb_baudrate->count());
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
    ui->cb_data_bits->setVisible(false);

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

    ui->pb_send_file->setEnabled(false);
    ui->pb_record_raw_data->setEnabled(false);

    ui->dw_cmd_tools->setVisible(false);
    ui->dw_cmd_tools->setFloating(true);
    ui->dw_cmd_tools->layout()->setSpacing(0);
    ui->dw_cmd_tools->layout()->setMargin(0);

    connect(ui->dw_cmd_tools, SIGNAL(topLevelChanged(bool)), this, SLOT(cmdToolBoxTopLevelChanged(bool)));


    ui->cb_custom_cmd_list->setMaxVisibleItems(MAX_CUSTOM_CMD_NUM);
    //ui->cb_custom_cmd_list->view()->setFixedWidth(580);
    //ui->cb_addon_module_list->view()->setFixedWidth(196);

    read_setting_file();

    ui->cb_port_name->setMaxVisibleItems(15);
    ui->cb_port_name->setCurrentIndex(0);

    m_LuaPlugin = new LuaPlugin(m_setting_file, this);
    m_LuaPlugin->setVisible(false);
    m_LuaPlugin->setModal(false);

    detect_serial_port(false);
}

void hell_serial::read_setting_file()
{

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

        m_setting_file->beginGroup("Box_List");
        for(int i=0; i<MAX_CUSTOM_CMD_NUM; i++) {
            cmd = m_setting_file->value("Box_"+QString::number(i)).toString();
            if(cmd.isEmpty()) {
                break;
            }
            new_custom_cmd_item(i, cmd);
        }
        m_setting_file->endGroup();

        m_setting_file->beginGroup("Misc");
        m_last_script = m_setting_file->value("LastScriptFile").toString();
        m_record_file_name = m_setting_file->value("LastRecordFile").toString();
        m_last_port_name = m_setting_file->value("LastPortName").toString();
        m_last_dlg_size = m_setting_file->value("LastDlgSize").toSize();
        if(!m_last_dlg_size.isValid()) {
            m_last_dlg_size = QSize(this->width(), this->height());
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
}
void hell_serial::hex_edit_init()
{
    m_hex_edit = new QHexEdit;
    m_hex_edit->setFocusPolicy(Qt::NoFocus);
    ui->horizontalLayout->insertWidget(0, m_hex_edit);

    m_hex_edit->setReadOnly(true);
    m_hex_edit->setAddressArea(false);
    m_hex_edit->setHighlighting(false);
    m_hex_edit->setHexCaps(true);
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

        ui->pb_send_file->setEnabled(false);
        ui->pb_record_raw_data->setText(tr("Record"));
        ui->pb_record_raw_data->setEnabled(false);
        ui->gb_port_setting->setEnabled(true);
    } else {
        //open
        m_qserial_port->setPortName(ui->cb_port_name->currentText());
        bool ok = false;
        int baudrate = ui->cb_baudrate->currentText().toInt(&ok);
        if(ok && m_qserial_port->open(QIODevice::ReadWrite)) {
            m_qserial_port->setBaudRate(baudrate);
            m_qserial_port->setDataBits((QSerialPort::DataBits)ui->cb_data_bits->currentText().toInt());
            m_qserial_port->setParity((QSerialPort::Parity)m_parity_map[ui->cb_parity->currentText()]);
            m_qserial_port->setStopBits((QSerialPort::StopBits)m_stop_bits_map[ui->cb_stop_bits->currentText()]);
            m_qserial_port->setFlowControl(QSerialPort::NoFlowControl);

            m_qserial_port->flush();

            ui->pb_port_ctrl->setText(tr("Close"));
            ui->pb_send_file->setEnabled(true);
            ui->pb_record_raw_data->setEnabled(true);
            ui->gb_port_setting->setEnabled(false);
            m_last_port_name = ui->cb_port_name->currentText();
        } else {
            QPoint pos = QWidget::mapToGlobal(ui->pb_port_ctrl->pos());
            //pos.setX(pos.x() + ui->pb_port_ctrl->width() - 16);
            pos.setY(pos.y() + 42);
            QToolTip::showText(pos, QString("Port open fail: ") + (ok ? "Port Busy" : "Baudrate Error"), ui->pb_port_ctrl);
        }
    }
}

void hell_serial::readSerialData()
{
    QByteArray data = m_qserial_port->readAll();

    if(m_LuaPlugin->m_lua_bind->isRunning()) {
        qDebug("Plugin read data len:%d", data.size());
        m_LuaPlugin->m_lua_bind->lua_bind_receive_serial_data(data);
    }

    if(is_ascii_mode) {
        //qDebug("is_ascii_mode");
        QTextCursor cursor = ui->pte_out_ascii_mode->textCursor();
        cursor.movePosition(QTextCursor::End);
        ui->pte_out_ascii_mode->setTextCursor(cursor);
        ui->pte_out_ascii_mode->insertPlainText(QString(data));
        ui->pte_out_ascii_mode->verticalScrollBar()->setValue(ui->pte_out_ascii_mode->verticalScrollBar()->maximum());
    }
    //m_hex_edit->insert(m_hex_edit->dataSize(), data);
    m_hex_edit->appendData(data);
    m_hex_edit->scrollToBottom();

    if(record_file.isOpen()) {
        record_file.write(data);
    }

    //qDebug("ascii num=%d", ui->pte_out_ascii_mode->blockCount());

    ui->lb_dbg->setText(QString("%1").arg(m_hex_edit->getDataSize()));
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

void hell_serial::moveEvent(QMoveEvent * e)
{
    //qDebug("moveEvent x=%d y=%d", event->pos().x(), event->pos().y());
    if(ui->dw_cmd_tools->isVisible()) {
        ui->dw_cmd_tools->move( e->pos().x() + this->width()+8, e->pos().y());
    }
    if(m_LuaPlugin->isVisible()) {
        if((m_LuaPlugin->pos().x() < e->oldPos().x() + this->width())
                && (m_LuaPlugin->pos().y() < e->oldPos().y() + this->height()) ) {
            m_LuaPlugin->move(m_LuaPlugin->pos().x() + e->pos().x() - e->oldPos().x(),
                              m_LuaPlugin->pos().y() + e->pos().y() - e->oldPos().y() );
        } else {
            m_LuaPlugin->move(e->pos().x() + this->width()+8, e->pos().y() - 30);
        }
    }
}
void hell_serial::resizeEvent(QResizeEvent *event)
{
    if(is_ascii_mode) {
        m_last_dlg_size = event->size();
    }
}

void hell_serial::on_pb_clear_clicked()
{
    QByteArray empty;

    m_hex_edit->setData(empty);

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
        if(m_hex_edit->getDataSize() == 0) {
            return;
        }
        raw_data = m_hex_edit->data();
    }

    QString output_file = QFileDialog::getSaveFileName(this, tr("save file"),
                                               m_record_file_name, tr("All (*.*)"));
    if(output_file.isEmpty() == true) {
        return;
    }
    m_record_file_name = output_file;

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
                                                   m_record_file_name, tr("All (*.*)"));
        if(f.isEmpty() == true) {
            return;
        }
        m_record_file_name = f;
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
    if(!ui->pb_script_send->isEnabled()) {
        return;
    }

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
            //qDebug("can't found text = %s", cmd.toLatin1().data());
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
            ui->cb_autorepeat_interval->setCurrentIndex(3);
            return 100;
        }
    } else if(time_str.right(1) == "S"){
        num = time_str.left(time_str.length()-1).toInt(&ok);
        factor = 1000;//S base
        if(!ok) {
            ui->cb_autorepeat_interval->setCurrentIndex(3);
            return 100;
        }
    } else {
        ui->cb_autorepeat_interval->setCurrentIndex(3);
        return 100;
    }

    if(ok) {
        //qDebug("get_sampling_time = %d", num * factor);
        return num * factor;
    } else {
        //ui->cb_autorepeat_interval->setCurrentIndex(2);
        //qDebug("get_sampling_time faile 100 default");
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
        hex_send(ui->cb_custom_cmd_list->currentText());
    }
}
bool hell_serial::hex_send(const QString &_cmd)
{
    QString cmd = _cmd;
    if(cmd.isEmpty()) {
        return false;
    }
    QString cmd_backup = cmd;

    //qDebug("cmd = %s", cmd.toAscii().data());

    //eat comment
    if(cmd.contains("//")) {
        cmd = cmd.left(cmd.indexOf("//"));
    }
    if(cmd.isEmpty()) {
        return false;
    }

    //remove space char
    cmd.remove(QChar(' '), Qt::CaseSensitive);
    if(cmd.isEmpty()) {
        return false;
    }

    //odd number
    if(cmd.size() & 1) {
        //qDebug("Odd:%s", cmd.toLatin1().data());
        QMessageBox::warning(this, tr("Error"), tr("Invalid data length, [Odd] length!"), QMessageBox::Yes );
        return false;
    }

    //qDebug("%d, %s", cmd.size(), cmd.toLatin1().data());
    if(cmd.contains(QRegExp("[^0-9,A-F,a-f]"))) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid data format, 0-9,A-F,a-f chars need!"), QMessageBox::Yes );
        return false;
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
        return true;
    }

    //qDebug("cmd = %s", cmd.toAscii().data());
    return false;
}

void hell_serial::plugin_SerialPortWrite(QByteArray wr_data)
{
    if(m_qserial_port->isOpen()) {
        m_qserial_port->write(wr_data);
    }
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
        ascii_send(ui->cb_custom_cmd_list->currentText());
    }
}
void hell_serial::ascii_send(const QString &_cmd)
{
    QString cmd = _cmd;
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

        this->setMaximumWidth(WG_MAXIMUM_WIDTH);
        this->setWindowFlags(windowFlags() & (~Qt::WindowMaximizeButtonHint));
    } else {
        is_ascii_mode = true;
        ui->pb_mode_switch->setText("HEX Mode");
        m_hex_edit->setVisible(false);
        ui->pte_out_ascii_mode->setPlainText(QString(m_hex_edit->data()));
        ui->pte_out_ascii_mode->verticalScrollBar()->setValue(ui->pte_out_ascii_mode->verticalScrollBar()->maximum());
        ui->pte_out_ascii_mode->setVisible(true);

        this->setMaximumWidth(QWIDGETSIZE_MAX);
        int iTitleBarHeight = style()->pixelMetric(QStyle::PM_TitleBarHeight);  // 获取标题栏高度
        this->setGeometry(x(), y()+iTitleBarHeight, m_last_dlg_size.width(), m_last_dlg_size.height());

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
        ui->pb_hex_send->setChecked(false);
        ui->pb_hex_send->setCheckable(false);
        ui->pb_ascii_send->setCheckable(false);
        stop_autorepeat();
    }
}


void hell_serial::on_pb_home_page_clicked()
{
    QDesktopServices::openUrl(QUrl("http://www.hellprototypes.com/", QUrl::TolerantMode));
}

void hell_serial::new_custom_cmd_item(int id, const QString &command)
{
    custom_cmd_item *cmd_item;

    cmd_item = new custom_cmd_item(id);
    cmd_item->set_cmd_text(command);
    m_custom_cmd_item_list.push_back(cmd_item);
    ui->vl_cmd_tools_list->addWidget(cmd_item);
    connect(cmd_item, SIGNAL(send_custom_cmd(bool, QString)), this, SLOT(cmdToolBox_send_custom_cmd(bool, QString)));
    connect(cmd_item, SIGNAL(remove_custom_cmd(int)), this, SLOT(cmdToolBox_remove_custom_cmd(int)));

    resize_custom_cmd_tools_box();
}

void hell_serial::on_pb_make_cmd_list_clicked()
{
    if(!ui->dw_cmd_tools->isVisible()) {
        if(m_custom_cmd_item_list.isEmpty()) {
            new_custom_cmd_item(0, ui->cb_custom_cmd_list->currentText());
        }
        ui->dw_cmd_tools->setVisible(true);
        //qDebug("ui->dw_cmd_tools->width()=%d", ui->dw_cmd_tools->width());

        QPoint pos = QWidget::mapToGlobal(QPoint(0,0));
        ui->dw_cmd_tools->move( pos.x() + this->width()+8, pos.y());
    } else {
        new_custom_cmd_item(m_custom_cmd_item_list.size(), ui->cb_custom_cmd_list->currentText());
    }
}

void hell_serial::cmdToolBox_send_custom_cmd(bool is_hex_send, QString cmd)
{
    if(is_hex_send) {
        hex_send(cmd);
    } else {
        ascii_send(cmd);
    }
}

void hell_serial::autorepeat_hex_send()
{
    if(!hex_send(ui->cb_custom_cmd_list->currentText())) {
        m_hex_send_autorepeat_timer.stop();
        ui->pb_hex_send->setChecked(false);
    }
}
void hell_serial::autorepeat_ascii_send()
{
    ascii_send(ui->cb_custom_cmd_list->currentText());
}

void hell_serial::cmdToolBox_remove_custom_cmd(int id)
{
    if(id < m_custom_cmd_item_list.size()) {
        custom_cmd_item *cmd_item = m_custom_cmd_item_list.at(id);
        delete cmd_item;
        m_custom_cmd_item_list.removeAt(id);

        for(int i=0; i<m_custom_cmd_item_list.size(); i++) {
            m_custom_cmd_item_list.at(i)->set_cmd_id(i);
        }
        resize_custom_cmd_tools_box();
    }
}

void hell_serial::resize_custom_cmd_tools_box()
{
    QRect rect = ui->dw_cmd_tools->geometry();
    rect.setHeight(m_custom_cmd_item_list.size()*26+24);
    ui->dw_cmd_tools->setGeometry(rect);
}

void hell_serial::cmdToolBoxTopLevelChanged(bool top)
{
    if(!top) {
        ui->dw_cmd_tools->setFloating(true);
        resize_custom_cmd_tools_box();
    }
}

void hell_serial::on_cb_autorepeat_interval_currentIndexChanged(const QString &arg1)
{
    m_hex_send_autorepeat_timer.setInterval(get_sampling_time(arg1));
}

//msleep xxx
bool hell_serial::sleep_process(QString &line_data)
{
    if(line_data.toLower().left(6) == "msleep") {
        bool ok;
        line_data.remove(0, 6);
        line_data.remove(" ");
        int delay_time = line_data.toInt(&ok);
        if(ok && (delay_time > 0)) {
            ui->pb_script_send->setText("Sleeping");
            QTime dieTime = QTime::currentTime().addMSecs(delay_time);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
            }
            ui->pb_script_send->setText("Sending");
        } else {
            QMessageBox::warning(this, tr("Warning"),
                                 tr("Error delay param :" ) + line_data, QMessageBox::Yes );
        }
        return true;
    }
    return false;
}

//loop x
//...
//end
bool hell_serial::loop_process(QString &line_data, QFile &file)
{
    if(line_data.toLower().left(4) != "loop") {
        return false;
    }
    line_data.remove(0, 4);
    line_data.remove(" ");

    QStringList line_list;

    bool ok;
    int loop_cnt = line_data.toInt(&ok);
    //qDebug("loop_cnt:%d", loop_cnt);
    if(ok && (loop_cnt > 0)) {
        //qDebug("start loop");
        while(1) {
            if(file.atEnd()) {
                break;
            }
            QString line = QString(file.readLine());
            line.remove(" ");
            if(line.toLower().left(3) != "end") {
                line.remove("\n");
                line.remove("\r");
                if(!line.isEmpty()) {
                    line_list.append(line);
                }
            } else {
                while(loop_cnt--) {
                    //qDebug(" loop_cnt:%d", loop_cnt);
                    for(int i=0; i<line_list.size(); i++) {
                        QString item = line_list.at(i);
                        qDebug("loop:%s", item.toLatin1().data());
                        if(sleep_process(item)) {
                            ;
                        } else {
                            hex_send(item);
                        }
                    }
                }
                break;
            }
        }
        //qDebug("end loop");
    }
    return true;
}

void hell_serial::on_pb_script_send_clicked(bool checked)
{
    Q_UNUSED(checked);
    if(!m_qserial_port->isOpen()) {
        QMessageBox::warning(this, tr("Warning"), tr("Port not open!"), QMessageBox::Yes );
        return;
    }

    QString input_file;
    QString curr_file = ui->cb_custom_cmd_list->currentText();
    if((curr_file.left(7) == "file://") && (curr_file.right(4) == ".hss")) {
        input_file = curr_file.remove(0, 7);
    } else {
        input_file = QFileDialog::getOpenFileName(this, "Scrip file",
                                        m_last_script, tr("Scrip (*.hss);;All (*.*)"));
    }

    QFile file(input_file);
    if(!file.exists()) {
        return;
    }
    m_last_script = input_file;
    add_custom_cmd_to_list("file://" + input_file);

    if(!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Warning"), tr("Open file error!"), QMessageBox::Yes );
        return;
    }
    QString pb_text = ui->pb_script_send->text();
    ui->pb_script_send->setText("Sending");
    ui->pb_script_send->setEnabled(false);
    while(!file.atEnd()) {
        QString line_data = QString(file.readLine());

        line_data.remove("\n");
        line_data.remove("\r");

        if(sleep_process(line_data)) {
            ;
        } else if(loop_process(line_data, file)) {
            ;
        } else {
            if(!hex_send(line_data)) {
                //break;
            }
        }
    }

    file.close();
    ui->pb_script_send->setText(pb_text);
    ui->pb_script_send->setEnabled(true);
}

void hell_serial::on_pb_plugin_dlg_clicked()
{
    QPoint pos = QWidget::mapToGlobal(QPoint(0,0));
    if(m_LuaPlugin->isVisible()) {
        m_LuaPlugin->setVisible(false);
        int iTitleBarHeight = style()->pixelMetric(QStyle::PM_TitleBarHeight);  // 获取标题栏高度
        this->move( pos.x() + m_LuaPlugin->width()/2, pos.y()-iTitleBarHeight);
    } else {
        int iTitleBarHeight = style()->pixelMetric(QStyle::PM_TitleBarHeight);  // 获取标题栏高度
        int x = pos.x() - m_LuaPlugin->width()/2;
        int delta = 0;
        if(x <0) {
            delta += x;
            x = 0;
        }
        this->move( x, pos.y()-iTitleBarHeight);

        m_LuaPlugin->move(pos.x() + this->width()- m_LuaPlugin->width()/2 + 3 - delta, pos.y()-iTitleBarHeight);
        m_LuaPlugin->setVisible(true);
    }
}

void hell_serial::detect_serial_port(bool port_opened)
{
    QList<QSerialPortInfo> port_info_list = QSerialPortInfo::availablePorts();

    ui->cb_port_name->clear();
    QStringList list;
    for(int i=0; i<port_info_list.size(); i++) {
        QSerialPortInfo info = port_info_list.at(i);
        //qDebug("%s: %s", info.portName().toLatin1().data(),
        //                 info.description().toLatin1().data());
        list.push_back(info.portName());
    }
    list.sort();
    if(!port_opened) {
        if(list.contains(m_last_port_name)) {
            ui->cb_port_name->setCurrentText(m_last_port_name);
            list.move(list.indexOf(m_last_port_name), 0);
        }
    }
    ui->cb_port_name->addItems(list);

}

bool hell_serial::nativeEvent(const QByteArray & eventType, void * message, long * result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);
#ifdef Q_OS_WIN32
    if(((MSG *)message)->message == WM_DEVICECHANGE) {
        if (((MSG *)message)->wParam == DBT_DEVNODES_CHANGED) {
            //qDebug("detect_serial_port");
            bool port_opened = m_qserial_port->isOpen();
            QString port_name = m_qserial_port->portName();
            bool port_removed = true;

            detect_serial_port(port_opened);

            if(port_opened) {
                for(int i=0; i<ui->cb_port_name->count(); i++) {
                    if(ui->cb_port_name->itemText(i) == port_name) {
                        port_removed = false;
                    }
                }
                if(port_removed) {
                    on_pb_port_ctrl_clicked();
                } else {
                    ui->cb_port_name->setCurrentText(port_name);
                }
            }

        }
    }
#endif
    return false;
}

void hell_serial::on_cb_baudrate_activated(const QString &text)
{
    if(text == "CUSTOM") {
        ui->cb_baudrate->setEditable(true);
        ui->cb_baudrate->setCurrentText("");
    } else {
        ui->cb_baudrate->setEditable(false);
    }
}

void hell_serial::on_cb_port_name_currentIndexChanged(const QString &arg1)
{
    if(this->isVisible()) {
        on_pb_port_ctrl_clicked();
    }
}
