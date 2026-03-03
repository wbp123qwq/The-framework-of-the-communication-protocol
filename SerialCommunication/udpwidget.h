#ifndef UDPWIDGET_H
#define UDPWIDGET_H

#include <QWidget>
#include <QUdpSocket>
#include <QHostAddress>

// 引入UI类
namespace Ui {
class UdpWidget;
}

class UdpWidget : public QWidget
{
    Q_OBJECT
public:
    explicit UdpWidget(QWidget *parent = nullptr);
    ~UdpWidget();

private slots:
    void onSendData();
    void onReadyRead();

private:
    Ui::UdpWidget *ui;
    QUdpSocket *m_udpSocket;
};

#endif // UDPWIDGET_H
