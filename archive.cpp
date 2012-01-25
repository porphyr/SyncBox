#include "archive.h"
//#include "inotify.h"
#include "main.h"
#include "mainthread.h"
#include "mydirentry.h"

#include <QBuffer>
#include <QSet>
#include <QSettings>
#include <QTimer>


void Archive::addPathToWatcher(const QString& path) {
  if (!watcher.directories().contains(path)) watcher.addPath(path);
}

MyDir* Archive::archiveDirFromLocalDir(const QDir& dir) {
  if (dir == localBase) return root;
  return (MyDir*)archiveDirFromLocalDir(QFileInfo(dir.path()).dir())->entryAt(dir.dirName());
}

void Archive::synchronizeLocalFolder(const QString& path) {
  if (path=="") {
    writeRootFile();
    return;
  }
  
  // Attention! At the time this is called, the path may no longer exist!
  qDebug("Archive::synchronizeLocalFolder(%s)", path.toLatin1().constData());
/*  QTimer *timer=updateMap[path];
  signalMapper.removeMappings(timer);
  updateMap.remove(path);
  delete timer;
*/
  QDir localDir(path);
  if (!localDir.exists()) return;
  MyDir* archiveDir = archiveDirFromLocalDir(localDir);
  //syncDirEntry(archiveDir, archiveDir, info);

//void Archive::syncDir(QDir localDir, MyDir* lastKnownArchiveDir, MyDir* latestArchiveDir) {
  
  localDir.setFilter(QDir::AllEntries|QDir::Hidden|QDir::NoDotAndDotDot);
  QFileInfoList list = localDir.entryInfoList();
  QSet<QString> archiveNames = archiveDir->fileNames().toSet();

  for (int i = 0; i < list.size(); ++i) {
    QFileInfo fileInfo = list.at(i);
    QString fileName=fileInfo.fileName();

    archiveNames.remove(fileName);
    syncLocalDirEntry(archiveDir, fileInfo);
  }

  foreach (QString fileName, archiveNames) {
    syncLocalDirEntry(archiveDir, QFileInfo(localDir.filePath(fileName)));
  }
  if (root->isModified()) localFolderChanged("");
}


void Archive::localFolderChanged(const QString& path) {
  qDebug("Archive::localFolderChanged(%s)", path.toLatin1().constData());
  updateMap[path]=QDateTime::currentDateTime().addSecs(2);
}


void Archive::checkForNewRootFile() {
  QString fileName = findNewestRootFile();
  if (fileName != lastRootName) {
    qDebug("Found new root file: %s", fileName.toLatin1().constData());
    writeRootFile();
    MyDir* oldRoot = root;
    root=readRootFile(fileName);
    syncDir(localBase, oldRoot, root);
    delete(oldRoot);
    lastRootName = fileName;
    QSettings settings;
    settings.setValue("Archive/lastRootName", lastRootName);
    settings.sync();
  } else {
    QMap<QString, QDateTime>::iterator it = updateMap.begin();
    QDateTime current=QDateTime::currentDateTime();
    while (it != updateMap.end()) {
      QString path=it.key();
      if (it.value()<current) {
	it = updateMap.erase(it);
	synchronizeLocalFolder(path);
      } else ++it;
    }
    
  }
  if (root->isModified()) writeRootFile(); 
  timer.start(2000);
}


void Archive::handleLocalDeletion(QString path) {
  qDebug("Archive::handleLocalDeletion(%s)", path.toLatin1().constData());
}


void Archive::handleLocalModification(QString path) {
  qDebug("Archive::handleLocalModification(%s)", path.toLatin1().constData());
}

void Archive::deleteLocalFile(const QFileInfo& info) {
  // Delete no longer is recursive for folder; this should rather be done
  // by recursive synchronisation (because we might miss local changes in
  // subfolders otherwise
  mainThread->log(QString("Deleting %1").arg(info.absoluteFilePath()));
  if (info.isDir()) {
/*    QFileInfoList list = info.absoluteDir().entryInfoList();
    foreach (QFileInfo subinfo, list)
      deleteLocalFile(subinfo);*/
    info.dir().rmdir(info.fileName());
  } else info.dir().remove(info.fileName());
  //FIXME: give user feedback if deletion fails
}

void Archive::resolveConflict(MyDir* latestArchiveDir, QFileInfo info) {
  mainThread->log(QString("Have to resolve conflict %1 %2").arg(latestArchiveDir->getName()).arg(info.absoluteFilePath()));
}

void Archive::copyFileToArchive(const QFileInfo& localFileInfo, MyDir* archivePath) {
  QString path = localFileInfo.absoluteFilePath();

  MyDirEntry *archiveEntry;
  if (localFileInfo.isSymLink()) {
    archiveEntry = new MySymlink();
    archiveEntry->setName(localFileInfo.fileName());
    ((MySymlink*)archiveEntry)->setLinkTarget(localFileInfo.symLinkTarget());
  } else
  if (localFileInfo.isDir()) {
    archiveEntry = new MyDir(localFileInfo);
    syncDir(path, 0, (MyDir*)archiveEntry);
  } else {
    archiveEntry = new MyFile(localFileInfo);

    QString archiveFileName = QString("%1/%2-files/%3").arg(archiveBase).arg(hostName).arg(QDateTime::currentMSecsSinceEpoch());
  
    while (mainThread->exists(archiveFileName))
    {
      sleep(1);
      archiveFileName = QString("%1/%2-files/%3").arg(archiveBase).arg(hostName).arg(QDateTime::currentMSecsSinceEpoch());
    };
    mainThread->log(QString("Sending %1 as %2").arg(path).arg(archiveFileName));
    QBuffer outfile;
    QFile infile(path);
    infile.open(QIODevice::ReadOnly);
    outfile.open(QIODevice::WriteOnly);

    QDataStream out(&outfile);

    QByteArray data;
    QCA::Hash hash(QString("md5"));
    while (1)
    {
      data = infile.read(1024*1024);
      hash.update(data);
      if (data.size() == 0) break;
      out << qCompress(data);
    }
    QByteArray md5 = hash.final().toByteArray();
    //mainThread->log(QString("MD5: ")+QCA::arrayToHex(md5));
    infile.close();
    outfile.close();
    mainThread->upload(archiveFileName, mainThread->encrypt(archiveFileName.toLatin1(), outfile.data()));

    ((MyFile*)archiveEntry)->setArchiveName(archiveFileName);
    ((MyFile*)archiveEntry)->setCompressedSize(outfile.size());
    ((MyFile*)archiveEntry)->setOrigSize(localFileInfo.size());
    ((MyFile*)archiveEntry)->setMd5(md5);
  }
  archivePath->addEntry(localFileInfo.fileName(), archiveEntry);
}

void Archive::download(const QFileInfo& localFileInfo, MyDirEntry* archiveFile) {
  copyFileFromArchive(localFileInfo, archiveFile);    
}

void Archive::copyFileFromArchive(const QFileInfo& localFileInfo, MyDirEntry* archiveFile) {
  QString path = localFileInfo.absoluteFilePath();
  mainThread->log(QString("Copy from archive %1").arg(path));
  QString fileName = localFileInfo.fileName();
  if (archiveFile->isDir()) {
    if (localFileInfo.dir().mkdir(fileName)) 
      syncDir(path, 0, (MyDir*)archiveFile);
    else {
      mainThread->log(QString("Error! Cannot create dir %1").arg(path));
      return;
    }
  } else if (archiveFile->isSymLink()) {
    qDebug("Not creating symlink! not yet supported!");
    return;
  } else {
    QString archiveFileName = ((MyFile*)archiveFile)->getArchiveName();
    mainThread->log(QString("Receiving %1 as %2").arg(fileName).arg(archiveFileName));
    //QByteArray data = mainThread->decrypt(archiveFileName.toLatin1(), mainThread->download(archiveFileName));


    QByteArray byteArray = mainThread->decrypt(archiveFileName.toLatin1(), mainThread->download(archiveFileName));
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::ReadOnly);
    QDataStream in(&buffer);
    QFile outfile(path);
    outfile.open(QIODevice::WriteOnly);
    QByteArray data;
    while (!in.atEnd())
    {
      in >> data;
      outfile.write(qUncompress(data));
    }
    outfile.close();
  }

  archiveFile->setMetadata(path);


}


void Archive::syncDirEntry(MyDir* lastKnownArchiveDir, MyDir* latestArchiveDir, const QFileInfo& info) {
  QDir localDir = info.dir();
  QString fileName = info.fileName();
//  QString path = info.absoluteFilePath();
  bool existsLocally = info.exists();
  MyDirEntry* lastKnownEntry = lastKnownArchiveDir? lastKnownArchiveDir->entryAt(fileName) : 0;
  MyDirEntry* latestEntry = latestArchiveDir ? latestArchiveDir->entryAt(fileName) : 0;
  
  qDebug("Synchronizing entry %s %s %s %s", info.absoluteFilePath().toLatin1().constData(), 
	 existsLocally?"existsLocally":"notLocally",
	 lastKnownEntry?"lastKnown":"notLastKnown",
	 latestEntry?"latestEntry":"notLatestEntry");

  // Check for local modification
  if (lastKnownEntry) {
    if (existsLocally) {
      if (lastKnownEntry->equals(info)) {
        if (latestEntry) {
          // No local modification, file exists in archive - check for archive mod
          if (latestEntry->equals(info)) {
	    // No remote mod either -> if it is directory dive to sync
	    if (lastKnownEntry->isDir()) syncDir(info.absoluteFilePath(), (MyDir*)lastKnownEntry, (MyDir*)latestEntry);
	  } else {
	    // no local mod, but changed in archive. Check first if type has changed
	    if (lastKnownEntry->isDir()) {
	      if (latestEntry->isDir()) {
		syncDir(info.absoluteFilePath(), (MyDir*)lastKnownEntry, (MyDir*)latestEntry);
		latestEntry->setMetadata(info.absoluteFilePath());
	      } else {
		// type mismatch: was folder before
		syncDir(info.absoluteFilePath(), (MyDir*)lastKnownEntry, 0);
		deleteLocalFile(info);
		copyFileFromArchive(info, latestEntry);
	      }
	    } else {
	      // if it was a file before, just delete and then copy from archive
	      // (no matter if it is a file or directory now)
	      deleteLocalFile(info);
	      copyFileFromArchive(info, latestEntry);
	    }
	  }
        } else { // !latestEntry
          // No local modification, deleted remotely - delete locally!
          if (lastKnownEntry->isDir()) syncDir(info.absoluteFilePath(), (MyDir*)lastKnownEntry, 0);
          deleteLocalFile(info);
        }
      } else { // !lastKnownEntry->equals(info)
        if (latestEntry) {
          // local modification, file exists in archive - check for archive mod
          if (latestEntry->equals(lastKnownEntry)) {
	    // local mod, but unchanged in archive -> copy to archive
	    if (info.isDir() && latestEntry->isDir()) {
		syncDir(info.absoluteFilePath(), (MyDir*)lastKnownEntry, (MyDir*)latestEntry);
		latestEntry->setMetadata(info.absoluteFilePath());
	    } else {
	      // local entry is a file or type mismatch - delete the archive entry, upload the local file
	      latestArchiveDir->removeEntry(fileName);
	      copyFileToArchive(info, latestArchiveDir);
	    }
	  } else {
	    // local mod, also changed in archive -> worst case?
	    resolveConflict(latestArchiveDir, info);
	  }
        } else {
          // Local modification, restore in Archive
	  copyFileToArchive(info, latestArchiveDir);
        }
      }
    } else { // !existsLocally
      // Existed before, but was deleted locally
      if (latestEntry) {
        if (latestEntry->equals(lastKnownEntry)) {
	  // Deleted locally, unchanged in archive -> delete in archive
	  mainThread->log(QString("Deleting from archive: %1").arg(info.absoluteFilePath()));
	  latestArchiveDir->removeEntry(fileName);
	} else {
	  // Deleted locally, but modified in archive -> copy from archive
	  copyFileFromArchive(info, latestEntry);
	}
      } else {
        // Deleted both locally and in newest Archive
        // No action required here
        return;
      }
    }
  } else { // !lastKnownEntry
    if (existsLocally) {
      // Did not exist before, exists now - check for archive updates
      if (latestEntry) {
        // This is a conflict with almost 100% probability: created both locally and in archive
	resolveConflict(latestArchiveDir, info);
      } else {
        // File created locally, copy it to archive
        copyFileToArchive(info, latestArchiveDir);
      }
    } else {
      // Neither existed before, nor exists locally
      if (latestEntry) {
        // File created in newest Archive
        copyFileFromArchive(info, latestEntry);
      } else {
        // We should never actually end up here - file exists nowhere ?!?
        return;
      }
    }
  }
}

void Archive::syncLocalDirEntry(MyDir* archiveDir, const QFileInfo& info) {
  QDir localDir = info.dir();
  QString fileName = info.fileName();
//  QString path = info.absoluteFilePath();
  bool existsLocally = info.exists();
  MyDirEntry* archiveEntry = archiveDir ? archiveDir->entryAt(fileName) : 0;
  
  qDebug("Synchronizing entry %s %s %s", info.absoluteFilePath().toLatin1().constData(), 
	 existsLocally?"existsLocally":"notLocally",
	 archiveEntry?"archiveEntry":"notArchiveEntry");

  // Check for local modification
  if (archiveEntry) {
    if (existsLocally) {
      if (archiveEntry->equals(info)) {
        // No local modification - nothing left to do - not even dive into dirs...
        
      } else { // !archiveEntry->equals(info)
          // local modification
	    if (info.isDir() && archiveEntry->isDir()) {
	        //no need to dive - subdir change would have caused separate notification
		//syncDir(info.absoluteFilePath(), (MyDir*)lastKnownEntry, (MyDir*)latestEntry);
		archiveEntry->setMetadata(info.absoluteFilePath());
	    } else {
	      // local entry is a file or type mismatch - delete the archive entry, upload the local file
	      archiveDir->removeEntry(fileName);
	      copyFileToArchive(info, archiveDir);
	    }
      }
    } else { // !existsLocally
      // Existed before, but was deleted locally
	  mainThread->log(QString("Deleting from archive: %1").arg(info.absoluteFilePath()));
	  archiveDir->removeEntry(fileName);
    }
  } else { // !archiveEntry
    if (existsLocally) {
      // Did not exist before, exists now - check for archive updates
        // File created locally, copy it to archive
        copyFileToArchive(info, archiveDir);
    }
    
  }
}


void Archive::syncDir(QDir localDir, MyDir* lastKnownArchiveDir, MyDir* latestArchiveDir) {
  qDebug("Synchronizing %s", localDir.absolutePath().toLatin1().constData());
  
//  notify->addWatch(localDir.absolutePath());
  addPathToWatcher(localDir.absolutePath());

  localDir.setFilter(QDir::AllEntries|QDir::Hidden|QDir::NoDotAndDotDot);
  QFileInfoList list = localDir.entryInfoList();
  QSet<QString> archiveNames;
  if (latestArchiveDir) {
    archiveNames += latestArchiveDir->fileNames().toSet();
    qDebug("archive contents: %s", QStringList(latestArchiveDir->fileNames()).join(", ").toLatin1().constData());
  }
  if (lastKnownArchiveDir)
    archiveNames.unite(lastKnownArchiveDir->fileNames().toSet());
  for (int i = 0; i < list.size(); ++i) {
    QFileInfo fileInfo = list.at(i);
    QString fileName=fileInfo.fileName();

    archiveNames.remove(fileName);
    syncDirEntry(lastKnownArchiveDir, latestArchiveDir, fileInfo);
  }

  foreach (QString fileName, archiveNames) {
    syncDirEntry(lastKnownArchiveDir, latestArchiveDir, QFileInfo(localDir.filePath(fileName)));
  }
  
}

MyRoot* Archive::readRootFile(const QString &fileName) {
  QByteArray byteArray = qUncompress(mainThread->decrypt(fileName.toLatin1(), mainThread->download(fileName)));
  QBuffer buffer(&byteArray);
  buffer.open(QIODevice::ReadOnly);
  QDataStream in(&buffer);
  return (MyRoot*)MyDirEntry::fromStream(in);
}


void Archive::writeRootFile() {
  if (!root->isModified()) return; 
  QString fileName = QString("%1/root/%2-%3").arg(archiveBase).arg(hostName).arg(QDateTime::currentMSecsSinceEpoch());
  
  while (mainThread->exists(fileName))
  {
    mainThread->log("Root File Name Conflict!");
    sleep(1);
    fileName = QString("%1/root/%2-%3").arg(archiveBase).arg(QDateTime::currentMSecsSinceEpoch()).arg(hostName);  
  };
  
  QByteArray byteArray;
  QBuffer buffer(&byteArray);
  buffer.open(QIODevice::WriteOnly);
  QDataStream out(&buffer);
  root->setLastRootName(lastRootName);
  root->setLastModifiedBy(hostName);
  root->setLastModified(QDateTime::currentDateTime());
  root->writeToStream(out);
  buffer.close();
  
  mainThread->log(QString("Writing root file as ")+fileName);
  mainThread->upload(fileName, mainThread->encrypt(fileName.toLatin1(), qCompress(byteArray)));
  lastRootName = fileName;
}

QString Archive::findNewestRootFile() {
  const QList<QWebdavUrlInfo>& infoList = mainThread->listDir(QString("%1/root").arg(archiveBase));
  QString name;
  QDateTime modified;
  for (int i=0; i<infoList.size(); ++i) {
    if (infoList[i].isFile() && (modified.isNull() || modified < infoList[i].lastModified())) {
      name=infoList[i].displayName();
      modified=infoList[i].lastModified(); 
    }
  } 
  if (name.isEmpty()) return name;
  return QString("%1/root/%2").arg(archiveBase).arg(name);
}


void Archive::initializeFolders() {
  mainThread->mkdir(archiveBase);
  mainThread->mkdir(QString("%1/root").arg(archiveBase));
  mainThread->mkdir(QString("%1/%2-files").arg(archiveBase).arg(hostName));
}

Archive::Archive(MainThread* aThread, QDir aLocalBase, QString anArchiveBase): 
  localBase (aLocalBase),
  archiveBase (anArchiveBase),
  mainThread (aThread) 
{
//  notify = new INotify(this);
  QSettings settings;
  lastRootName = settings.value("Archive/lastRootName").toString();
  connect(&watcher, SIGNAL(directoryChanged(const QString&)), this, SLOT(localFolderChanged(const QString &)));
//  connect(&signalMapper, SIGNAL(mapped(const QString&)), this, SLOT(synchronizeLocalFolder(const QString &)));
  connect(&timer, SIGNAL(timeout()), this, SLOT(checkForNewRootFile()));
  timer.setSingleShot(true);  
/*

  qDebug("all roots built");
  QList<QWebdavUrlInfo> repoList = mainThread->listDir(archiveBase);
  for (int i=0; i<repoList.size(); ++i) {
    QString repoName=repoList[i].displayName();
    qDebug("%s", repoName.toLatin1().constData());
    if (repoName.endsWith("-files")) {
      QList<QWebdavUrlInfo> fileList = mainThread->listDir(QString("%1/%2").arg(archiveBase).arg(repoName));
      for (int j=0; j<fileList.size(); ++j) {
        if (fileList[j].isFile()) {
          QString fileName=QString("%1/%2/%3").arg(archiveBase).arg(repoName).arg(fileList[j].displayName());
          if(!usedArchiveFiles.contains(fileName)) {
            qDebug("orphaned file %s", fileName.toLatin1().constData());
            mainThread->remove(fileName);
          }
        }
      }
    }
  }
*/
}

void Archive::synchronizeAll() {
  MyRoot *oldRoot = 0;
  if (!lastRootName.isEmpty()) oldRoot=readRootFile(lastRootName);
  
  QString rootFileName = findNewestRootFile();
  qDebug("newestrootfile is %s", rootFileName.toLatin1().constData());
  root = rootFileName.isEmpty()? new MyRoot : readRootFile(rootFileName);
//  root -> dump();
  syncDir(localBase, oldRoot, root);
  delete oldRoot;
  lastRootName = rootFileName;
  qDebug("write root file");
  if (root->isModified()) writeRootFile(); 
  QSettings settings;
  settings.setValue("Archive/lastRootName", lastRootName);
  settings.sync();
/*  qDebug("dumping root file");
  root->dump();*/
//  notify->start();
}

MyRoot* Archive::getRoot() {
  QSet<QString> usedArchiveFiles;
  MyRoot *allRoots = new MyRoot();
  const QList<QWebdavUrlInfo>& infoList = mainThread->listDir(QString("%1/root").arg(archiveBase));
  MyRoot *previous=0;
  for (int i=0; i<infoList.size(); ++i) {
    qDebug("%d %s", i, infoList[i].displayName().toLatin1().constData());
    if (infoList[i].isFile()) {
      QString name=infoList[i].displayName();
      MyRoot *subRoot=readRootFile(QString("%1/root/%2").arg(archiveBase).arg(name));
      usedArchiveFiles += subRoot->allArchiveFileNames();
      if (previous) previous->removeAllUnchanged(subRoot);
      previous = subRoot;
      subRoot->setName(name);
      allRoots->addEntry(name, subRoot);
    }
  } 
  qDebug("all roots built");
  return allRoots;
}


void Archive::startUpdates() {
  synchronizeAll();
  timer.start(2000);
}