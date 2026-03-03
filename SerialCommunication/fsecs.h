#ifndef FSECS_H
#define FSECS_H

#include <QWidget>
#include <QTimer>
#include <QByteArray>
#include <QDateTime>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QtEndian>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QMutex>
#include <QElapsedTimer>

#define SAFE_DELETE_PTR(ptr) if(ptr) { delete ptr; ptr = nullptr; }
#define SAFE_CAST_TO_UINT8(val) static_cast<quint8>(qBound(0, val, 255))
#define SAFE_CAST_TO_UINT16(val) static_cast<quint16>(qBound(0, val, 65535))


#define FRAME_HEADER 0xAA55
#define FRAME_TAIL   0x55AA


#define MIN_CORE_DATA_LEN 11
#define FULL_FRAME_LEN (2 + MIN_CORE_DATA_LEN + 2)

struct SemiconductorProtocolData {
    quint8 deviceId;
    quint8 command;
    quint16 parameter;
    quint8 status;
    quint32 timestamp;
    quint16 checksum;
};

struct DeviceThreshold {
    quint16 normalMax;
    quint16 warnMax;
    quint16 faultMax;
};

class fsecs : public QWidget
{
    Q_OBJECT

public:
    explicit fsecs(QWidget *parent = nullptr);
    ~fsecs() override;

private slots:
    void on_sendBtn_clicked();
    void on_receiveBtn_clicked();
    void updateDeviceStatus();
    void on_manualSetStatusBtn_clicked();
    void on_resetFaultBtn_clicked();
    void on_autoUpdateCheckBox_clicked();
    void updateRealTimeParam();
    void on_serialPortChanged(int index);
    void on_openSerialBtn_clicked();
    void on_serialReadyRead();
    void parseSerialFrame();
    void on_serialErrorOccurred(QSerialPort::SerialPortError error); // 新增：串口错误处理

private:
    // UI控件
    QSpinBox *deviceIdSpinBox = nullptr;
    QComboBox *commandComboBox = nullptr;
    QSpinBox *parameterSpinBox = nullptr;
    QComboBox *statusComboBox = nullptr;
    QPushButton *sendBtn = nullptr;
    QPushButton *receiveBtn = nullptr;
    QTextEdit *logTextEdit = nullptr;
    QLabel *parsedDeviceIdLabel = nullptr;
    QLabel *parsedStatusLabel = nullptr;
    QLabel *parsedParamLabel = nullptr;
    QLabel *deviceStatusLabel = nullptr;
    QLabel *realTimeParamLabel = nullptr;
    QPushButton *manualSetStatusBtn = nullptr;
    QPushButton *resetFaultBtn = nullptr;
    QCheckBox *autoUpdateCheckBox = nullptr;
    QSpinBox *thresholdWarnSpin = nullptr;
    QSpinBox *thresholdFaultSpin = nullptr;
    QComboBox *serialPortComboBox = nullptr;
    QComboBox *baudRateComboBox = nullptr;
    QPushButton *openSerialBtn = nullptr;


    QTimer *statusTimer = nullptr;
    QTimer *paramTimer = nullptr;
    QTimer *frameTimeoutTimer = nullptr;
    int currentStatus = 0;
    int statusDuration = 0;
    quint16 currentParamValue = 50;
    DeviceThreshold paramThreshold;
    bool isAutoUpdate = true;
    QSerialPort *serialPort = nullptr;
    QByteArray serialRecvBuffer;
    QMutex bufferMutex;


    QByteArray packProtocolData(const SemiconductorProtocolData &data);
    bool unpackProtocolData(const QByteArray &data, SemiconductorProtocolData &outData);
    quint16 calculateChecksum(const QByteArray &data);
    void logMessage(const QString &msg);
    void updateStatusByParam();
    void setDeviceStatus(int newStatus);
    void createUI();
    void refreshSerialPortList();

    bool isSerialPortValid();
    void clearSerialBufferSafely();
};

#endif
