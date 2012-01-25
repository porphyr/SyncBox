#ifndef NOTIFICATIONQUEUE_H
#define NOTIFICATIONQUEUE_H

#include <QThread>
#include <QMutex>
#include <QMap>
#include <QDateTime>

class Archive;


class NotificationQueue: public QThread {
private:
  QMutex mutex;
  QMap<QString,QDateTime> updateQueue;
  Archive *archive;
  bool stopping;
public:
  NotificationQueue(Archive* theArchive);
  void run();
  void stop() { stopping = true; };
  
  void notifyLocalDeletion(const QString& localPath, const QString& name);
  void notifyLocalModification(const QString& localPath, const QString& name);
};

#endif