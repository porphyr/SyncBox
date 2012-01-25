#include "inotify.h"

#include "notificationQueue.h"

#include <QFile>
#include <sys/inotify.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFSIZE ((sizeof(struct inotify_event)+FILENAME_MAX)*1024)


class WatchThread : public QThread {
public:
  INotify* inotify;
  void run() { inotify->watchLoop(); };
};

INotify::INotify(Archive *theArchive) : thread(new NotificationQueue(theArchive)) {
  fd = inotify_init();
  if (fd == -1) perror("Inotify_init");
  qDebug("Inotify Filedescriptor = %d", fd);
  buf = (char*)malloc(BUFSIZE);
  thread->start();   
}


void INotify::addWatch(QString path) {
  qDebug("Inotify Filedescriptor = %d", fd);
    int wd = inotify_add_watch(fd, (QFile::encodeName(path)), 
	IN_ATTRIB|IN_CLOSE_WRITE|IN_CREATE|IN_DELETE|IN_DELETE_SELF|
	IN_MODIFY|IN_MOVE_SELF|IN_MOVED_FROM|IN_MOVED_TO);
    if (wd==-1) perror("Register watch");
    watches[wd] = path;
    qDebug("Watch registered: %s = %d", QFile::encodeName(path).constData(), wd);
}

void INotify::rmWatch(QString path) {
}

void INotify::start() {
  watchThread = new WatchThread();
  ((WatchThread*)watchThread)->inotify = this;
  watchThread->start();
}

void INotify::stop() {
  qDebug("Terminating watchThread");
  watchThread->terminate();
  qDebug("Waiting...");
  watchThread->wait(2000);
  qDebug("Done.");
  delete watchThread;
  
  qDebug("Terminating updateThread");
  thread->stop();
  qDebug("Waiting...");
  thread->wait(2500);
  qDebug("Done.");
  delete thread;
  
}



void INotify::watchLoop() {
    while (1) {
	int len=read(fd, buf, BUFSIZE);
	int i=0;
	while (i<len) {
	    struct inotify_event *pevent = (struct inotify_event *)&buf[i];
	    QString name = pevent->len ? pevent->name : watches[pevent->wd];

      if (pevent->mask & IN_ACCESS) qDebug("%s was read %d", name.toLatin1().constData(), pevent->cookie);
      if (pevent->mask & IN_ATTRIB) {
	qDebug("%s metadata changed %d", name.toLatin1().constData(), pevent->cookie);
	thread->notifyLocalModification(watches[pevent->wd], pevent->name);
	};
      if (pevent->mask & IN_CLOSE_WRITE) {
	qDebug("%s closed (written) %d", name.toLatin1().constData(), pevent->cookie);
	thread->notifyLocalModification(watches[pevent->wd], pevent->name);
	};
      if (pevent->mask & IN_CLOSE_NOWRITE) qDebug("%s closed(not written) %d", name.toLatin1().constData(), pevent->cookie);
      if (pevent->mask & IN_CREATE) {
	qDebug("%s was created %d", name.toLatin1().constData(), pevent->cookie);
	thread->notifyLocalModification(watches[pevent->wd], pevent->name);
	};
      if (pevent->mask & IN_DELETE) {
	qDebug("%s was deleted %d", name.toLatin1().constData(), pevent->cookie);
	thread->notifyLocalDeletion(watches[pevent->wd], pevent->name);
      }
      if (pevent->mask & IN_DELETE_SELF) qDebug("%s self deleted %d", name.toLatin1().constData(), pevent->cookie);
      if (pevent->mask & IN_MODIFY) {
	qDebug("%s was modified %d", name.toLatin1().constData(), pevent->cookie);
	thread->notifyLocalModification(watches[pevent->wd], pevent->name);
	};
      if (pevent->mask & IN_MOVE_SELF)   qDebug("%s was self moved %d", name.toLatin1().constData(), pevent->cookie);
      if (pevent->mask & IN_MOVED_FROM) {
	qDebug("%s was moved out %d", name.toLatin1().constData(), pevent->cookie);
	//handleLocalDeletion(watches[pevent->wd], pevent->name);
	};
      if (pevent->mask & IN_MOVED_TO) {
	qDebug("%s was moved in %d", name.toLatin1().constData(), pevent->cookie);
	//handleLocalModification(watches[pevent->wd], pevent->name);
	};
      if (pevent->mask & IN_OPEN) qDebug("%s was opened %d", name.toLatin1().constData(), pevent->cookie);
 
	
	    
	    
	    i += sizeof(struct inotify_event) + pevent->len;
	}
    }

}


