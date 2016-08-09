#ifndef LUAPLUGIN_H
#define LUAPLUGIN_H

#include <QDialog>
#include <QFileSystemWatcher>
#include <QSettings>
#include "luahighlighter.h"
#include "lua_bind.h"

namespace Ui {
class LuaPlugin;
}

class LuaPlugin : public QDialog
{
    Q_OBJECT

public:
    explicit LuaPlugin(QSettings *setting_file, QWidget *parent = 0);
    ~LuaPlugin();

    lua_bind *m_lua_bind;


private slots:
    void luaTextChanged();
    void pluginFinished();
    void pluginPrintMsg(QString msg);
    void file_watcher_fileChanged(QString path);

    void on_pb_browse_plugin_file_clicked();
    void on_pb_load_plugin_file_clicked();
    void on_pb_save_plugin_file_clicked();
    void on_pb_run_plugin_clicked(bool checked);
    void on_pb_clear_lua_dbg_msg_clicked();
    void on_pb_plugin_help_clicked();

private:
    Ui::LuaPlugin *ui;

    Highlighter *m_LuaHighlighter;
    bool m_luaTextChanged;
    QString m_currPluginFile;

    QFileSystemWatcher m_file_watcher;
    QSettings *m_setting_file;

protected:
    void closeEvent ( QCloseEvent * e );
    void resizeEvent( QResizeEvent * e );
    void keyPressEvent ( QKeyEvent * event );
};

#endif // LUAPLUGIN_H
