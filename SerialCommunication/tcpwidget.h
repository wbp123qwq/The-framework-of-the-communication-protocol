#ifndef TCPWIDGET_H
#define TCPWIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include <QTcpServer>
#include <QHostAddress>
#include <QMessageBox>

namespace Ui {
class TcpWidget;
}

enum class TcpMode {
    None,
    Client,
    Server
};

class TcpWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TcpWidget(QWidget *parent = nullptr);
    ~TcpWidget();

private slots:
    void onConnectToServer();
    void onStartServer();
    void onSendData();
    void onReadyRead();
    void onNewConnection();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    Ui::TcpWidget *ui;

    QTcpSocket *m_tcpSocket;
    QTcpServer *m_tcpServer;
    QTcpSocket *m_serverClientSocket;

    TcpMode m_currentMode;

    void resetAllState();
    void updateButtonStates();
};

#endif // TCPWIDGET_H
