#include "serialwidget.h"
#include "ui_serialwidget.h"

SerialWidget::SerialWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SerialWidget)
{
    ui->setupUi(this);

    initSerialParams();

    loadAvailablePorts();

    m_serialPort = new QSerialPort(this);

    connect(ui->btnOpenSerial, &QPushButton::clicked, this, &SerialWidget::onOpenSerial);
    connect(ui->btnSendData, &QPushButton::clicked, this, &SerialWidget::onSendData);
    connect(ui->btnRefreshPort, &QPushButton::clicked, this, &SerialWidget::onRefreshPortList);
    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialWidget::onReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialWidget::onSerialError);

    updateButtonStates();
}

SerialWidget::~SerialWidget()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }
    delete m_serialPort;
    delete ui;
}

void SerialWidget::initSerialParams()
{
    ui->comboBaudRate->addItems({"9600", "19200", "38400", "57600", "115200"});
    ui->comboBaudRate->setCurrentText("9600");

    ui->comboDataBits->addItem("8", QSerialPort::Data8);
    ui->comboDataBits->addItem("7", QSerialPort::Data7);
    ui->comboDataBits->addItem("6", QSerialPort::Data6);
    ui->comboDataBits->addItem("5", QSerialPort::Data5);
    ui->comboDataBits->setCurrentIndex(0);

    ui->comboParity->addItem("无", QSerialPort::NoParity);
    ui->comboParity->addItem("奇校验", QSerialPort::OddParity);
    ui->comboParity->addItem("偶校验", QSerialPort::EvenParity);
    ui->comboParity->setCurrentIndex(0);

    ui->comboStopBits->addItem("1", QSerialPort::OneStop);
    ui->comboStopBits->addItem("1.5", QSerialPort::OneAndHalfStop);
    ui->comboStopBits->addItem("2", QSerialPort::TwoStop);
    ui->comboStopBits->setCurrentIndex(0);
}

void SerialWidget::loadAvailablePorts()
{
    ui->comboPortName->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        QString portText = QString("%1 (%2)").arg(info.portName()).arg(info.description());
        ui->comboPortName->addItem(portText, info.portName());
    }
    if (ui->comboPortName->count() == 0) {
        ui->btnOpenSerial->setEnabled(false);
        ui->textEditRecvData->append(
            QString("[%1] 未检测到可用串口")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    } else
    {
        ui->btnOpenSerial->setEnabled(true);
    }
}

void SerialWidget::updateButtonStates()
{
    if (m_isSerialOpen)
    {
        ui->btnOpenSerial->setText("关闭串口");
        ui->btnSendData->setEnabled(true);
        ui->comboPortName->setEnabled(false);
        ui->comboBaudRate->setEnabled(false);
        ui->comboDataBits->setEnabled(false);
        ui->comboParity->setEnabled(false);
        ui->comboStopBits->setEnabled(false);
        ui->btnRefreshPort->setEnabled(false);
    }
    else
    {
        ui->btnOpenSerial->setText("打开串口");
        ui->btnSendData->setEnabled(false);
        ui->comboPortName->setEnabled(true);
        ui->comboBaudRate->setEnabled(true);
        ui->comboDataBits->setEnabled(true);
        ui->comboParity->setEnabled(true);
        ui->comboStopBits->setEnabled(true);
        ui->btnRefreshPort->setEnabled(true);
    }
}

void SerialWidget::onRefreshPortList()
{
    loadAvailablePorts();
    ui->textEditRecvData->append(
        QString("[%1] 已刷新串口列表")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
}

void SerialWidget::onOpenSerial()
{
    if (m_isSerialOpen)
    {
        m_serialPort->close();
        m_isSerialOpen = false;
        ui->textEditRecvData->append(
            QString("[%1] 串口已关闭")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    }
    else
    {
        if (ui->comboPortName->currentIndex() == -1) {
            QMessageBox::warning(this, "错误", "请选择有效的串口！");
            return;
        }

        QString portName = ui->comboPortName->currentData().toString();
        qint32 baudRate = ui->comboBaudRate->currentText().toInt();
        QSerialPort::DataBits dataBits = static_cast<QSerialPort::DataBits>(
            ui->comboDataBits->currentData().toInt());
        QSerialPort::Parity parity = static_cast<QSerialPort::Parity>(
            ui->comboParity->currentData().toInt());
        QSerialPort::StopBits stopBits = static_cast<QSerialPort::StopBits>(
            ui->comboStopBits->currentData().toInt());

        m_serialPort->setPortName(portName);
        m_serialPort->setBaudRate(baudRate);
        m_serialPort->setDataBits(dataBits);
        m_serialPort->setParity(parity);
        m_serialPort->setStopBits(stopBits);
        m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

        if (m_serialPort->open(QIODevice::ReadWrite))
        {
            m_isSerialOpen = true;
            ui->textEditRecvData->append(
                QString("[%1] 串口打开成功：%2 @ %3bps")
                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"),
                         portName,
                         QString::number(baudRate)));
        }
        else
        {
            QMessageBox::critical(this, "错误", "串口打开失败：" + m_serialPort->errorString());
            ui->textEditRecvData->append(
                QString("[%1] 串口打开失败：%2")
                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"),
                         m_serialPort->errorString()));
            return;
        }
    }

    updateButtonStates();
}

void SerialWidget::onSendData()
{
    if (!m_isSerialOpen || !m_serialPort->isOpen()) {
        QMessageBox::warning(this, "错误", "串口未打开，无法发送数据！");
        return;
    }

    QString sendText = ui->lineEditSendData->text().trimmed();
    if (sendText.isEmpty()) {
        QMessageBox::warning(this, "提示", "发送数据不能为空！");
        return;
    }

    QByteArray sendData = sendText.toUtf8();
    qint64 sendLen = m_serialPort->write(sendData);

    if (sendLen > 0) {
        ui->textEditRecvData->append(
            QString("[%1] 发送：%2")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), sendText));
        ui->lineEditSendData->clear();
    } else {
        ui->textEditRecvData->append(
            QString("[%1] 发送失败：%2")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"),
                     m_serialPort->errorString()));
    }
}

void SerialWidget::onReadyRead()
{
    if (!m_isSerialOpen)
        return;

    QByteArray recvData = m_serialPort->readAll();
    if (!recvData.isEmpty())
    {
        ui->textEditRecvData->append(
            QString("[%1] 接收：%2")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"),
                     QString::fromUtf8(recvData)));
    }
}

void SerialWidget::onSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError)
        return;

    ui->textEditRecvData->append(
        QString("[%1] 串口错误：%2")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"),
                 m_serialPort->errorString()));

    if (m_isSerialOpen)
    {
        m_serialPort->close();
        m_isSerialOpen = false;
        updateButtonStates();
    }
}
