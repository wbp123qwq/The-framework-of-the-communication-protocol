QT += core gui widgets network serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    fsecs.cpp \
    main.cpp \
    modbustcpwidget.cpp \
    serialwidget.cpp \
    tcpwidget.cpp \
    udpwidget.cpp \
    widget.cpp

HEADERS += \
    fsecs.h \
    modbustcpwidget.h \
    serialwidget.h \
    tcpwidget.h \
    udpwidget.h \
    widget.h

FORMS += \
    TcpWidget.ui \
    fsecs.ui \
    modbustcpwidget.ui \
    serialwidget.ui \
    udpwidget.ui \
    widget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
