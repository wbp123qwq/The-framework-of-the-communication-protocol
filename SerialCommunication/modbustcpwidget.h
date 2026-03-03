#ifndef MODBUSTCPWIDGET_H
#define MODBUSTCPWIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include <QByteArray>
#include <QDateTime>

namespace Ui {
class ModbusTcpWidget;
}

class ModbusTcpWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ModbusTcpWidget(QWidget *parent = nullptr);
    ~ModbusTcpWidget();

    enum DataType {
        Int16 = 0,
        Float32 = 1
    };

private slots:
    void onConnect();
    void onReadCoils();
    void onWriteCoil();
    void onReadHoldingRegisters();
    void onWriteHoldingRegister();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onClearLog();
private:
    QByteArray buildReadCoilsRequest(int addr, int count);
    QByteArray buildWriteCoilRequest(int addr, int value);
    QByteArray buildReadHoldingRegistersRequest(int addr, int count);
    QByteArray buildWriteHoldingRegisterRequest(int addr, int value);
    QByteArray buildWriteHoldingRegisterRequestFloat(int addr, float value);

    void parseReadCoilsResponse(const QByteArray &response);
    void parseReadHoldingRegistersResponse(const QByteArray &response);

    quint16 getNextTransactionId();
    float bytesToFloat(const QByteArray &data);
    QByteArray floatToBytes(float value);
    QString formatModbusMessage(const QByteArray &data, bool isRequest = true);

private:
    Ui::ModbusTcpWidget *ui;
    QTcpSocket *m_tcpSocket;
    quint16 m_transactionId;
};

#endif // MODBUSTCPWIDGET_H
