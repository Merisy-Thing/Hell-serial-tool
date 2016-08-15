#include "lua_bind.h"
#include <QFile>
#include <QCoreApplication>
#include <QStringList>
#include <QMessageBox>
#include <QFileDialog>
#include <QTimer>


lua_State *lua_bind::L = NULL;
lua_bind  *lua_bind::m_this = NULL;

lua_bind::lua_bind(QObject *parent) : QThread(parent)
{
    lua_bind_init();

    m_abort_plugin = false;
    m_serial_data.clear();

    m_this = this;
}

lua_bind::~lua_bind()
{
    lua_close(L);
}

void * lua_bind::l_alloc (void *ud, void *ptr, size_t osize, size_t nsize)
{
    Q_UNUSED(ud);
    Q_UNUSED(osize);

    if(nsize == 0) {
        free(ptr);
        return NULL;
    } else if(!ptr) {
        return malloc(nsize);
    } else {
        return realloc(ptr, nsize);
    }
}

int lua_bind::l_panic (lua_State *_L)
{
    L = NULL;
    qFatal("Error in calling Lua API : %s", lua_tostring(_L, -1));
    return 0;
}

void lua_bind::lua_bind_init()
{
    if(L != NULL) {
        return;
    }

    L = lua_newstate(l_alloc, this);
    if(!L) {
        qWarning("Failed to create lua engine state");
        return;
    }
    lua_atpanic(L, l_panic);
    luaL_openlibs(L);

    //SerialPortWrite(table or string)  //支持数组和字符串
    //SerialPortRead(read_len, timeout) //read_len和timeout不能同时为0

    /* Declare API Func to Lua env : Start */
    const t_api_list api_list[] = {
        {"SerialPortWrite",  &lua_bind::_lua_bind_SerialPortWrite},
        {"SerialPortRead",   &lua_bind::_lua_bind_SerialPortRead},
        {"GetOpenFileName",  &lua_bind::_lua_bind_GetOpenFileName},
        {"GetSaveFileName",  &lua_bind::_lua_bind_GetSaveFileName},
        {"print",            &lua_bind::_lua_bind_DebugMessage},
        {"msleep",           &lua_bind::_lua_bind_msleep},
    };

    for(uint i=0; i<sizeof(api_list)/sizeof(t_api_list); i++) {
        lua_register(L, api_list[i].api_name, api_list[i].api_fn);
    }
    /* Declare C Func to Lua env : End */
}

QByteArray lua_bind::get_byte_table_value(lua_State *_L)
{
    int table_element_num;
    QByteArray ret;

    if(!lua_istable(_L, -1)) {//is table ?
        return ret;
    }

    table_element_num = lua_rawlen(_L, -1);
    if(table_element_num < 1) {
        goto lb_ret;
    }

    for(int i=0; i<table_element_num; i++) {
        lua_pushinteger(_L, 1 + i);
        lua_gettable(_L, -2);
        if(!lua_isnumber(_L, -1)) {
            ret.clear();
            goto lb_ret;
        } else {
            ret.append( (char)lua_tointeger(_L, -1) );
        }
        lua_pop(_L, 1);
    }

lb_ret:
    lua_pop(_L, 1);

    return ret;
}

void lua_bind::popup_msg_box(QString msg)
{
    QMessageBox::warning(NULL, tr("Plugin"), msg, QMessageBox::Ok);
}

int lua_bind::lua_bind_do_string(QByteArray &lua_str)
{
    m_lua_string = lua_str;
    //qDebug("m_lua_string: %s", m_lua_string.data());

    m_serial_data_mutex.lock();
    m_serial_data.clear();
    m_serial_data_mutex.unlock();

    this->start();
    return 0;
}

void lua_bind::run()
{
    if(m_lua_string.isEmpty()) {
        return;
    }
    m_abort_plugin = false;

    //qDebug("lua_bind start ");
    int ret = luaL_dostring(L, m_lua_string.data());

    if(ret != LUA_OK) {
        emit printMsg("ERROR: " + QString(lua_tostring(L,-1)));
        //popup_msg_box("Run error :\r\n  " + QString() + "\r\nReturn value = " + QString::number(ret));
    }
    //qDebug("lua_bind end ");
}

int lua_bind::lua_bind_SerialPortWrite(lua_State *_L)
{
    if(m_abort_plugin) {
        return 0;
    }
    //qDebug("lua_bind_SerialPortWrite");
    if(lua_gettop(_L) != 1) {
        return 0;
    }

    QByteArray send_data = get_byte_table_value(_L);
    if(send_data.size() == 0) {
        const char *str = lua_tostring(_L, -1);
        if(str) {
            send_data = QByteArray(str);
        }
    }

    int ret = 0;
    if(send_data.size() > 0) {
        emit SerialPortWrite(send_data);

        lua_pushboolean(_L, 1);
        ret = 1;
    }

    return ret;
}

void lua_bind::lua_bind_receive_serial_data(QByteArray &serial_data)
{
    if(m_serial_data_mutex.tryLock(1000)) {
        m_serial_data.append(serial_data);
        m_serial_data_mutex.unlock();
    } else {
        //qDebug("Error: Data drop");
    }
}

int lua_bind::lua_bind_SerialPortRead(lua_State *_L)
{
    int readed_size, read_len = 0, timeout = 0;

    if( lua_gettop(_L) != 2) {
        return 0;
    }

    if(!lua_isnumber(_L, -1)) {
        return 0;
    }
    timeout = lua_tointeger(_L, -1);
    //qDebug("timeout = %d", timeout);
    if( timeout < 0) {
        return 0;
    }

    if(!lua_isnumber(_L, -2)) {
        return 0;
    }
    read_len = lua_tointeger(_L, -2);
    //qDebug("read_len = %d", read_len);
    if( read_len < 0) {
        return 0;
    }

    if((timeout == 0) && (read_len == 0)) {
        return 0;
    }

    //qDebug("lua_bind_SerialPortRead, read len: %d", read_len);

    QTimer tm;
    tm.setTimerType(Qt::PreciseTimer);
    tm.setSingleShot(true);
    tm.start(timeout);
    do {
        if(m_abort_plugin) {
            //qDebug("lua_bind_SerialPortRead: abort");
            return 0;
        }

        m_serial_data_mutex.lock();
        readed_size = m_serial_data.size();
        m_serial_data_mutex.unlock();

        if( (read_len > 0) && (readed_size >= read_len )) {
            //qDebug("read_len = %d, readed_size = %d", read_len, readed_size);
            break;
        }
        if((timeout > 0) && (!tm.isActive())) {
            break;
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, timeout);
    }while(1);

    //qDebug("readed_size = %d", readed_size);

    if(!readed_size) {
        return 0;
    }

    lua_pop(_L, lua_gettop(_L));
    lua_newtable(_L);

    m_serial_data_mutex.lock();
    for(int i=0; i<m_serial_data.size(); i++) {
        lua_pushinteger(_L, (uchar)m_serial_data.at(i));
        lua_rawseti (_L, -2, i+1);//1
    }
    m_serial_data.clear();
    m_serial_data_mutex.unlock();

    return 1;
}

int lua_bind::lua_bind_DebugMessage(lua_State *_L)
{
    QString msg;

    int n = lua_gettop(_L);  /* number of arguments */
    int i;
    lua_getglobal(_L, "tostring");
    for (i=1; i<=n; i++) {
      const char *s;
      size_t l;
      lua_pushvalue(_L, -1);  /* function to be called */
      lua_pushvalue(_L, i);   /* value to print */
      lua_call(_L, 1, 1);
      s = lua_tolstring(_L, -1, &l);  /* get result */
      if (s == NULL)
        return luaL_error(_L, "'tostring' must return a string to 'print'");
      if (i>1) msg.append("\t");
      msg.append(s);
      lua_pop(_L, 1);  /* pop result */
    }

    emit printMsg(msg);

    return 0;
}

int lua_bind::lua_bind_msleep(lua_State *_L)
{
    int ms = 0;

    if(lua_gettop(_L) != 1) {
        return 0;
    }

    if(!lua_isnumber(_L, -1)) {
        return 0;
    }
    ms = lua_tointeger(_L, -1);
    if( ms <= 0) {
        return 0;
    }

    //qDebug("msleep: %d", ms);
    msleep(ms);

    lua_pushboolean(_L, 1);
    return 1;
}

int lua_bind::lua_bind_GetOpenFileName(lua_State *_L)
{
    Q_UNUSED(_L);
    if(m_abort_plugin) {
        return 0;
    }
    //qDebug("lua_bind_GetOpenFileName");

    QString save_file = QFileDialog::getOpenFileName(NULL, tr("Open file"), "", tr("All (*.*)"));
    if( save_file.isEmpty() ) {
        return 0;
    }

    lua_pushstring(L, save_file.toUtf8().data());

    return 1;
}

int lua_bind::lua_bind_GetSaveFileName(lua_State *_L)
{
    Q_UNUSED(_L);
    if(m_abort_plugin) {
        return 0;
    }

    QString output_file = QFileDialog::getSaveFileName(NULL, tr("Save file"), "", tr("All (*.*)"));
    if( output_file.isEmpty() ) {
        return 0;
    }

    lua_pushstring(L, output_file.toUtf8().data());

    //qDebug("lua_bind_GetSaveFileName");
    return 1;
}


