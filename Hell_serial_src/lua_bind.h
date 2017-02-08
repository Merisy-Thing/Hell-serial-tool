#ifndef LUA_BIND_H
#define LUA_BIND_H

#include <QObject>
#include <QMutex>
#include <QThread>

#include "lua_5.3.x/lua.hpp"

typedef struct{
    const char *api_name;
    lua_CFunction api_fn;
} t_api_list;


class lua_bind : public QThread
{
    Q_OBJECT
public:
    explicit lua_bind(QObject *parent = 0);
    ~lua_bind();

    void lua_bind_receive_serial_data(QByteArray &serial_data);
    void lua_bind_abort();
    int  lua_bind_do_string(QByteArray &lua_str);

private:
    QByteArray m_lua_string;
    QMutex     m_serial_data_mutex;
    QByteArray m_serial_data;
    bool       m_abort_plugin;

    void lua_bind_init();
    void popup_msg_box(QString msg);

    QByteArray get_byte_table_value(lua_State *_L);

    int lua_bind_SerialPortWrite(lua_State *_L);
    int lua_bind_SerialPortRead(lua_State *_L);
    int lua_bind_GetOpenFileName(lua_State *_L);
    int lua_bind_GetSaveFileName(lua_State *_L);
    int lua_bind_msleep(lua_State *_L);
    int lua_bind_utf8ToLocal(lua_State *_L);
    int lua_bind_DebugMessage(lua_State *_L);

private:
    static lua_bind  *m_this;


    //---------------------------------------------//
    static lua_State * L;


    static void *l_alloc (void *ud, void *ptr, size_t osize, size_t nsize);
    static int l_panic (lua_State *L);

    static int _lua_bind_SerialPortWrite(lua_State *_L) {
        return m_this->lua_bind_SerialPortWrite(_L);
    }
    static int _lua_bind_SerialPortRead(lua_State *_L) {
        return m_this->lua_bind_SerialPortRead(_L);
    }
    static int _lua_bind_GetOpenFileName(lua_State *_L) {
        return m_this->lua_bind_GetOpenFileName(_L);
    }
    static int _lua_bind_GetSaveFileName(lua_State *_L) {
        return m_this->lua_bind_GetSaveFileName(_L);
    }
    static int _lua_bind_DebugMessage(lua_State *_L) {
       return  m_this->lua_bind_DebugMessage(_L);
    }
    static int _lua_bind_msleep(lua_State *_L) {
       return  m_this->lua_bind_msleep(_L);
    }
    static int _lua_bind_utf8ToLocal(lua_State *_L) {
       return  m_this->lua_bind_utf8ToLocal(_L);
    }

protected:
    void run();

signals:
    void SerialPortWrite(QByteArray);
    void printMsg(QString msg);

public slots:
};

#endif // LUA_BIND_H
