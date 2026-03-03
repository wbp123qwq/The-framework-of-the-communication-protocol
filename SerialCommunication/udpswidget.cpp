#include "udpwidget.h"
#include "ui_udpwidget.h"
#include <QIntValidator>

UdpWidget::UdpWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UdpWidget)
{
    // 加载UI设计
    ui->setupUi(this);

    // 设置端口输入框为数字验证（1-65535）
    ui->lineEditPort->setValidator(new QIntValidator(1, 65535, this));

    // 初始化UDP对象
    m_udpSocket = new QUdpSocket(this);
    // 绑定本地端口用于接收
    m_udpSocket->bind(9999);

    // 信号槽连接
    connect(ui->btnSend, &QPushButton::clicked, this, &UdpWidget::onSendData);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &UdpWidget::onReadyRead);
}

UdpWidget::~UdpWidget()
{
    // 释放资源
    if (m_udpSocket->isOpen()) {
        m_udpSocket->close();
    }
    delete m_udpSocket;
    delete ui;
}

void UdpWidget::onSendData()
{
    // 从UI获取输入
    QString ip = ui->lineEditIp->text().trimmed();
    QString portStr = ui->lineEditPort->text().trimmed();
    QString data = ui->lineEditSendData->text().trimmed();

    // 输入验证
    if (ip.isEmpty() || portStr.isEmpty()) {
        ui->textEditRecv->append("错误：IP地址或端口不能为空");
        return;
    }
    if (data.isEmpty()) {
        ui->textEditRecv->append("错误：发送数据不能为空");
        return;
    }

    quint16 port = portStr.toUInt();
    // 发送UDP数据报
    qint64 len = m_udpSocket->writeDatagram(data.toUtf8(), QHostAddress(ip), port);
    if (len > 0) {
        ui->textEditRecv->append("发送UDP数据：" + data + " 到 " + ip + ":" + QString::number(port));
        ui->lineEditSendData->clear(); // 发送成功后清空输入框
    } else {
        ui->textEditRecv->append("UDP发送失败：" + m_udpSocket->errorString());
    }
}

void UdpWidget::onReadyRead()
{
    // 读取所有待处理的UDP数据报
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        ui->textEditRecv->append("接收UDP数据：" + QString(datagram) + " 来自 " + sender.toString() + ":" + QString::number(senderPort));
    }
}
