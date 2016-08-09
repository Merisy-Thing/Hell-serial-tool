#include <QFileDialog>
#include <QMessageBox>

#include "lua_plugin.h"
#include "ui_lua_plugin.h"

LuaPlugin::LuaPlugin(QWidget *parent) :
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

    m_luaTextChanged = false;

    ui->pb_save_plugin_file->setEnabled(false);
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

void LuaPlugin::luaTextChanged()
{
    if(!m_currPluginFile.isEmpty()) {
        m_luaTextChanged = true;
        setWindowTitle(m_currPluginFile + " [Changed]");
        ui->pb_save_plugin_file->setEnabled(true);
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

    ui->pte_lua_text->clear();
    m_LuaHighlighter->highlightBlock(lua_str);
    ui->pte_lua_text->setPlainText(lua_str);
    m_currPluginFile = ui->cb_plugin_file_list->currentText();
    setWindowTitle(m_currPluginFile);
}

void LuaPlugin::on_pb_save_plugin_file_clicked()
{
    QFile file(ui->cb_plugin_file_list->currentText());

    if(!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Warning"), tr("Open file error!"), QMessageBox::Yes );
        return;
    }

    file.write(ui->pte_lua_text->toPlainText().toUtf8());

    file.close();
    setWindowTitle(m_currPluginFile + " [Saved]");
    ui->pb_save_plugin_file->setEnabled(false);
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
