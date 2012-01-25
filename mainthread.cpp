#include "mainthread.h"
#include "mainwindow.h"

#include "webdav/webdav.h"
#include "webdav/webdav_url_info.h"
#include <QBuffer>
#include <QInputDialog>
#include <QEventLoop>
#include <QSettings>

MainThread::MainThread(MainWindow* _mainwindow) : cipher(QString("aes256"),QCA::Cipher::OFB) {
  mainwindow = _mainwindow;
  webdav = new QWebdav(this);
  connect(webdav, SIGNAL(sslErrors( const QList<QSslError> &)), webdav, SLOT(ignoreSslErrors()));
  connect(webdav, SIGNAL(authenticationRequired(const QString &, quint16,QAuthenticator *)),
         mainwindow, SLOT(authenticationRequired(const QString &, quint16, QAuthenticator *)));
  connect(webdav, SIGNAL(stateChanged(int)), mainwindow, SLOT(stateChanged(int)));
  connect(webdav, SIGNAL(requestFinished(int, bool)), this, SLOT(requestFinished(int, bool)));
  connect(webdav, SIGNAL(listInfo(const QWebdavUrlInfo&)), this, SLOT(listInfo(const QWebdavUrlInfo&)));
  connect(webdav, SIGNAL(dataReadProgress ( int, int )), this, SLOT(fortschritt(int, int)));
  connect(webdav, SIGNAL(dataSendProgress ( int, int )), this, SLOT(fortschritt(int, int)));

  QSettings settings;
  webdavHost = settings.value("webdavHost").toString();
  webdavUser = settings.value("webdavUser").toString();
  webdavPassword = settings.value("webdavPassword").toString().toUtf8();
  QCA::SecureArray cipherPassword = settings.value("cipherPassword").toString().toUtf8();
  
  bool ok=true;
  while (webdavHost.isEmpty() || !ok) 
    webdavHost = QInputDialog::getText(mainwindow, "SyncBox", "WebDAV Host", QLineEdit::Normal, webdavHost, &ok);  
  while (webdavUser.isEmpty() || !ok) 
    webdavUser = QInputDialog::getText(mainwindow, "SyncBox", "WebDAV User", QLineEdit::Normal, webdavUser, &ok);  
  while (webdavPassword.isEmpty() || !ok) 
    webdavPassword = QInputDialog::getText(mainwindow, "SyncBox", "WebDAV Password", QLineEdit::Password, "", &ok).toUtf8();  
  while (cipherPassword.isEmpty() || !ok) 
    cipherPassword = QInputDialog::getText(mainwindow, "SyncBox", "Cipher Password", QLineEdit::Password, "", &ok).toUtf8();  

  settings.setValue("webdavHost", webdavHost);
  settings.setValue("webdavUser", webdavUser);
  settings.sync();
  
  key = QCA::SymmetricKey(cipherPassword);
  webdav->setUser(webdavUser, QString::fromUtf8(webdavPassword.constData()));
  webdav->setHost(webdavHost, QHttp::ConnectionModeHttps, 443);
}

void MainThread::fortschritt(int done, int total) {
  emit dataProgress(done, total);
}

void MainThread::fortschritt(qint64 done, qint64 total) {
  emit dataProgress(done, total);
}


void MainThread::log(const QString& string) {
  qDebug("Log: %s", string.toLatin1().constData());
  emit logSignal(string);
}

QByteArray MainThread::encrypt(const QByteArray& iv, const QByteArray &data) {
  cipher.setup(QCA::Encode, key, QCA::InitializationVector(iv));
  return cipher.process(data).toByteArray();
}

QByteArray MainThread::decrypt(const QByteArray& iv, const QByteArray &data) {
  cipher.setup(QCA::Decode, key, QCA::InitializationVector(iv));
  return cipher.process(data).toByteArray();
}

bool MainThread::exists(const QString& url) {
  return !listDir(url).isEmpty();
}

const QList<QWebdavUrlInfo>& MainThread::listDir(const QString& url) {
  QEventLoop *em = new QEventLoop();
  infoList.clear();
  int id = webdav->list(url);
  eventManagers[id]=em;
  em->exec();
  eventManagers.remove(id);
  delete em;
  return infoList;
}


void MainThread::requestFinished(int id, bool error) {
  if (error) emit logSignal(QString("Request finished %1 %2").arg(id).arg(error?"failure":"ok"));
  qDebug("request finished %d %s %d", id, error?"failure":"ok", webdav->error());
  if (error) qDebug("%s", webdav->errorString().toLatin1().constData());
  if (eventManagers.contains(id)) {
    eventManagers[id]->quit();
  }  
}

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

QByteArray MainThread::download(const QString& path) {
  qDebug("download %s", path.toLatin1().constData());
  /*QBuffer *buffer = new QBuffer();
  QEventLoop *em = new QEventLoop();
  buffer->open(QBuffer::ReadWrite);
  int id = webdav->get(url, buffer);
  eventManagers[id]=em;
  em->exec();
  eventManagers.remove(id);
  delete em;
  return buffer;*/
  QNetworkAccessManager nam;
  QEventLoop ev;
  connect(&nam, SIGNAL(finished(QNetworkReply*)), &ev, SLOT(quit()));
  QUrl url;
  url.setHost(webdavHost);
  url.setScheme("https");
  url.setUserName(webdavUser);
  url.setPassword(QString::fromUtf8(webdavPassword.constData()));
  url.setPath(path);
  QNetworkReply *rep = nam.get(QNetworkRequest(url));
  connect(rep, SIGNAL(downloadProgress ( qint64, qint64 )),
          this, SLOT(fortschritt(qint64, qint64)));
  ev.exec();
  QByteArray data = rep->readAll();
  delete rep;
  return data;
}

void MainThread::upload(const QString& url, const QByteArray& data) {
  QEventLoop *em = new QEventLoop();
  int id = webdav->put(url, data);
  eventManagers[id]=em;
  em->exec();
  eventManagers.remove(id);
  delete em;
}

void MainThread::listInfo(const QWebdavUrlInfo & url)
{                                                                                                                                                                                             
  infoList << url;
}                                                                                                                                                                                             



void MainThread::mkdir(const QString& url) {
  QEventLoop *em = new QEventLoop();
  int id = webdav->mkdir(url);
  eventManagers[id]=em;
  emit logSignal(QString("Mkdir %1 %2").arg(id).arg(url));
  em->exec();
  eventManagers.remove(id);
  delete em;
}

void MainThread::remove(const QString& url) {
  QEventLoop *em = new QEventLoop();
  int id = webdav->remove(url);
  eventManagers[id]=em;
  emit logSignal(QString("Remove %1 %2").arg(id).arg(url));
  em->exec();
  eventManagers.remove(id);
  delete em;
}
