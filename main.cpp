#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QFile>

int main(int argc, char *argv[])
{
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
