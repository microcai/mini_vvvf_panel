

#include "mainwindow.hpp"
#include <QMessageBox>
#include "awaitable.hpp"
#include <QDebug>

bool operator < (const QSerialPortInfo& a, const QSerialPortInfo& b)
{
    return a.portName() < b.portName();
}

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
    , ports_group(this)
    , baud_rate_group(this)
{
    setupUi(this);

    connect(tab, SIGNAL(set_target(float)), &vfd_ctrl, SLOT(set_target(float)));

    connect(&vfd_ctrl, SIGNAL(uart_list(QList<QSerialPortInfo>)), this, SLOT(update_menuC(QList<QSerialPortInfo>)));

    connect(menu_C, SIGNAL(aboutToShow()), &vfd_ctrl, SLOT(update_uart()));

    connect(&ports_group, SIGNAL(triggered(QAction*)), this, SLOT(SelectPort(QAction*)));

    connect(this, SIGNAL(RequrestPort(QSerialPortInfo)), &vfd_ctrl, SLOT(OpenPort(QSerialPortInfo)));

    connect(&vfd_ctrl, SIGNAL(vfd_info_update(float, float, int, char)), tab, SLOT(update_vfd_info(float, float, int, char)));

    // 波特率菜单项设置
    baud_rate_group.addAction(action_9600);
    baud_rate_group.addAction(action_19200);
    baud_rate_group.addAction(action_38400);
    baud_rate_group.addAction(action_57600);
    baud_rate_group.addAction(action_115200);
    baud_rate_group.addAction(action_230400);
    baud_rate_group.setExclusive(true);

    connect(&baud_rate_group, SIGNAL(triggered(QAction*)), this, SLOT(SelectBaudRate(QAction*)));

    // 默认选中 115200
    action_115200->setChecked(true);
}

MainWindow::~MainWindow(){}

void MainWindow::on_action_About_triggered()
{
    QMessageBox::about(this, "关于..", "本工具 DR 出品。");
}

void MainWindow::on_action_About_Qt_triggered()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::SelectPort(QAction* a)
{
    for (auto it = uart_ports_menu.begin(); it != uart_ports_menu.end(); it++)
    {
        if (a == it.value())
        {
            Q_EMIT RequrestPort(it.key());
            return;
        }
    }
}

void MainWindow::SelectBaudRate(QAction* a)
{
    if (a == action_9600)
    {
        vfd_ctrl.setBaudRate(9600);
    }
    else if (a == action_19200)
    {
        vfd_ctrl.setBaudRate(19200);
    }
    else if (a == action_38400)
    {
        vfd_ctrl.setBaudRate(38400);
    }
    else if (a == action_57600)
    {
        vfd_ctrl.setBaudRate(57600);
    }
    else if (a == action_115200)
    {
        vfd_ctrl.setBaudRate(115200);
    }
    else if (a == action_230400)
    {
        vfd_ctrl.setBaudRate(230400);
    }
}

void MainWindow::update_menuC(QList<QSerialPortInfo> info)
{
    for (auto it = uart_ports_menu.begin(); it != uart_ports_menu.end(); it++)
    {
        // 清理掉消失的 port.
    }

    for (auto & p: info)
    {
        qDebug() << p.portName();

        if (uart_ports_menu.find(p) == uart_ports_menu.end())
        {
            auto new_menu_action = new QAction(p.portName());
            new_menu_action->setCheckable(true);
            ports_group.addAction(new_menu_action);
            uart_ports_menu.insert(p, new_menu_action);
            menu_C->addAction(new_menu_action);
        }
    }
    // menu_C->addMenu()
}

void MainWindow::on_action_Use_Bluetooth_Connection_triggered()
{
    // 这里将打开蓝牙连接的模态对话框
    // 对话框内容将在后续实现
    QMessageBox::information(this, "蓝牙连接", "蓝牙连接对话框将在这里打开");
}
