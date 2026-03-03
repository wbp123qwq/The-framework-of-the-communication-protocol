#include "widget.h"
#include "ui_widget.h"
#include <QVBoxLayout>
#include <QStyle>
#include <QSizePolicy>
#include <QTabBar>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    setWindowTitle("多协议通信测试工具");
    resize(1100, 700);
    setMinimumSize(900, 600);
    m_tabWidget = ui->m_tabWidget;
    m_tabWidget->tabBar()->setExpanding(true);
    m_tabWidget->tabBar()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_tabWidget->setStyleSheet(R"(
        QTabBar::tab {
            min-width: 100px;
            height: 40px;
            font-size: 14px;
        }
        QTabBar::tab:selected {
            background-color: #007bff;
            color: white;
        }
        QTabWidget::pane {
            border: 1px solid #ddd;
        }
        QTabWidget {
            border: none;
        }
    )");

    m_tcpWidget = new TcpWidget(this);
    m_udpWidget = new UdpWidget(this);
    m_serialWidget = new SerialWidget(this);
    m_modbusTcpWidget = new ModbusTcpWidget(this);
    m_fsecs=new fsecs(this);
    while (m_tabWidget->count() > 0) {
        m_tabWidget->removeTab(0);
    }
    m_tabWidget->addTab(m_tcpWidget, "TCP 通信");
    m_tabWidget->addTab(m_udpWidget, "UDP 通信");
    m_tabWidget->addTab(m_serialWidget, "RS232/RS485 通信");
    m_tabWidget->addTab(m_modbusTcpWidget, "ModbusTCP 通信");
    m_tabWidget->addTab(m_fsecs,"半导体协议");
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_tabWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    setLayout(mainLayout);
    m_tabWidget->setCurrentIndex(3);
}

Widget::~Widget()
{
    delete ui;
}
