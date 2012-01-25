#include <QApplication>
#include <QtCrypto>
#include <QHostInfo>
#include <QSettings>

#include "mainwindow.h"
#include "main.h"

QString hostName;


int main(int argv, char **args)
{
  hostName = QHostInfo::localHostName();
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QCoreApplication::setOrganizationName("MarkReiche");
  QCoreApplication::setApplicationName("SyncBox");

  QApplication app(argv, args);
  QCA::Initializer init;
  if (!QCA::isSupported("aes256-ofb")) {
    qDebug("This application requires QCA support of AES256 in OFB mode!");
    return -1;
  }
  MainWindow mainwindow;
  mainwindow.show();

  return app.exec();
}
