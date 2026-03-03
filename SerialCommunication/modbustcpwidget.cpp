#include "modbustcpwidget.h"
#include "ui_modbustcpwidget.h"
#include <QDebug>
#include <QDataStream>
#include <QMessageBox>
#include <cmath>

ModbusTcpWidget::ModbusTcpWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ModbusTcpWidget),
    m_transactionId(1)
{
    ui->setupUi(this);

    m_tcpSocket = new QTcpSocket(this);

    connect(ui->pushButton_connect, &QPushButton::clicked, this, &ModbusTcpWidget::onConnect);
    connect(ui->pushButton_readCoils, &QPushButton::clicked, this, &ModbusTcpWidget::onReadCoils);
    connect(ui->pushButton_writeCoil, &QPushButton::clicked, this, &ModbusTcpWidget::onWriteCoil);
    connect(ui->pushButton_readRegs, &QPushButton::clicked, this, &ModbusTcpWidget::onReadHoldingRegisters);
    connect(ui->pushButton_writeReg, &QPushButton::clicked, this, &ModbusTcpWidget::onWriteHoldingRegister);
    connect(ui->pushButton_clearLog, &QPushButton::clicked, this, &ModbusTcpWidget::onClearLog);
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &ModbusTcpWidget::onReadyRead);
    connect(m_tcpSocket, &QTcpSocket::errorOccurred, this, &ModbusTcpWidget::onSocketError);
    connect(m_tcpSocket, &QTcpSocket::disconnected, [this]() {
        ui->pushButton_connect->setText("连接Modbus服务器");
        ui->pushButton_readCoils->setEnabled(false);
        ui->pushButton_writeCoil->setEnabled(false);
        ui->pushButton_readRegs->setEnabled(false);
        ui->pushButton_writeReg->setEnabled(false);
        ui->textEdit_log->append(QString("[%1] 与服务器断开连接").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    });
}

ModbusTcpWidget::~ModbusTcpWidget()
{
    if (m_tcpSocket->state() == QTcpSocket::ConnectedState) {
        m_tcpSocket->disconnectFromHost();
        m_tcpSocket->waitForDisconnected(1000);
    }
    delete ui;
}

void ModbusTcpWidget::onConnect()
{
    if (m_tcpSocket->state() == QTcpSocket::ConnectedState) {
        m_tcpSocket->disconnectFromHost();
        ui->pushButton_connect->setText("连接Modbus服务器");
        ui->pushButton_readCoils->setEnabled(false);
        ui->pushButton_writeCoil->setEnabled(false);
        ui->pushButton_readRegs->setEnabled(false);
        ui->pushButton_writeReg->setEnabled(false);
        ui->textEdit_log->append(QString("[%1] 已断开ModbusTCP连接").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    } else {

        QString ip = ui->lineEdit_ip->text().trimmed();

        QString portStr = ui->spinBox_port->text().trimmed();


        if (ip.isEmpty()) {
            QMessageBox::warning(this, "输入错误", "请输入服务器IP地址！");
            return;
        }


        if (portStr.isEmpty()) {
            QMessageBox::warning(this, "输入错误", "请输入服务器端口号！");
            return;
        }
        bool isPortValid;
        quint16 port = static_cast<quint16>(portStr.toUInt(&isPortValid));
        if (!isPortValid || port <= 0 || port > 65535) {
            QMessageBox::warning(this, "输入错误", "请输入1-65535之间的有效端口号！");
            return;
        }


        m_tcpSocket->abort();

        m_tcpSocket->connectToHost(ip, port);

        if (m_tcpSocket->waitForConnected(3000)) {
            ui->pushButton_connect->setText("断开连接");
            ui->pushButton_readCoils->setEnabled(true);
            ui->pushButton_writeCoil->setEnabled(true);
            ui->pushButton_readRegs->setEnabled(true);
            ui->pushButton_writeReg->setEnabled(true);
            ui->textEdit_log->append(QString("[%1] 成功连接ModbusTCP服务器：%2:%3")
                                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                                         .arg(ip).arg(port));
        } else {
            ui->textEdit_log->append(QString("[%1] 连接失败：%2")
                                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                                         .arg(m_tcpSocket->errorString()));
        }
    }
}

quint16 ModbusTcpWidget::getNextTransactionId()
{
    m_transactionId++;
    if (m_transactionId >= 0xFFFF) {
        m_transactionId = 1;
    }
    return m_transactionId;
}

QByteArray ModbusTcpWidget::buildReadCoilsRequest(int addr, int count)
{
    QByteArray request;
    QDataStream stream(&request, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    stream << getNextTransactionId();
    stream << (quint16)0x0000;
    stream << (quint16)0x0006;
    stream << (quint8)0x01;
    stream << (quint8)0x01;
    stream << (quint16)addr;
    stream << (quint16)count;

    return request;
}

QByteArray ModbusTcpWidget::buildWriteCoilRequest(int addr, int value)
{
    QByteArray request;
    QDataStream stream(&request, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    stream << getNextTransactionId();
    stream << (quint16)0x0000;
    stream << (quint16)0x0006;
    stream << (quint8)0x01;
    stream << (quint8)0x05;
    stream << (quint16)addr;
    stream << (quint16)(value ? 0xFF00 : 0x0000);

    return request;
}

QByteArray ModbusTcpWidget::buildReadHoldingRegistersRequest(int addr, int count)
{
    QByteArray request;
    QDataStream stream(&request, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    stream << getNextTransactionId();
    stream << (quint16)0x0000;
    stream << (quint16)0x0006;
    stream << (quint8)0x01;
    stream << (quint8)0x03;
    stream << (quint16)addr;
    stream << (quint16)count;

    return request;
}

QByteArray ModbusTcpWidget::buildWriteHoldingRegisterRequest(int addr, int value)
{
    QByteArray request;
    QDataStream stream(&request, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    stream << getNextTransactionId();
    stream << (quint16)0x0000;
    stream << (quint16)0x0006;
    stream << (quint8)0x01;
    stream << (quint8)0x06;
    stream << (quint16)addr;
    stream << (quint16)value;

    return request;
}

QByteArray ModbusTcpWidget::buildWriteHoldingRegisterRequestFloat(int addr, float value)
{
    QByteArray request;
    QDataStream stream(&request, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    QByteArray floatBytes = floatToBytes(value);
    quint16 intValue = 0;

    if (floatBytes.size() >= 2) {
        intValue = (static_cast<quint8>(floatBytes[0]) << 8) | static_cast<quint8>(floatBytes[1]);
    }

    stream << getNextTransactionId();
    stream << (quint16)0x0000;
    stream << (quint16)0x0006;
    stream << (quint8)0x01;
    stream << (quint8)0x06;
    stream << (quint16)addr;
    stream << intValue;

    return request;
}

QByteArray ModbusTcpWidget::floatToBytes(float value)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << value;
    return bytes;
}

float ModbusTcpWidget::bytesToFloat(const QByteArray &data)
{
    if (data.size() < 4) return 0.0f;
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    float value;
    stream >> value;
    return value;
}

void ModbusTcpWidget::parseReadCoilsResponse(const QByteArray &response)
{
    if (response.size() < 9) {
        ui->textEdit_log->append(QString("[%1] 响应报文格式错误：长度不足")
                                     .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
        return;
    }

    QDataStream stream(response);
    stream.setByteOrder(QDataStream::BigEndian);
    quint16 transId, protoId, len;
    quint8 unitId, funcCode, byteCount;
    stream >> transId >> protoId >> len >> unitId >> funcCode >> byteCount;

    if (funcCode == 0x01) {
        ui->textEdit_log->append(QString("[%1] 读取线圈响应 - 字节数：%2").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")).arg(byteCount));
        int addr = ui->spinBox_addr->value();
        int count = ui->spinBox_count->value();

        for (int i = 0; i < count && i < byteCount * 8; i++) {
            int byteIndex = i / 8;
            int bitIndex = i % 8;
            quint8 byteValue = static_cast<quint8>(response[8 + byteIndex]);
            int coilState = (byteValue >> (7 - bitIndex)) & 0x01;

            ui->textEdit_log->append(QString("  线圈地址%1: %2").arg(addr + i).arg(coilState));
        }
    } else if (funcCode == 0x81) {
        quint8 errorCode = response[8];
        QString errorMsg = "";
        switch (errorCode) {
        case 0x01: errorMsg = "非法功能码"; break;
        case 0x02: errorMsg = "非法数据地址"; break;
        case 0x03: errorMsg = "非法数据值"; break;
        default: errorMsg = "未知错误";
        }
        ui->textEdit_log->append(QString("[%1] 读取线圈失败：错误码=%2 (%3)")
                                     .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                                     .arg(errorCode).arg(errorMsg));
    }
}

void ModbusTcpWidget::parseReadHoldingRegistersResponse(const QByteArray &response)
{
    if (response.size() < 9) {
        ui->textEdit_log->append(QString("[%1] 响应报文格式错误：长度不足")
                                     .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
        return;
    }

    QDataStream stream(response);
    stream.setByteOrder(QDataStream::BigEndian);

    quint16 transId, protoId, len;
    quint8 unitId, funcCode, byteCount;
    stream >> transId >> protoId >> len >> unitId >> funcCode >> byteCount;

    if (funcCode == 0x03) {
        ui->textEdit_log->append(QString("[%1] 读取寄存器响应 - 字节数：%2").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")).arg(byteCount));

        int addr = ui->spinBox_addr->value();
        int count = ui->spinBox_count->value();
        DataType dataType = static_cast<DataType>(ui->comboBox_dataType->currentData().toInt());

        if (dataType == Int16) {
            for (int i = 0; i < count && i * 2 < byteCount; i++) {
                quint16 regValue = (static_cast<quint8>(response[8 + i*2]) << 8) | static_cast<quint8>(response[9 + i*2]);
                ui->textEdit_log->append(QString("  寄存器地址%1: %2 (0x%3)")
                                             .arg(addr + i)
                                             .arg(static_cast<qint16>(regValue))
                                             .arg(QString::number(regValue, 16).toUpper().rightJustified(4, '0')));
            }
        } else if (dataType == Float32) {
            int floatCount = count / 2;
            for (int i = 0; i < floatCount && i * 4 < byteCount; i++) {
                QByteArray floatData;
                floatData.append(response[8 + i*4]);
                floatData.append(response[9 + i*4]);
                floatData.append(response[10 + i*4]);
                floatData.append(response[11 + i*4]);

                float floatValue = bytesToFloat(floatData);
                ui->textEdit_log->append(QString("  寄存器地址%1-%2: %3")
                                             .arg(addr + i*2)
                                             .arg(addr + i*2 + 1)
                                             .arg(floatValue));
            }
        }
    } else if (funcCode == 0x83) {
        quint8 errorCode = response[8];
        QString errorMsg = "";
        switch (errorCode) {
        case 0x01: errorMsg = "非法功能码"; break;
        case 0x02: errorMsg = "非法数据地址"; break;
        case 0x03: errorMsg = "非法数据值"; break;
        default: errorMsg = "未知错误";
        }
        ui->textEdit_log->append(QString("[%1] 读取寄存器失败：错误码=%2 (%3)")
                                     .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                                     .arg(errorCode).arg(errorMsg));
    }
}

QString ModbusTcpWidget::formatModbusMessage(const QByteArray &data, bool isRequest)
{
    QString prefix = isRequest ? "发送请求报文" : "接收响应报文";
    QString hexStr = data.toHex(' ').toUpper();

    QString mbapInfo = "";
    if (data.size() >= 7) {
        quint16 transId = (static_cast<quint8>(data[0]) << 8) | static_cast<quint8>(data[1]);
        quint16 protoId = (static_cast<quint8>(data[2]) << 8) | static_cast<quint8>(data[3]);
        quint16 len = (static_cast<quint8>(data[4]) << 8) | static_cast<quint8>(data[5]);
        quint8 unitId = static_cast<quint8>(data[6]);

        mbapInfo = QString(" [MBAP: 事务ID=%1, 协议ID=%2, 长度=%3, 单元ID=%4]")
                       .arg(transId).arg(protoId).arg(len).arg(unitId);
    }

    return QString("[%1] %2：%3%4")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
        .arg(prefix)
        .arg(hexStr)
        .arg(mbapInfo);
}

void ModbusTcpWidget::onReadCoils()
{
    if (m_tcpSocket->state() != QTcpSocket::ConnectedState) {
        ui->textEdit_log->append(QString("[%1] 错误：请先连接Modbus服务器")
                                     .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
        return;
    }

    int addr = ui->spinBox_addr->value();
    int count = ui->spinBox_count->value();

    QByteArray request = buildReadCoilsRequest(addr, count);
    qint64 bytesWritten = m_tcpSocket->write(request);

    if (bytesWritten > 0) {
        ui->textEdit_log->append(formatModbusMessage(request, true));
        ui->textEdit_log->append(QString("[%1] 发送读取线圈请求：起始地址%2, 数量%3")
                                     .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                                     .arg(addr).arg(count));
    } else {
        ui->textEdit_log->append(QString("[%1] 发送请求失败：%2")
                                     .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                                     .arg(m_tcpSocket->errorString()));
    }
}

void ModbusTcpWidget::onWriteCoil()
{
    if (m_tcpSocket->state() != QTcpSocket::ConnectedState) {
        ui->textEdit_log->append(QString("[%1] 错误：请先连接Modbus服务器")
                                     .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
        return;
    }

    int addr = ui->spinBox_addr->value();
    QString valueStr = ui->lineEdit_value->text().trimmed();

    if (valueStr != "0" && valueStr != "1") {
        QMessageBox::warning(this, "输入错误", "线圈值只能是0或1！");
        return;
    }

    int value = valueStr.toInt();
    QByteArray request = buildWriteCoilRequest(addr, value);
    qint64 bytesWritten = m_tcpSocket->write(request);

    if (bytesWritten > 0) {
        ui->textEdit_log->append(formatModbusMessage(request, true));
        ui->textEdit_log->append(QString("[%1] 发送写入线圈请求：地址%2, 值%3")
                                     .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                                     .arg(addr).arg(value));
    } else {
        ui->textEdit_log->append(QString("[%1] 发送请求失败：%2")
                                     .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                                     .arg(m_tcpSocket->errorString()));
    }
}

void ModbusTcpWidget::onReadHoldingRegisters()
{
    if (m_tcpSocket->state() != QTcpSocket::ConnectedState) {
        ui->textEdit_log->append(QString("[%1] 错误：请先连接Modbus服务器")
                                     .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
        return;
    }

    int addr = ui->spinBox_addr->value();
    int count = ui->spinBox_count->value();

    DataType dataType = static_cast<DataType>(ui->comboBox_dataType->currentData().toInt());
    if (dataType == Float32 && count % 2 != 0) {
        count++;
        ui->spinBox_count->setValue(count);
        ui->textEdit_log->append(QString("[%1] 提示：浮点数需要偶数个寄存器，已自动调整数量为%2")
                                     .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                                     .arg(count));
    }

    QByteArray request = buildReadHoldingRegistersRequest(addr, count);
    qint64 bytesWritten = m_tcpSocket->write(request);

    if (bytesWritten > 0) {
        ui->textEdit_log->append(formatModbusMessage(request, true));
        ui->textEdit_log->append(QString("[%1] 发送读取寄存器请求：起始地址%2, 数量%3, 数据类型%4")
                                     .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                                     .arg(addr).arg(count)
                                     .arg(ui->comboBox_dataType->currentText()));
    } else {
        ui->textEdit_log->append(QString("[%1] 发送请求失败：%2")
                                     .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                                     .arg(m_tcpSocket->errorString()));
    }
}

void ModbusTcpWidget::onWriteHoldingRegister()
{
    if (m_tcpSocket->state() != QTcpSocket::ConnectedState) {
        ui->textEdit_log->append(QString("[%1] 错误：请先连接Modbus服务器")
                                     .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
        return;
    }

    int addr = ui->spinBox_addr->value();
    QString valueStr = ui->lineEdit_value->text().trimmed();

    bool ok;
    QByteArray request;
    DataType dataType = static_cast<DataType>(ui->comboBox_dataType->currentData().toInt());

    if (dataType == Int16) {
        int value = valueStr.toInt(&ok);
        if (!ok || value < -32768 || value > 65535) {
            QMessageBox::warning(this, "输入错误", "请输入有效的16位整数（-32768~65535）！");
            return;
        }
        request = buildWriteHoldingRegisterRequest(addr, value);
    } else if (dataType == Float32) {
        float value = valueStr.toFloat(&ok);
        if (!ok) {
            QMessageBox::warning(this, "输入错误", "请输入有效的浮点数！");
            return;
        }
        request = buildWriteHoldingRegisterRequestFloat(addr, value);
    }

    qint64 bytesWritten = m_tcpSocket->write(request);

    if (bytesWritten > 0) {
        ui->textEdit_log->append(formatModbusMessage(request, true));
        ui->textEdit_log->append(QString("[%1] 发送写入寄存器请求：地址%2, 值%3, 数据类型%4")
                                     .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                                     .arg(addr).arg(valueStr)
                                     .arg(ui->comboBox_dataType->currentText()));
    } else {
        ui->textEdit_log->append(QString("[%1] 发送请求失败：%2")
                                     .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                                     .arg(m_tcpSocket->errorString()));
    }
}

void ModbusTcpWidget::onReadyRead()
{
    QByteArray response = m_tcpSocket->readAll();
    if (response.isEmpty()) return;

    ui->textEdit_log->append(formatModbusMessage(response, false));

    if (response.size() > 7) {
        quint8 funcCode = static_cast<quint8>(response[7]);

        switch (funcCode) {
        case 0x01:
            parseReadCoilsResponse(response);
            break;

        case 0x05:
            ui->textEdit_log->append(QString("[%1] 写入线圈成功")
                                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
            break;

        case 0x03:
            parseReadHoldingRegistersResponse(response);
            break;

        case 0x06:
            ui->textEdit_log->append(QString("[%1] 写入寄存器成功")
                                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
            break;

        case 0x81:
            parseReadCoilsResponse(response);
            break;

        case 0x83:
            parseReadHoldingRegistersResponse(response);
            break;

        default:
            ui->textEdit_log->append(QString("[%1] 收到未知功能码响应：%2")
                                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                                         .arg(funcCode));
        }
    }
}

void ModbusTcpWidget::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    ui->textEdit_log->append(QString("[%1] 网络错误：%2")
                                 .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                                 .arg(m_tcpSocket->errorString()));
}

void ModbusTcpWidget::onClearLog()
{
    ui->textEdit_log->clear();
    ui->textEdit_log->append(QString("[%1] 日志已清空").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
}

