TEMPLATE = app

QT = gui core

unix:!mac{
    LIBS += -lpthread
}

HEADERS += \
    base/event/t_event.h \
    base/exception/t_exception.h \
    base/thread/t_resalloc.h \
    base/thread/t_thread.h \
    dev/serial/t_serialport.h \
    base/stropt/t_stropt.h \
    dev/socket/t_socket.h \
    dev/socket/t_tcpsocket.h \
    dev/socket/t_udpsocket.h \
    dev/socket/t_localsocket.h \
    dev/socket/t_netlinksocket.h \
    dev/socket/t_cansocket.h

SOURCES += \
    main.cpp \
    base/event/t_event.cpp \
    base/exception/t_exception.cpp \
    base/thread/t_resalloc.cpp \
    base/thread/t_thread.cpp \
    dev/serial/t_serialport_unix.cpp \ 
    base/stropt/t_stropt.cpp \
    dev/socket/t_socket.cpp \
    dev/socket/t_tcpsocket.cpp \
    dev/socket/t_udpsocket.cpp \
    dev/socket/t_localsocket.cpp \
    dev/socket/t_netlinksocket.cpp \
    dev/socket/t_cansocket.cpp

