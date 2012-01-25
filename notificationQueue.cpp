#include "notificationQueue.h"
#include "archive.h"

#include <QDir>

NotificationQueue::NotificationQueue(Archive* theArchive) : archive(theArchive), stopping(false) {
}

void NotificationQueue::notifyLocalDeletion(const QString& localPath, const QString& name) {
  QString fullPath=QDir(localPath).filePath(name);
  mutex.lock();
  updateQueue.remove(fullPath);
  updateQueue["root"] = QDateTime::currentDateTime().addSecs(2);
  mutex.unlock();
  archive->handleLocalDeletion(fullPath);
}

void NotificationQueue::notifyLocalModification(const QString& localPath, const QString& name) {
  QString fullPath=QDir(localPath).filePath(name);
  mutex.lock();
  updateQueue[fullPath] = QDateTime::currentDateTime().addSecs(2);
  updateQueue["root"] = QDateTime::currentDateTime().addSecs(2);
  mutex.unlock();
}

void NotificationQueue::run() {
  while(!stopping) {
    QString foundPath="";
//    archive->checkForNewRootFile();
    mutex.lock();
    QDateTime now=QDateTime::currentDateTime();
    QMap<QString,QDateTime>::iterator i = updateQueue.begin();
    while (i != updateQueue.end() && foundPath == "") {
      if ( i.value() < now) {
	foundPath=i.key();
	updateQueue.erase(i);
      }
      ++i;
    }
    mutex.unlock();
    if (foundPath == "") sleep(2);
    else {	
      if (foundPath == "root") archive -> writeRootFile();
      else archive->handleLocalModification(foundPath);
    }
  }
}

