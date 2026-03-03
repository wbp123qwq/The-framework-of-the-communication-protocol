#include "tcpwidget.h"
#include "ui_TcpWidget.h"
#include <QIntValidator>

TcpWidget::TcpWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TcpWidget),
    m_serverClientSocket(nullptr),
    m_currentMode(TcpMode::None)
{
    ui->setupUi(this);

    ui->lineEditIp->setText("127.0.0.1");
    ui->lineEditPort->setValidator(new QIntValidator(1, 65535, this));
    ui->lineEditPort->setText("8888");
    ui->textEditLog->setReadOnly(true);
    ui->textEditLog->setMinimumHeight(200);
    ui->btnSend->setEnabled(false);

    m_tcpSocket = new QTcpSocket(this);
    m_tcpServer = new QTcpServer(this);

    connect(ui->btnConnect, &QPushButton::clicked, this, &TcpWidget::onConnectToServer);
    connect(ui->btnStartServer, &QPushButton::clicked, this, &TcpWidget::onStartServer);
    connect(ui->btnSend, &QPushButton::clicked, this, &TcpWidget::onSendData);

    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &TcpWidget::onReadyRead);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &TcpWidget::onSocketDisconnected);
    connect(m_tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &TcpWidget::onSocketError);

    connect(m_tcpServer, &QTcpServer::newConnection, this, &TcpWidget::onNewConnection);
}

TcpWidget::~TcpWidget()
{
    resetAllState();
    delete ui;
}

void TcpWidget::resetAllState()
{
    if (m_tcpSocket->state() != QTcpSocket::UnconnectedState) {
        m_tcpSocket->abort();
    }

    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
    }

    if (m_serverClientSocket) {
        m_serverClientSocket->abort();
        m_serverClientSocket->deleteLater();
        m_serverClientSocket = nullptr;
    }

    m_currentMode = TcpMode::None;
    ui->btnConnect->setText("连接服务器");
    ui->btnStartServer->setText("启动服务端");
    updateButtonStates();
}

void TcpWidget::updateButtonStates()
{
    switch (m_currentMode) {
    case TcpMode::None:
        ui->btnConnect->setEnabled(true);
        ui->btnStartServer->setEnabled(true);
        ui->btnSend->setEnabled(false);
        break;
    case TcpMode::Client:
        ui->btnConnect->setEnabled(true);
        ui->btnStartServer->setEnabled(false);
        ui->btnSend->setEnabled(m_tcpSocket->state() == QTcpSocket::ConnectedState);
        break;
    case TcpMode::Server:
        ui->btnConnect->setEnabled(false);
        ui->btnStartServer->setEnabled(true);
        ui->btnSend->setEnabled(m_serverClientSocket && m_serverClientSocket->state() == QTcpSocket::ConnectedState);
        break;
    }
}

void TcpWidget::onConnectToServer()
{
    if (m_tcpSocket->state() == QTcpSocket::ConnectedState) {
        resetAllState();
        ui->textEditLog->append("已主动断开与服务器的连接");
        return;
    }

    resetAllState();

    QString ip = ui->lineEditIp->text().trimmed();
    QString portStr = ui->lineEditPort->text().trimmed();
    if (ip.isEmpty() || portStr.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "IP地址和端口不能为空！");
        return;
    }
    quint16 port = portStr.toUInt();

    m_currentMode = TcpMode::Client;
    updateButtonStates();

    m_tcpSocket->connectToHost(ip, port);
    if (!m_tcpSocket->waitForConnected(3000)) {
        resetAllState();
        ui->textEditLog->append("连接失败：" + m_tcpSocket->errorString());
        QMessageBox::critical(this, "连接失败", "无法连接到 " + ip + ":" + portStr + "\n原因：" + m_tcpSocket->errorString());
    } else {
        ui->textEditLog->append("成功连接到服务器：" + ip + ":" + portStr);
        ui->btnConnect->setText("断开服务器");
        updateButtonStates();
    }
}

void TcpWidget::onStartServer()
{
    if (m_serverClientSocket || m_tcpServer->isListening()) {
        resetAllState();
        ui->textEditLog->append("已停止服务端监听");
        return;
    }

    resetAllState();

    QString portStr = ui->lineEditPort->text().trimmed();
    if (portStr.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "端口不能为空！");
        return;
    }
    quint16 port = portStr.toUInt();

    m_currentMode = TcpMode::Server;
    updateButtonStates();

    if (!m_tcpServer->listen(QHostAddress::AnyIPv4, port)) {
        resetAllState();
        ui->textEditLog->append("服务端启动失败：" + m_tcpServer->errorString());
        QMessageBox::critical(this, "启动失败", "无法监听端口 " + portStr + "\n原因：" + m_tcpServer->errorString());
    } else {
        ui->textEditLog->append("服务端启动成功，监听端口：" + portStr);
        ui->btnStartServer->setText("停止服务端");
        updateButtonStates();
    }
}

void TcpWidget::onSendData()
{
    QString data = ui->lineEditSendData->text().trimmed();
    if (data.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "发送数据不能为空！");
        return;
    }

    QTcpSocket *sendSocket = nullptr;
    if (m_currentMode == TcpMode::Client) {
        sendSocket = m_tcpSocket;
    } else if (m_currentMode == TcpMode::Server) {
        sendSocket = m_serverClientSocket;
    }

    if (!sendSocket || sendSocket->state() != QTcpSocket::ConnectedState) {
        QMessageBox::warning(this, "发送失败", "未建立有效连接，无法发送数据！");
        return;
    }

    QByteArray sendData = data.toUtf8();
    qint64 len = sendSocket->write(sendData);
    if (len > 0) {
        ui->textEditLog->append("发送数据：" + data);
        ui->lineEditSendData->clear();
    } else {
        ui->textEditLog->append("发送失败：" + sendSocket->errorString());
    }
}

void TcpWidget::onReadyRead()
{
    QTcpSocket *recvSocket = qobject_cast<QTcpSocket*>(sender());
    if (!recvSocket) return;

    QByteArray data = recvSocket->readAll();
    if (!data.isEmpty()) {
        ui->textEditLog->append("接收数据：" + QString::fromUtf8(data));
    }
}

void TcpWidget::onNewConnection()
{
    if (m_serverClientSocket)
    {
        m_serverClientSocket->abort();
        m_serverClientSocket->deleteLater();
    }
    m_serverClientSocket = m_tcpServer->nextPendingConnection();
    ui->textEditLog->append("新客户端连接：" + m_serverClientSocket->peerAddress().toString() +
                            ":" + QString::number(m_serverClientSocket->peerPort()));
    connect(m_serverClientSocket, &QTcpSocket::readyRead, this, &TcpWidget::onReadyRead);
    connect(m_serverClientSocket, &QTcpSocket::disconnected, this, [this]()
            {
        ui->textEditLog->append("客户端断开连接：" + m_serverClientSocket->peerAddress().toString());
        m_serverClientSocket->deleteLater();
        m_serverClientSocket = nullptr;
        updateButtonStates();
    });
    connect(m_serverClientSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &TcpWidget::onSocketError);

    updateButtonStates();
}

void TcpWidget::onSocketDisconnected()
{
    ui->textEditLog->append("连接已断开");
    resetAllState();
}

void TcpWidget::onSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    QTcpSocket *errSocket = qobject_cast<QTcpSocket*>(sender());
    if (errSocket)
    {
        ui->textEditLog->append("通信错误：" + errSocket->errorString());
        resetAllState();
    }
}
