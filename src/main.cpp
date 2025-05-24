
#include <QApplication>
#include <QFile>
#include <QDir>
#include "mainwindow.hpp"
#include <QDebug>

#include "vfd_ctl.hpp"

static const char desktop_file_content[] = R"ss([Desktop Entry]
Categories=Development;Electronics;
Comment=MiniVFD 的上位机
Icon=MiniVFD
Name=MiniVFD Panel
Type=Application
StartupWMClass=MiniVFD
Exec=)ss";

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    if (app.platformName() == "wayland")
    {
        // 解决 wayland 下 没 icon 的问题
        QFile iconfile(QDir::homePath() + "/.local/share/icons/MiniVFD.png");
        QFile resiconfile(":/images/res/logo.png");
        resiconfile.open(QIODeviceBase::ReadOnly);
        iconfile.open(QIODeviceBase::ReadWrite);

        iconfile.write(resiconfile.readAll());
        resiconfile.close(); iconfile.close();

        QFile desktopfile(QDir::homePath() + "/.local/share/applications/MiniVFD.desktop");
        if (desktopfile.open(QIODeviceBase::ReadWrite))
        {
            desktopfile.resize(sizeof (desktop_file_content));
            desktopfile.write(desktop_file_content, sizeof (desktop_file_content) - 1);
            desktopfile.write(app.applicationFilePath().toUtf8());
            desktopfile.write("\n", 1);
        }
        desktopfile.close();

        app.setDesktopFileName("edatool");
    }

    QIcon ico(":/images/res/logo.png");
    app.setWindowIcon(ico);

    VFDCtrl vfd_ctrl;

    MainWindow mywin;

    QObject::connect(mywin.tab, SIGNAL(set_target(float)), &vfd_ctrl, SLOT(set_target(float)));


    mywin.setWindowIcon(ico);

    mywin.show();

    return app.exec();
}
