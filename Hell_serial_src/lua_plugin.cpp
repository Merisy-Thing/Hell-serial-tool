#include <QFileDialog>
#include <QMessageBox>

#include "lua_plugin.h"
#include "ui_lua_plugin.h"

LuaPlugin::LuaPlugin(QSettings *setting_file, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LuaPlugin)
{
    ui->setupUi(this);
    setWindowFlags(this->windowFlags() | Qt::WindowMinMaxButtonsHint);

    m_LuaHighlighter = new Highlighter(ui->pte_lua_text->document());
    m_lua_bind = new lua_bind(this);

    connect(ui->pte_lua_text, SIGNAL(textChanged()), this, SLOT(luaTextChanged()));
    connect(m_lua_bind, SIGNAL(finished()), this, SLOT(pluginFinished()));
    connect(m_lua_bind, SIGNAL(printMsg(QString)), this, SLOT(pluginPrintMsg(QString)));

    connect(&m_file_watcher, SIGNAL(fileChanged(QString)), this,SLOT(file_watcher_fileChanged(QString)));

    m_luaTextChanged = false;
    ui->pb_save_plugin_file->setEnabled(false);
    ui->cb_plugin_file_list->setMaxVisibleItems(32);
    //ui->cb_plugin_file_list->view()->setFixedWidth(320);

    m_setting_file = setting_file;
    m_setting_file->beginGroup("PluginFileList");
    QString file;
    for(int i=0; i<32; i++) {
        file = m_setting_file->value("File" + QString::number(i)).toString();
        if(file.isEmpty()) {
            break;
        }
        ui->cb_plugin_file_list->addItem(file);
    }
    m_setting_file->endGroup();
}

LuaPlugin::~LuaPlugin()
{
    delete m_LuaHighlighter;
    delete ui;
}

void LuaPlugin::closeEvent ( QCloseEvent * e )
{
    if(ui->pb_run_plugin->isChecked()) {
        e->ignore();
    } else {
        m_setting_file->beginGroup("PluginFileList");
        QString file;
        for(int i=0; (i<32) && (i<ui->cb_plugin_file_list->count()); i++) {
            file = m_setting_file->value("File" + QString::number(i)).toString();
            m_setting_file->setValue("File" + QString::number(i), ui->cb_plugin_file_list->itemText(i));
        }
        m_setting_file->endGroup();
    }
}

void LuaPlugin::resizeEvent( QResizeEvent * e )
{
    //qDebug("resizeEvent");
    QList<int> size_list;
    size_list.push_back(ui->splitter->height()*75/100);
    size_list.push_back(ui->splitter->height()*25/100);
    ui->splitter->setSizes(size_list);
}

void LuaPlugin::keyPressEvent ( QKeyEvent * e )
{
    if(e->key() == Qt::Key_Escape) {
        e->ignore();
    } else {
        if((e->modifiers() & Qt::ControlModifier) && (e->key() == Qt::Key_S)) {
            if(m_luaTextChanged) {
                on_pb_save_plugin_file_clicked();
            }
        }
    }
}

void LuaPlugin::pluginFinished()
{
    ui->pb_run_plugin->setChecked(false);
    ui->pb_run_plugin->setText("Run");
}

void LuaPlugin::pluginPrintMsg(QString msg)
{
    ui->pte_lua_debug_msg->appendPlainText(msg);
}

void LuaPlugin::on_pb_browse_plugin_file_clicked()
{
    QString plugin_file = QFileDialog::getOpenFileName(this, "Plugin file",
                                    m_currPluginFile, tr("Scrip (*.lua);;All (*.*)"));
    if(plugin_file.isEmpty()) {
        return;
    }
    ui->cb_plugin_file_list->insertItem(0, plugin_file);
    ui->cb_plugin_file_list->setCurrentIndex(0);

    on_pb_load_plugin_file_clicked();
}

void LuaPlugin::on_pb_load_plugin_file_clicked()
{
    QFile file(ui->cb_plugin_file_list->currentText());
    if(!file.exists()) {
        return;
    }

    if(!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Warning"), tr("Open file error!"), QMessageBox::Yes );
        return;
    }

    QString lua_str = QString( file.readAll() );

    file.close();

    ui->pte_lua_text->setPlainText(lua_str);
    m_currPluginFile = ui->cb_plugin_file_list->currentText();
    int item_idx = ui->cb_plugin_file_list->findText(m_currPluginFile);
    if(item_idx > 0) {
        ui->cb_plugin_file_list->removeItem(item_idx);
        ui->cb_plugin_file_list->insertItem(0, m_currPluginFile);
        ui->cb_plugin_file_list->setCurrentIndex(0);
    }
    setWindowTitle(m_currPluginFile);

    if(m_file_watcher.files().size()) {
        m_file_watcher.removePaths(m_file_watcher.files());
    }
    m_file_watcher.addPath(m_currPluginFile);
    m_luaTextChanged = false;
}

void LuaPlugin::luaTextChanged()
{
    if(!m_currPluginFile.isEmpty()) {
        m_luaTextChanged = true;
        setWindowTitle("[Changed] " + m_currPluginFile);
        ui->pb_save_plugin_file->setEnabled(true);
    }
}

void LuaPlugin::file_watcher_fileChanged(QString path)
{
    if(!m_currPluginFile.isEmpty()) {
        if(m_luaTextChanged) {
            if(QMessageBox::question(this, tr("Warning"), tr("Update File: ") + path,
                                     QMessageBox::Yes | QMessageBox::No) == QMessageBox::No){
                return;
            }
        }
        on_pb_load_plugin_file_clicked();
    }
}

void LuaPlugin::on_pb_save_plugin_file_clicked()
{
    if(!m_luaTextChanged) {
        return;
    }
    if(m_file_watcher.files().size()) {
        m_file_watcher.removePaths(m_file_watcher.files());
    }

    QFile file(m_currPluginFile);

    if(!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Warning"), tr("Open file error!"), QMessageBox::Yes );
        return;
    }

    file.write(ui->pte_lua_text->toPlainText().toUtf8());

    file.close();
    setWindowTitle("[Saved] " + m_currPluginFile);
    ui->pb_save_plugin_file->setEnabled(false);
     m_luaTextChanged = false;
     m_file_watcher.addPath(m_currPluginFile);
}

void LuaPlugin::on_pb_run_plugin_clicked(bool checked)
{
    if(checked) {
        QByteArray text = ui->pte_lua_text->toPlainText().toUtf8();
        m_lua_bind->lua_bind_do_string(text);
        ui->pb_run_plugin->setText("Stop");
    } else {
        m_lua_bind->lua_bind_abort();
        ui->pb_run_plugin->setText("Run");
    }
}

void LuaPlugin::on_pb_clear_lua_dbg_msg_clicked()
{
    ui->pte_lua_debug_msg->clear();
}

void LuaPlugin::on_pb_plugin_help_clicked()
{
    QStringList help;

    help << "Plugin API:";
    help << "  SerialPortWrite(table or string)";
    help << "  SerialPortRead(read_len, timeout)";
    help << "  GetOpenFileName()";
    help << "  GetSaveFileName";
    help << "  print(...)";
    help << "  msleep(ms)";

    for(int i=0; i<help.size(); i++) {
        ui->pte_lua_debug_msg->appendPlainText(help.at(i));
    }
}
