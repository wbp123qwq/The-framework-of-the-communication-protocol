#ifndef SERIALWIDGET_H
#define SERIALWIDGET_H

#include <QDateTime>
#include <QMessageBox>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QWidget>

namespace Ui {
class SerialWidget;
}

class SerialWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SerialWidget(QWidget *parent = nullptr);
    ~SerialWidget() override;

private slots:
    void onOpenSerial();
    void onSendData();
    void onReadyRead();
    void onRefreshPortList();
    void onSerialError(QSerialPort::SerialPortError error);

private:
    void initSerialParams();
    void updateButtonStates();
    void loadAvailablePorts();

    Ui::SerialWidget *ui;
    QSerialPort *m_serialPort;
    bool m_isSerialOpen = false;
};

#endif // SERIALWIDGET_H
