#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTabWidget>


#include "tcpwidget.h"
#include "udpswidget.h"
#include "serialwidget.h"
#include "modbustcpwidget.h"
#include"fsecs.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    Ui::Widget *ui;
    fsecs*m_fsecs;
    QTabWidget *m_tabWidget;
    TcpWidget *m_tcpWidget;
    UdpWidget *m_udpWidget;
    SerialWidget *m_serialWidget;
    ModbusTcpWidget *m_modbusTcpWidget;
};
#endif // WIDGET_H
