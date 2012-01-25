#include "mydirentry.h"
//#include "main.h"

#include <sys/time.h>
#include <pwd.h>
#include <grp.h>

#include <QtCrypto>
#include <QSet>

extern QString hostName;

MyDirEntry::MyDirEntry(const QFileInfo& info) : 
  name(info.fileName()),
  owner(info.owner()), 
  group(info.group()), 
  lastModified(info.lastModified()),
  permissions(info.permissions()),
  lastModifiedBy(hostName),
  parent(0)
{};


MyDir::~MyDir() {
  for (QMap<QString, MyDirEntry*>::iterator i = contents.begin(); i != contents.end(); ++i) 
    delete i.value();
  
}


void MyDir::dump() const {
  for (QMap<QString, MyDirEntry*>::const_iterator i = contents.constBegin(); i != contents.constEnd(); ++i) {
	if (i.value()->isDir()) {
	  qDebug("Dir %s", i.key().toLatin1().constData());
	  ((MyDir*)i.value())->dump();
	} else {
	  qDebug("File %s -> %s", i.key().toLatin1().constData(), ((MyFile*)i.value())->getArchiveName().toLatin1().constData());
	}
    }

}

bool MyDir::isModified() const {
  if (modified) return true;
  foreach (MyDirEntry* entry, contents)
    if (entry->isDir() && ((MyDir*)entry)->isModified()) return true;
  return false;
};
  
void MyDir::resetModified() {
  modified = false;
  foreach (MyDirEntry* entry, contents)
    if (((MyDir*)entry)->isModified()) ((MyDir*)entry)->resetModified();
}

void MyDirEntry::writeToStream(QDataStream &out) const{
  out << owner << group << lastModified << lastModifiedBy << (qint32)permissions;
}


void MyDirEntry::readFromStream(QDataStream &in, int) {
  qint32 perm;
  in >> owner >> group >> lastModified >> lastModifiedBy >> perm;
  permissions = QFlag(perm);
}

/*void MyDirEntry::update(const QFileInfo& info) {
  owner = info.owner();
  group = info.group();
  lastModified = info.lastModified();
  permissions = info.permissions();
  lastModifiedBy = hostName;
} */

bool MyDirEntry::equals(const QFileInfo& info) const{
  return (  isDir() == info.isDir()
    && owner == info.owner()
    && lastModified == info.lastModified()
    && permissions == info.permissions());
}

bool MyDirEntry::equals(const MyDirEntry* other) const {
  /*return (  isDir() == other->isDir()
    && owner == other->owner
    && lastModified == other->lastModified
    && permissions == other->permissions
    && lastModifiedBy == other->lastModifiedBy);*/
  if (isDir() != other->isDir() || isSymLink() != other->isSymLink()) {
    qDebug("type mismatch");
    return false;
  }
  if (owner != other->owner) {
    qDebug("owner mismatch");
    return false;
  }
  if (lastModified != other->lastModified) {
    qDebug("lastModified mismatch");
    return false;
  }
  if (permissions != other->permissions) {
    qDebug("permissions mismatch");
    return false;
  }
  if (lastModifiedBy != other->lastModifiedBy) {
    qDebug("lastModifiedBy mismatch");
    return false;
  }
  return true;
}

int MyDirEntry::row() const{
  if (parent) return parent->indexOf(this);
  return 0;
}

bool MyDir::equals(const MyDirEntry* other) const {
  if (!other->isDir()) {
    qDebug("type mismatch");
    return false;
  }
  if (owner != other->getOwner()) {
    qDebug("owner mismatch");
    return false;
  }
   if (permissions != other->getPermissions()) {
    qDebug("permissions mismatch");
    return false;
  }
  return true;
}

bool MyFile::equals(const MyDirEntry* other) const {
  return (  MyDirEntry::equals(other)
    && archiveName == ((MyFile*)other)->archiveName );
}

MyDirEntry* MyDirEntry::fromStream(QDataStream &in) {
  qint8 type;
  MyDirEntry *retval=0;
  in >> type;
  switch (type) {
    case 1:
	retval = new MyFile();
	break;
    case 2:
	retval = new MyDir();
	break;
    case 3:
	retval = new MyRoot();
	break;
    case 4:
	retval = new MySymlink();
	break;
    default:
      qDebug("Invalid type found in root file: %d", type);
      return 0;
  }
  retval->readFromStream(in, type);
  return retval;
}

void MyDirEntry::setMetadata(const QString& path) const{
    QFile file(path);
    file.setPermissions(permissions);
    
    struct timeval tv[2];
    tv[0].tv_sec=lastModified.toTime_t();
    tv[0].tv_usec=0;
    tv[1].tv_sec=lastModified.toTime_t();
    tv[1].tv_usec=0;
    lutimes(QFile::encodeName(file.fileName()), tv);
    lchown(QFile::encodeName(file.fileName()), uid(), gid());
}

uid_t MyDirEntry::uid() const {
    struct passwd *pwd = getpwnam(owner.toLatin1().constData());
    return pwd->pw_uid;
}

gid_t MyDirEntry::gid() const {
    struct group *grp = getgrnam(group.toLatin1().constData());
    return grp->gr_gid;
}

int MyDirEntry::columnCount() const {
  return 7;
}

QVariant MyDirEntry::data(int column) const {
  switch (column) {
    case 0: return name;
    case 1: return lastModified;
    case 2: return lastModifiedBy;
    default: return QVariant();
  }
};

QVariant MyFile::data(int column) const {
  switch (column-3) {
    case 0: return origSize;
    case 1: return compressedSize;
    case 2: return archiveName;
    case 3: return QCA::arrayToHex(md5);
    default: return MyDirEntry::data(column);
  }
};



void MyFile::writeToStream(QDataStream &out) const{
  out << (qint8)1;
  MyDirEntry::writeToStream(out);
  out << archiveName << origSize << compressedSize << md5;
}

void MyFile::readFromStream(QDataStream &in, int type) {
  MyDirEntry::readFromStream(in, type);
  in >> archiveName >> origSize >> compressedSize >> md5;
}

int MyFile::childCount() const {
  return history.size();
}

MyDirEntry *MyFile::entryAt(int index) {
  return index < history.size() ? history[index] : 0;
}

int MyFile::indexOf(const MyDirEntry* entry) {
  return history.indexOf((MyFile*)entry);
}

void MyFile::addHistoryFrom(MyFile* other, bool addSelf) {
  int x = other->childCount();
  if (addSelf) {
    MyFile *first = new MyFile();
    first->setArchiveName(other->getArchiveName());
    first->setCompressedSize(other->getCompressedSize());
    first->setOrigSize(other->getOrigSize());
    first->setMd5(other->getMd5());
    first->setLastModified(other->getLastModified());
    first->setLastModifiedBy(other->getLastModifiedBy());
    first->setPermissions(other->getPermissions());
    first->setOwner(other->getOwner());
    first->setGroup(other->getGroup());
    first->setParent(this);
    first->setName(QString("V%1").arg(x+1));
    history << first;
  }
  for (x=0; x<other->getHistory().size(); ++x) {
    MyFile *entry = other->getHistory()[x];
    entry->setParent(this);
    history << entry;
  }
  other->getHistory().clear();
}

int MyDir::childCount() const {
  return contents.size();
}



void MyDir::writeToStream(QDataStream &out) const{
  out << (qint8)2;
  MyDirEntry::writeToStream(out);
  out << (qint32)contents.size();
  for (QMap<QString, MyDirEntry*>::const_iterator i = contents.constBegin(); i != contents.constEnd(); ++i) {
	out << i.key();
	i.value()->writeToStream(out);
  }
}

void MyDir::readFromStream(QDataStream &in, int type) {
  qint32 size;
  MyDirEntry::readFromStream(in, type);
  in >> size;
  for (qint32 i=0; i<size; ++i) {
    QString key;
    in >> key;
    MyDirEntry *entry = MyDirEntry::fromStream(in);
    entry->setParent(this);
    entry->setName(key);
    contents[key]=entry;
    displayContents[entry->displayKey()]=entry;
  }
}


void MyDir::addEntry(const QString& entryName, MyDirEntry* entry) { 
  if (!modified) qDebug("%s: dirty by adding %s", name.toLatin1().constData(),
    entryName.toLatin1().constData());
  modified = true;
  lastModifiedBy = hostName;
  contents[entryName] = entry;
  displayContents[entry->displayKey()] = entry;
  entry->setParent(this);
}

MyDirEntry *MyDir::entryAt(int index) {
  MyDirEntry *retval=0;
  QMap<QString,MyDirEntry*>::const_iterator it = displayContents.constBegin() + index;
  
  if (it != displayContents.constEnd()) retval = it.value();
  
  return retval;
}

int MyDir::indexOf(const MyDirEntry* entry) {
  int i=0;
  QMap<QString,MyDirEntry*>::const_iterator it = displayContents.constBegin();
  while (it.value() != entry && it != displayContents.constEnd()) {
    ++i;
    ++it;
  }
  return i;
}

void MyDir::removeEntry(const QString& entryName) {
  if (!modified) qDebug("%s: dirty by removing %s", name.toLatin1().constData(),
    entryName.toLatin1().constData());
  modified = true;
  lastModifiedBy = hostName;
  MyDirEntry *entry=contents.value(entryName, 0);
  contents.remove(entryName);
  if (entry)displayContents.remove(entry->displayKey());
}

MyDir* MyDir::deletedItems() {
  MyDirEntry *entry = contents.value(" deleted ", 0);
  if(!entry) {
    entry = new MyDir;
    entry->setName(" deleted ");
    addEntry(" deleted ", entry);
  }
  return (MyDir*) entry;
}

void MyDir::removeAllUnchanged(MyDir *other) {
  QMap<QString, MyDirEntry*>::iterator it = contents.begin();
  while (it != contents.end()) {
    if (it.value()->getName() == " deleted ") {
      (static_cast<MyDir*>(it.value()))->removeAllUnchanged(other);
      it=contents.erase(it);
    } else {
      MyDirEntry *otherEntry=other->entryAt(it.value()->getName());
      if (otherEntry) {
	if (it.value()->isDir() && otherEntry->isDir()) {
	  (static_cast<MyDir*>(it.value()))->removeAllUnchanged(static_cast<MyDir*>(otherEntry));
	}
      
	if (otherEntry->equals(it.value())) {
	  if (it.value()->isFile() && otherEntry->isFile()) (static_cast<MyFile*>(otherEntry))->addHistoryFrom((static_cast<MyFile*>(it.value())), false); 
	  if (!it.value()->childCount()) {
	    displayContents.remove(it.value()->displayKey());
	    it=contents.erase(it);
	  } else ++it;
	} else {
	  if (it.value()->isFile() && otherEntry->isFile()) (static_cast<MyFile*>(otherEntry))->addHistoryFrom((static_cast<MyFile*>(it.value())), true); 
	  ++it;
	}
      } else {
	other->deletedItems()->addEntry(it.key(), it.value());
	++it;
      }
    }
  }
}

void MyRoot::writeToStream(QDataStream &out) const {
  out << (qint8)3;
  out << lastRootName;
  MyDir::writeToStream(out);
}

void MyRoot::readFromStream(QDataStream &in, int) {
  in >> lastRootName;
  qint8 dummytype;
  in >> dummytype; // type info written by MyDir::writeToStream
  MyDir::readFromStream(in, dummytype);
}

QString MyDir::displayKey() const {
  return QString("1") + name.toLower();
}

QString MyRoot::displayKey() const {
  return name.split('-').last();
}

QString MyFile::displayKey() const {
  return QString("3") + name.toLower();
}

QSet<QString> MyFile::allArchiveFileNames() const {
  QSet<QString> ret;
  ret << archiveName;
  return ret;
}

QSet<QString> MyDir::allArchiveFileNames() const {
  QSet<QString> ret;
  foreach (MyDirEntry* entry, contents)
    ret += entry->allArchiveFileNames();
  return ret;
}
