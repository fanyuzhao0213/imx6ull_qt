#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QFile>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtVirtualKeyboard/QtVirtualKeyboard>

int main(int argc, char *argv[])
{


    // **1. 启用虚拟键盘模块**
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

    // **2. 加载虚拟键盘的输入法**
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);


    QApplication a(argc, argv);

    QFile file(":/style.qss");
    if(file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&file);
        QString qss = in.readAll();
        qDebug() << "QSS length =" << qss.length();
        qApp->setStyleSheet(qss);
    }

    MainWindow w;
    w.show();
    return a.exec();
}
