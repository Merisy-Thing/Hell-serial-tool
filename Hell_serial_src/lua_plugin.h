#ifndef LUAPLUGIN_H
#define LUAPLUGIN_H

#include <QDialog>
#include "luahighlighter.h"
#include "lua_bind.h"

namespace Ui {
class LuaPlugin;
}

class LuaPlugin : public QDialog
{
    Q_OBJECT

public:
    explicit LuaPlugin(QWidget *parent = 0);
    ~LuaPlugin();

    lua_bind *m_lua_bind;


private slots:
    void luaTextChanged();
    void pluginFinished();
    void pluginPrintMsg(QString msg);

    void on_pb_browse_plugin_file_clicked();
    void on_pb_load_plugin_file_clicked();
    void on_pb_save_plugin_file_clicked();
    void on_pb_run_plugin_clicked(bool checked);

    void on_pb_clear_lua_dbg_msg_clicked();

private:
    Ui::LuaPlugin *ui;

    Highlighter *m_LuaHighlighter;
    bool m_luaTextChanged;
    QString m_currPluginFile;

protected:
    void closeEvent ( QCloseEvent * e );
    void resizeEvent( QResizeEvent * e );
};

#endif // LUAPLUGIN_H
