#ifndef ARCHIVE_H
#define ARCHIVE_H


#include <QDateTime>
#include <QDir>
#include <QMap>
//#include <QSignalMapper>
#include <QFileSystemWatcher>
#include <QTimer>
class MainThread;
class MyDir;
class MyDirEntry;
class MyRoot;


class Archive : public QObject {
Q_OBJECT
public:
  Archive(MainThread* aThread, QDir aLocalBase, QString anArchiveBase);
  MyRoot *getRoot();
  void initializeFolders();
  void download(const QFileInfo& localFileInfo, MyDirEntry* archiveFile);

  void handleLocalDeletion(QString path);
  void handleLocalModification(QString path);
  void writeRootFile();
  
  
public slots:
  void startUpdates();
  void checkForNewRootFile();
  void localFolderChanged(const QString& path);
  void synchronizeLocalFolder(const QString& path);

private:
  void synchronizeAll();
  void syncDir(QDir localDir, MyDir* lastKnownArchiveDir, MyDir* latestArchiveDir);
  void syncLocalDirEntry(MyDir* archiveDir, const QFileInfo& info);
  void syncDirEntry(MyDir* lastKnownArchiveDir, MyDir* latestArchiveDir, const QFileInfo& info);
  void deleteLocalFile(const QFileInfo& info);
  void resolveConflict(MyDir* latestArchiveDir, QFileInfo info);
  void copyFileToArchive(const QFileInfo& localFileInfo, MyDir* archivePath);
  void copyFileFromArchive(const QFileInfo& localFileInfo, MyDirEntry* archiveFile);
  void addPathToWatcher(const QString& path);
  MyRoot* readRootFile(const QString &fileName);
  QString findNewestRootFile();
  MyDir* archiveDirFromLocalDir(const QDir& dir);

  MyRoot* root;
//  MyRoot* allRoots;
  QDir localBase;
  QString archiveBase;
  MainThread *mainThread;
  QString lastRootName;
  QFileSystemWatcher watcher;
//  QSignalMapper signalMapper;
  QMap<QString, QDateTime> updateMap;
  QTimer timer;
//  INotify* notify;
};



#endif