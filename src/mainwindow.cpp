

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
{
    setupUi(this);

    connect(tab, SIGNAL(set_target(float)), &vfd_ctrl, SLOT(set_target(float)));

    connect(&vfd_ctrl, SIGNAL(uart_list(QList<QSerialPortInfo>)), this, SLOT(update_menuC(QList<QSerialPortInfo>)));

    connect(menu_C, SIGNAL(aboutToShow()), &vfd_ctrl, SLOT(update_uart()));

    connect(&ports_group, SIGNAL(triggered(QAction*)), this, SLOT(SelectPort(QAction*)));

    connect(this, SIGNAL(RequrestPort(QSerialPortInfo)), &vfd_ctrl, SLOT(OpenPort(QSerialPortInfo)));

    connect(&vfd_ctrl, SIGNAL(vfd_info_update(float, float, int)), tab, SLOT(update_vfd_info(float, float, int)));
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
