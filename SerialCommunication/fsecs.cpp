#include "fsecs.h"
#include <QRandomGenerator>
#include <QDebug>
#include <QDataStream>
#include <QFont>
#include <QApplication>

fsecs::fsecs(QWidget *parent)
    : QWidget(parent)
{

    paramThreshold.normalMax = 80;
    paramThreshold.warnMax = 90;
    paramThreshold.faultMax = 100;


    statusTimer = new QTimer(this);
    paramTimer = new QTimer(this);
    frameTimeoutTimer = new QTimer(this);
    frameTimeoutTimer->setSingleShot(true);
    frameTimeoutTimer->setInterval(500);


    serialPort = new QSerialPort(this);


    createUI();


    statusTimer->setInterval(5000);
    connect(statusTimer, &QTimer::timeout, this, &fsecs::updateDeviceStatus);
    statusTimer->start();

    paramTimer->setInterval(1000);
    connect(paramTimer, &QTimer::timeout, this, &fsecs::updateRealTimeParam);
    paramTimer->start();

    connect(serialPort, &QSerialPort::readyRead, this, &fsecs::on_serialReadyRead);
    connect(serialPort, &QSerialPort::errorOccurred, this, &fsecs::on_serialErrorOccurred);
    connect(frameTimeoutTimer, &QTimer::timeout, this, &fsecs::clearSerialBufferSafely);


    logMessage("FSECS半导体控制系统初始化完成");
    logMessage(QString("初始参数阈值 - 警告: %1, 故障: %2").arg(paramThreshold.warnMax).arg(paramThreshold.faultMax));
}

fsecs::~fsecs()
{

    if (statusTimer) statusTimer->stop();
    if (paramTimer) paramTimer->stop();
    if (frameTimeoutTimer) frameTimeoutTimer->stop();


    if (serialPort && serialPort->isOpen()) {
        serialPort->close();
    }


    clearSerialBufferSafely();


}

void fsecs::createUI()
{
    this->setWindowTitle("半导体FSECS协议交互界面（稳定版）");
    this->resize(1000, 700);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);


    QGroupBox *serialGroup = new QGroupBox("串口配置", this);
    QHBoxLayout *serialLayout = new QHBoxLayout(serialGroup);
    serialLayout->setContentsMargins(10, 10, 10, 10);

    serialLayout->addWidget(new QLabel("串口端口:", this));
    serialPortComboBox = new QComboBox(this);
    refreshSerialPortList();
    serialLayout->addWidget(serialPortComboBox);
    connect(serialPortComboBox, &QComboBox::currentIndexChanged, this, &fsecs::on_serialPortChanged);

    serialLayout->addWidget(new QLabel("波特率:", this));
    baudRateComboBox = new QComboBox(this);
    baudRateComboBox->addItems({"9600", "19200", "38400", "57600", "115200"});
    baudRateComboBox->setCurrentText("9600");
    serialLayout->addWidget(baudRateComboBox);

    openSerialBtn = new QPushButton("打开串口", this);
    openSerialBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; }");
    connect(openSerialBtn, &QPushButton::clicked, this, &fsecs::on_openSerialBtn_clicked);
    serialLayout->addWidget(openSerialBtn);

    serialLayout->addStretch();
    mainLayout->addWidget(serialGroup);

    QGroupBox *thresholdGroup = new QGroupBox("参数阈值配置（温度℃）", this);
    QHBoxLayout *thresholdLayout = new QHBoxLayout(thresholdGroup);
    thresholdLayout->setContentsMargins(10, 10, 10, 10);

    thresholdLayout->addWidget(new QLabel("警告阈值:", this));
    thresholdWarnSpin = new QSpinBox(this);
    thresholdWarnSpin->setRange(50, 99);
    thresholdWarnSpin->setValue(paramThreshold.warnMax);
    connect(thresholdWarnSpin, &QSpinBox::valueChanged, this, [=](int value) {
        paramThreshold.warnMax = SAFE_CAST_TO_UINT16(value);
        logMessage(QString("警告阈值更新为: %1℃").arg(value));
    });
    thresholdLayout->addWidget(thresholdWarnSpin);

    thresholdLayout->addWidget(new QLabel("故障阈值:", this));
    thresholdFaultSpin = new QSpinBox(this);
    thresholdFaultSpin->setRange(60, 150);
    thresholdFaultSpin->setValue(paramThreshold.faultMax);
    connect(thresholdFaultSpin, &QSpinBox::valueChanged, this, [=](int value) {
        paramThreshold.faultMax = SAFE_CAST_TO_UINT16(value);
        logMessage(QString("故障阈值更新为: %1℃").arg(value));
    });
    thresholdLayout->addWidget(thresholdFaultSpin);
    autoUpdateCheckBox = new QCheckBox("启用自动状态更新", this);
    autoUpdateCheckBox->setChecked(isAutoUpdate);
    connect(autoUpdateCheckBox, &QCheckBox::clicked, this, &fsecs::on_autoUpdateCheckBox_clicked);
    thresholdLayout->addWidget(autoUpdateCheckBox);

    thresholdLayout->addStretch();
    mainLayout->addWidget(thresholdGroup);

    QGroupBox *sendGroup = new QGroupBox("协议数据发送", this);
    QGridLayout *sendLayout = new QGridLayout(sendGroup);
    sendLayout->setContentsMargins(10, 10, 10, 10);
    sendLayout->setSpacing(8);

    sendLayout->addWidget(new QLabel("设备ID:", this), 0, 0);
    deviceIdSpinBox = new QSpinBox(this);
    deviceIdSpinBox->setRange(1, 255);
    deviceIdSpinBox->setValue(1);
    sendLayout->addWidget(deviceIdSpinBox, 0, 1);

    sendLayout->addWidget(new QLabel("命令码:", this), 0, 2);
    commandComboBox = new QComboBox(this);
    commandComboBox->addItems({"读取状态(0x01)", "设置参数(0x02)", "报警(0x03)", "复位故障(0x04)"});
    sendLayout->addWidget(commandComboBox, 0, 3);

    sendLayout->addWidget(new QLabel("参数值(温度):", this), 1, 0);
    parameterSpinBox = new QSpinBox(this);
    parameterSpinBox->setRange(0, 65535);
    parameterSpinBox->setValue(currentParamValue);
    sendLayout->addWidget(parameterSpinBox, 1, 1);

    sendLayout->addWidget(new QLabel("设备状态:", this), 1, 2);
    statusComboBox = new QComboBox(this);
    statusComboBox->addItems({"正常(0)", "警告(1)", "故障(2)"});
    sendLayout->addWidget(statusComboBox, 1, 3);

    sendBtn = new QPushButton("发送数据（串口）", this);
    sendLayout->addWidget(sendBtn, 2, 0, 1, 2);
    connect(sendBtn, &QPushButton::clicked, this, &fsecs::on_sendBtn_clicked);

    receiveBtn = new QPushButton("模拟接收数据", this);
    sendLayout->addWidget(receiveBtn, 2, 2, 1, 2);
    connect(receiveBtn, &QPushButton::clicked, this, &fsecs::on_receiveBtn_clicked);

    mainLayout->addWidget(sendGroup);


    QGroupBox *controlGroup = new QGroupBox("设备控制", this);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlGroup);
    controlLayout->setContentsMargins(10, 10, 10, 10);

    manualSetStatusBtn = new QPushButton("手动设置状态", this);
    controlLayout->addWidget(manualSetStatusBtn);
    connect(manualSetStatusBtn, &QPushButton::clicked, this, &fsecs::on_manualSetStatusBtn_clicked);

    resetFaultBtn = new QPushButton("故障复位", this);
    resetFaultBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; }");
    controlLayout->addWidget(resetFaultBtn);
    connect(resetFaultBtn, &QPushButton::clicked, this, &fsecs::on_resetFaultBtn_clicked);

    controlLayout->addStretch();

    controlLayout->addWidget(new QLabel("实时温度:", this));
    realTimeParamLabel = new QLabel(QString::number(currentParamValue), this);
    realTimeParamLabel->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; }");
    controlLayout->addWidget(realTimeParamLabel);
    controlLayout->addWidget(new QLabel("℃", this));

    mainLayout->addWidget(controlGroup);


    QGroupBox *parseGroup = new QGroupBox("协议数据解析结果", this);
    QHBoxLayout *parseLayout = new QHBoxLayout(parseGroup);
    parseLayout->setContentsMargins(10, 10, 10, 10);

    parseLayout->addWidget(new QLabel("解析后设备ID:", this));
    parsedDeviceIdLabel = new QLabel("---", this);
    parsedDeviceIdLabel->setMinimumWidth(80);
    parseLayout->addWidget(parsedDeviceIdLabel);

    parseLayout->addWidget(new QLabel("解析后状态:", this));
    parsedStatusLabel = new QLabel("---", this);
    parsedStatusLabel->setMinimumWidth(80);
    parseLayout->addWidget(parsedStatusLabel);

    parseLayout->addWidget(new QLabel("解析后参数:", this));
    parsedParamLabel = new QLabel("---", this);
    parsedParamLabel->setMinimumWidth(80);
    parseLayout->addWidget(parsedParamLabel);

    parseLayout->addStretch();

    parseLayout->addWidget(new QLabel("实时设备状态:", this));
    deviceStatusLabel = new QLabel("正常", this);
    deviceStatusLabel->setStyleSheet("QLabel { color: green; font-weight: bold; font-size: 14px; }");
    parseLayout->addWidget(deviceStatusLabel);

    mainLayout->addWidget(parseGroup);

    QGroupBox *logGroup = new QGroupBox("系统日志", this);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    logLayout->setContentsMargins(10, 10, 10, 10);

    logTextEdit = new QTextEdit(this);
    logTextEdit->setReadOnly(true);
    logTextEdit->setFont(QFont("Consolas", 9));
    logLayout->addWidget(logTextEdit);

    mainLayout->addWidget(logGroup, 1);

    this->setLayout(mainLayout);
}

void fsecs::refreshSerialPortList()
{
    if (!serialPortComboBox) return;

    serialPortComboBox->clear();
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    if (portList.isEmpty()) {
        serialPortComboBox->addItem("无可用串口");
        if (openSerialBtn) {
            openSerialBtn->setEnabled(false);
            openSerialBtn->setToolTip("无可用串口");
        }
    } else {
        foreach (const QSerialPortInfo &info, portList) {
            serialPortComboBox->addItem(info.portName() + " (" + info.description() + ")", info.portName());
        }
        if (openSerialBtn) {
            openSerialBtn->setEnabled(true);
            openSerialBtn->setToolTip("");
        }
    }
}

bool fsecs::isSerialPortValid()
{
    if (!serialPort) return false;
    if (!serialPort->isOpen()) return false;
    if (serialPort->error() != QSerialPort::NoError) return false;
    return true;
}

void fsecs::clearSerialBufferSafely()
{
    QMutexLocker locker(&bufferMutex);
    serialRecvBuffer.clear();
}

void fsecs::on_serialPortChanged(int index)
{
    Q_UNUSED(index);
    if (isSerialPortValid()) {
        serialPort->close();
        if (openSerialBtn) {
            openSerialBtn->setText("打开串口");
            openSerialBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; }");
        }
        clearSerialBufferSafely();
        logMessage("串口已关闭（端口变更）");
    }
}

void fsecs::on_openSerialBtn_clicked()
{
    if (!serialPort || !serialPortComboBox || !baudRateComboBox || !openSerialBtn) {
        QMessageBox::critical(this, "错误", "UI控件初始化异常！");
        return;
    }

    if (serialPort->isOpen()) {

        serialPort->close();
        openSerialBtn->setText("打开串口");
        openSerialBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; }");
        clearSerialBufferSafely();
        logMessage("串口已关闭");
        return;
    }

    if (serialPortComboBox->count() == 0 || serialPortComboBox->currentText() == "无可用串口") {
        QMessageBox::warning(this, "错误", "无可用串口！");
        return;
    }


    QString portName = serialPortComboBox->currentData().toString();
    bool baudOk = false;
    qint32 baudRate = baudRateComboBox->currentText().toInt(&baudOk);
    if (!baudOk) {
        QMessageBox::warning(this, "错误", "波特率格式错误！");
        return;
    }


    serialPort->setPortName(portName);
    serialPort->setBaudRate(baudRate);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);


    if (serialPort->open(QIODevice::ReadWrite)) {
        openSerialBtn->setText("关闭串口");
        openSerialBtn->setStyleSheet("QPushButton { background-color: #f44336; color: white; }");
        clearSerialBufferSafely(); // 清空历史缓冲区
        logMessage(QString("串口打开成功 - 端口：%1，波特率：%2").arg(portName).arg(baudRate));
    } else {
        QString errorMsg = QString("串口打开失败：%1").arg(serialPort->errorString());
        QMessageBox::critical(this, "错误", errorMsg);
        logMessage(errorMsg);
    }
}

void fsecs::on_serialErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;


    if (serialPort && serialPort->isOpen()) {
        serialPort->close();
    }
    if (openSerialBtn) {
        openSerialBtn->setText("打开串口");
        openSerialBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; }");
    }
    clearSerialBufferSafely();
    logMessage(QString("串口异常：%1，已自动关闭").arg(serialPort->errorString()));
}

void fsecs::on_serialReadyRead()
{
    if (!serialPort || !serialPort->isOpen()) {
        clearSerialBufferSafely();
        return;
    }

    QByteArray data;
    {
        QMutexLocker locker(&bufferMutex);
        data = serialPort->readAll();
        if (data.isEmpty()) return;


        if (serialRecvBuffer.size() + data.size() > 1024) {
            logMessage("接收缓冲区过大，清空旧数据");
            serialRecvBuffer.clear();
        }

        serialRecvBuffer.append(data);
    }

    frameTimeoutTimer->start();

    logMessage(QString("串口接收原始数据（16进制）：%1").arg(QString(data.toHex(' '))));


    QMetaObject::invokeMethod(this, "parseSerialFrame", Qt::QueuedConnection);
}

void fsecs::parseSerialFrame()
{
    QMutexLocker locker(&bufferMutex);

    if (serialRecvBuffer.size() < FULL_FRAME_LEN) {
        return;
    }


    while (serialRecvBuffer.size() >= FULL_FRAME_LEN) {

        int headerPos = -1;
        int maxSearchPos = serialRecvBuffer.size() - 2;
        for (int i = 0; i <= maxSearchPos; i++) {

            if (i + 1 >= serialRecvBuffer.size()) break;
            quint16 header = qFromBigEndian(*reinterpret_cast<const quint16*>(serialRecvBuffer.constData() + i));
            if (header == FRAME_HEADER) {
                headerPos = i;
                break;
            }
        }

        if (headerPos == -1) {

            serialRecvBuffer.clear();
            logMessage("未找到帧头，清空接收缓冲区");
            return;
        }


        if (headerPos > 0) {
            logMessage(QString("跳过无效数据（长度：%1字节）").arg(headerPos));
            serialRecvBuffer.remove(0, headerPos);
            continue;
        }


        int tailPos = 2 + MIN_CORE_DATA_LEN;
        if (serialRecvBuffer.size() < tailPos + 2) {
            return; // 帧尾不完整，等待后续数据
        }


        quint16 tail = qFromBigEndian(*reinterpret_cast<const quint16*>(serialRecvBuffer.constData() + tailPos));
        if (tail != FRAME_TAIL) {

            static int badFrameCount = 0;
            badFrameCount++;
            if (badFrameCount > 100) {
                logMessage("连续无效帧过多，清空缓冲区");
                serialRecvBuffer.clear();
                badFrameCount = 0;
                return;
            }

            serialRecvBuffer.remove(0, 2);
            logMessage("帧尾不匹配，丢弃当前帧头继续解析");
            continue;
        }


        QByteArray coreData = serialRecvBuffer.mid(2, MIN_CORE_DATA_LEN);
        if (coreData.size() != MIN_CORE_DATA_LEN) {
            serialRecvBuffer.remove(0, FULL_FRAME_LEN);
            logMessage("核心数据长度异常，丢弃当前帧");
            continue;
        }


        serialRecvBuffer.remove(0, FULL_FRAME_LEN);


        locker.unlock();
        SemiconductorProtocolData parsedData;
        if (unpackProtocolData(coreData, parsedData)) {
            QString statusText = parsedData.status == 0 ? "正常" : (parsedData.status == 1 ? "警告" : "故障");
            logMessage(QString("串口解析有效帧 - 设备ID: %1, 状态: %2(%3), 参数: %4℃")
                           .arg(static_cast<int>(parsedData.deviceId))
                           .arg(statusText)
                           .arg(static_cast<int>(parsedData.status))
                           .arg(static_cast<int>(parsedData.parameter)));

            // 安全更新UI（主线程）
            QMetaObject::invokeMethod(this, "setDeviceStatus", Qt::QueuedConnection, Q_ARG(int, parsedData.status));
            if (parsedDeviceIdLabel) parsedDeviceIdLabel->setText(QString::number(static_cast<int>(parsedData.deviceId)));
            if (parsedStatusLabel) parsedStatusLabel->setText(statusText);
            if (parsedParamLabel) parsedParamLabel->setText(QString::number(static_cast<int>(parsedData.parameter)));
        } else {
            logMessage("串口帧数据解析失败（校验和错误）");
        }
        locker.relock();


        //badFrameCount = 0;
    }
}
void fsecs::on_sendBtn_clicked()
{
    if (!isSerialPortValid())
    {
        QMessageBox::warning(this, "警告", "请先打开有效串口！");
        return;
    }


    quint8 deviceId = 1;
    quint8 command = 1;
    quint16 parameter = 50;
    quint8 status = 0;

    if (deviceIdSpinBox) deviceId = SAFE_CAST_TO_UINT8(deviceIdSpinBox->value());
    if (commandComboBox) command = SAFE_CAST_TO_UINT8(commandComboBox->currentIndex() + 1);
    if (parameterSpinBox) parameter = SAFE_CAST_TO_UINT16(parameterSpinBox->value());
    if (statusComboBox) status = SAFE_CAST_TO_UINT8(statusComboBox->currentIndex());


    SemiconductorProtocolData data;
    data.deviceId = deviceId;
    data.command = command;
    data.parameter = parameter;
    data.status = status;
    data.timestamp = static_cast<quint32>(QDateTime::currentSecsSinceEpoch());


    QByteArray packedData = packProtocolData(data);
    if (packedData.isEmpty()) {
        logMessage("协议数据封装失败，发送取消");
        return;
    }


    qint64 sendLen = serialPort->write(packedData);
    if (sendLen == -1 || sendLen != packedData.size()) {
        logMessage(QString("串口发送失败：%1（发送长度：%2，期望：%3）")
                       .arg(serialPort->errorString()).arg(sendLen).arg(packedData.size()));
    } else {
        QString cmdName = commandComboBox ? commandComboBox->currentText().split("(").first() : "未知命令";
        logMessage(QString("串口发送成功（长度：%1字节） - 设备ID: %2, 命令: %3(0x%4), 参数: %5, 完整帧(16进制): %6")
                       .arg(sendLen)
                       .arg(static_cast<int>(data.deviceId))
                       .arg(cmdName)
                       .arg(static_cast<int>(data.command), 0, 16)
                       .arg(static_cast<int>(data.parameter))
                       .arg(QString(packedData.toHex(' '))));


        if (data.command == 0x02 && realTimeParamLabel) {
            currentParamValue = data.parameter;
            realTimeParamLabel->setText(QString::number(currentParamValue));
            logMessage(QString("手动设置参数值为: %1℃").arg(currentParamValue));
        }

        if (data.command == 0x04) {
            setDeviceStatus(0);
            logMessage("发送故障复位命令，设备状态恢复正常");
        }
    }
}

void fsecs::on_receiveBtn_clicked()
{

    SemiconductorProtocolData mockData;
    mockData.deviceId = deviceIdSpinBox ? SAFE_CAST_TO_UINT8(deviceIdSpinBox->value()) : 1;
    mockData.command = SAFE_CAST_TO_UINT8(QRandomGenerator::global()->bounded(1, 5));
    mockData.parameter = currentParamValue;
    mockData.status = SAFE_CAST_TO_UINT8(currentStatus);
    mockData.timestamp = static_cast<quint32>(QDateTime::currentSecsSinceEpoch());

    QByteArray packedData = packProtocolData(mockData);
    if (packedData.size() < FULL_FRAME_LEN) {
        logMessage("模拟数据封装失败");
        return;
    }

    SemiconductorProtocolData parsedData;
    if (unpackProtocolData(packedData.mid(2, MIN_CORE_DATA_LEN), parsedData)) {
        QString statusText = parsedData.status == 0 ? "正常" : (parsedData.status == 1 ? "警告" : "故障");
        logMessage(QString("模拟接收并解析数据成功 - 设备ID: %1, 状态: %2(%3), 参数: %4℃")
                       .arg(static_cast<int>(parsedData.deviceId))
                       .arg(statusText)
                       .arg(static_cast<int>(parsedData.status))
                       .arg(static_cast<int>(parsedData.parameter)));

        if (parsedDeviceIdLabel) parsedDeviceIdLabel->setText(QString::number(static_cast<int>(parsedData.deviceId)));
        if (parsedStatusLabel) parsedStatusLabel->setText(statusText);
        if (parsedParamLabel) parsedParamLabel->setText(QString::number(static_cast<int>(parsedData.parameter)));
    } else {
        logMessage("协议数据解析失败（校验和错误或格式错误）");
    }
}

void fsecs::on_autoUpdateCheckBox_clicked()
{
    if (!autoUpdateCheckBox) return;

    isAutoUpdate = autoUpdateCheckBox->isChecked();
    if (isAutoUpdate) {
        if (statusTimer) statusTimer->start();
        if (paramTimer) paramTimer->start();
        logMessage("启用自动状态和参数更新");
    } else {
        if (statusTimer) statusTimer->stop();
        if (paramTimer) paramTimer->stop();
        logMessage("禁用自动状态和参数更新");
    }
}

void fsecs::on_manualSetStatusBtn_clicked()
{
    if (!statusComboBox) return;

    int targetStatus = statusComboBox->currentIndex();
    setDeviceStatus(targetStatus);

    QString statusText = targetStatus == 0 ? "正常" : (targetStatus == 1 ? "警告" : "故障");
    logMessage(QString("手动设置设备状态为: %1").arg(statusText));
}

void fsecs::on_resetFaultBtn_clicked()
{
    if (currentStatus == 2) {
        setDeviceStatus(0);
        currentParamValue = paramThreshold.normalMax - 10;
        if (realTimeParamLabel) {
            realTimeParamLabel->setText(QString::number(currentParamValue));
        }
        logMessage("执行故障复位，设备状态恢复正常，参数重置为安全值");


        if (isSerialPortValid()) {
            SemiconductorProtocolData resetData;
            resetData.deviceId = deviceIdSpinBox ? SAFE_CAST_TO_UINT8(deviceIdSpinBox->value()) : 1;
            resetData.command = 0x04;
            resetData.parameter = 0;
            resetData.status = 0;
            resetData.timestamp = static_cast<quint32>(QDateTime::currentSecsSinceEpoch());

            QByteArray resetFrame = packProtocolData(resetData);
            serialPort->write(resetFrame);
            logMessage("串口发送故障复位命令");
        }
    } else {
        logMessage("当前设备未处于故障状态，无需复位");
    }
}

void fsecs::updateRealTimeParam()
{

    int delta = QRandomGenerator::global()->bounded(-3, 4);
    int newParam = static_cast<int>(currentParamValue) + delta;
    newParam = qBound(0, newParam, static_cast<int>(paramThreshold.faultMax) + 10);
    currentParamValue = SAFE_CAST_TO_UINT16(newParam);

    if (realTimeParamLabel) {
        realTimeParamLabel->setText(QString::number(currentParamValue));
    }

    updateStatusByParam();
}

void fsecs::updateStatusByParam()
{
    int newStatus = currentStatus;

    if (currentParamValue > paramThreshold.faultMax) {
        newStatus = 2;
        statusDuration++;
        if (statusDuration >= 2) {
            setDeviceStatus(2);
        }
    } else if (currentParamValue > paramThreshold.warnMax) {
        newStatus = 1;
        if (currentStatus != 1) {
            setDeviceStatus(1);
        }
        statusDuration = 0;
    } else if (currentParamValue <= paramThreshold.normalMax) {
        newStatus = 0;
        if (currentStatus != 0) {
            setDeviceStatus(0);
        }
        statusDuration = 0;
    }
}

void fsecs::updateDeviceStatus()
{
    if (!isAutoUpdate) return;

    int randValue = 0;
    switch (currentStatus) {
    case 0:
        if (QRandomGenerator::global()->bounded(100) < 5) {
            currentParamValue += 5;
            updateStatusByParam();
            logMessage("正常状态偶发异常，参数升高进入警告范围");
        }
        break;
    case 1:
        randValue = QRandomGenerator::global()->bounded(100);
        if (randValue < 20) {
            currentParamValue -= 5;
            updateStatusByParam();
            logMessage(QString("警告状态恢复正常，随机值：%1，参数降低至%2℃").arg(randValue).arg(currentParamValue));
        } else if (randValue >= 90) {
            currentParamValue += 5;
            updateStatusByParam();
            logMessage(QString("警告状态升级为故障，随机值：%1，参数升高至%2℃").arg(randValue).arg(currentParamValue));
        }
        break;
    case 2:
        logMessage("设备处于故障状态，需手动复位才能恢复");
        break;
    default:
        logMessage(QString("检测到未知设备状态：%1，重置为正常状态").arg(currentStatus));
        setDeviceStatus(0);
        currentParamValue = paramThreshold.normalMax - 10;
        updateStatusByParam();
        break;
    }
}

void fsecs::setDeviceStatus(int newStatus)
{

    newStatus = qBound(0, newStatus, 2);
    if (newStatus == currentStatus) return;

    currentStatus = newStatus;
    QString statusText;
    QString styleSheet;

    switch (currentStatus) {
    case 0:
        statusText = "正常";
        styleSheet = "QLabel { color: green; font-weight: bold; font-size: 14px; }";
        statusDuration = 0;
        break;
    case 1:
        statusText = "警告";
        styleSheet = "QLabel { color: orange; font-weight: bold; font-size: 14px; }";
        break;
    case 2:
        statusText = "故障";
        styleSheet = "QLabel { color: red; font-weight: bold; font-size: 14px; }";
        break;
    default:
        statusText = "未知";
        styleSheet = "QLabel { color: gray; font-weight: bold; font-size: 14px; }";
        break;
    }

    // 安全更新UI
    if (deviceStatusLabel) {
        deviceStatusLabel->setText(statusText);
        deviceStatusLabel->setStyleSheet(styleSheet);
    }
    if (statusComboBox) {
        statusComboBox->setCurrentIndex(currentStatus);
    }
}

QByteArray fsecs::packProtocolData(const SemiconductorProtocolData &data)
{
    QByteArray coreBuffer;
    QDataStream coreStream(&coreBuffer, QIODevice::WriteOnly);
    coreStream.setByteOrder(QDataStream::BigEndian);

    coreStream << data.deviceId;
    coreStream << data.command;
    coreStream << qToBigEndian(data.parameter);
    coreStream << data.status;
    coreStream << qToBigEndian(data.timestamp);


    quint16 checksum = calculateChecksum(coreBuffer);
    coreStream << qToBigEndian(checksum);


    if (coreBuffer.size() != MIN_CORE_DATA_LEN) {
        logMessage(QString("核心数据长度异常：%1（期望：%2）").arg(coreBuffer.size()).arg(MIN_CORE_DATA_LEN));
        return QByteArray();
    }


    QByteArray fullFrame;

    quint16 headerBE = qToBigEndian(static_cast<quint16>(FRAME_HEADER));
    fullFrame.append(reinterpret_cast<char*>(&headerBE), 2);

    fullFrame.append(coreBuffer);

    quint16 tailBE = qToBigEndian(static_cast<quint16>(FRAME_TAIL));
    fullFrame.append(reinterpret_cast<char*>(&tailBE), 2);
    return fullFrame;
}

bool fsecs::unpackProtocolData(const QByteArray &data, SemiconductorProtocolData &outData)
{

    if (data.size() != MIN_CORE_DATA_LEN) {
        logMessage(QString("协议数据长度不足，解析失败（当前长度：%1，期望：%2）").arg(data.size()).arg(MIN_CORE_DATA_LEN));
        return false;
    }

    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);

    QByteArray dataWithoutChecksum = data.left(data.size() - 2);

    stream >> outData.deviceId;
    stream >> outData.command;
    quint16 paramBE;
    stream >> paramBE;
    outData.parameter = qFromBigEndian(paramBE);
    stream >> outData.status;

    quint32 timestampBE;
    stream >> timestampBE;
    outData.timestamp = qFromBigEndian(timestampBE);


    quint16 receivedChecksumBE;
    stream >> receivedChecksumBE;
    quint16 receivedChecksum = qFromBigEndian(receivedChecksumBE);

    quint16 calculatedChecksum = calculateChecksum(dataWithoutChecksum);

    if (receivedChecksum != calculatedChecksum) {
        logMessage(QString("校验和不匹配 - 接收: 0x%1, 计算: 0x%2")
                       .arg(receivedChecksum, 4, 16, QChar('0'))
                       .arg(calculatedChecksum, 4, 16, QChar('0')));
        return false;
    }

    return true;
}

quint16 fsecs::calculateChecksum(const QByteArray &data)
{
    quint16 sum = 0;
    for (char c : data) {
        sum += static_cast<quint8>(c);

        sum &= 0xFFFF;
    }
    return sum;
}

void fsecs::logMessage(const QString &msg)
{
    if (!logTextEdit) return;


    QString log = QString("[%1] %2").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")).arg(msg);
    QMetaObject::invokeMethod(logTextEdit, "append", Qt::QueuedConnection, Q_ARG(QString, log));
}
