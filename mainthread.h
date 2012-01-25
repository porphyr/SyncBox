#ifndef MAINTHREAD_H
#define MAINTHREAD_H

#include <QThread>

#include <QHash>
#include <QList>
#include <QtCrypto>
#include "webdav/webdav_url_info.h"

class MainWindow;
class QBuffer;
class QWebdav;
class QEventLoop;
class Archive;
class MyRoot;

class MainThread : public QObject {
Q_OBJECT
public:
  MainThread(MainWindow *_mainwindow);

    
  QByteArray download(const QString& path);
  void upload(const QString& url, const QByteArray& data);
  void mkdir(const QString& url);
  void remove(const QString& url);
  bool exists(const QString& url);
  const QList<QWebdavUrlInfo>& listDir(const QString& url);
  void log(const QString& string);
  
  QByteArray encrypt(const QByteArray& iv, const QByteArray &data);
  QByteArray decrypt(const QByteArray& iv, const QByteArray &data);
 
  //QByteArray hash(const QByteArray& data) { return hash.hash(data); };
   
private slots:
  void requestFinished(int id, bool error);
  void listInfo ( const QWebdavUrlInfo & i );
  void fortschritt(int done, int total);
  void fortschritt(qint64 done, qint64 total);
    
signals:
  void logSignal(const QString& string);
  void dataProgress(int done, int total);

private:
  
  MainWindow *mainwindow;
  QWebdav *webdav;
  QHash<int, QEventLoop*> eventManagers;
  QList<QWebdavUrlInfo> infoList;
  QString webdavHost;
  QString webdavUser;
  
  QCA::SecureArray webdavPassword;
  QCA::Cipher cipher;
  QCA::SymmetricKey key;
  //QCA::Hash hash;
};


#endif


