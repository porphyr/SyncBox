#ifndef INOTIFY_H
#define INOTIFY_H

#include <QMap>
#include <QString>

class Archive;
class NotificationQueue;
class QThread;

class INotify {
public:
    INotify(Archive *theArchive);
    void addWatch(QString path);
    void rmWatch(QString path);
    void start();
    void stop();
    void watchLoop();
private:
    int fd;
    QMap<int, QString> watches;
    char* buf;
    NotificationQueue* thread;
    QThread* watchThread;
};


#endif