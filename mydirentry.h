#ifndef MYDIRENTRY_H
#define MYDIRENTRY_H

#include <QString>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QDataStream>
#include <QVariant>



class MyDirEntry {
protected:
  QString name;
  QString owner;
  QString group;
  QDateTime lastModified;
  QFile::Permissions permissions;
  QString lastModifiedBy;
  MyDirEntry *parent;
public:
  MyDirEntry() {};
  MyDirEntry(const QFileInfo& info);
  virtual bool isDir() const=0;
  virtual bool isSymLink() const=0;
  virtual bool isFile() const=0;
  virtual void writeToStream(QDataStream &out) const;
  virtual void readFromStream(QDataStream &in, int type);
  static MyDirEntry* fromStream(QDataStream &in);
  void setMetadata(const QString& path) const;
  uid_t uid() const;
  gid_t gid() const;
  const QDateTime& getLastModified() const { return lastModified; }
  void setLastModified(const QDateTime& dt) { lastModified = dt; }
  const QString& getLastModifiedBy() const { return lastModifiedBy; }
  void setLastModifiedBy(const QString& by) { lastModifiedBy = by; }
//  void update(const QFileInfo& info);
  virtual bool equals(const QFileInfo& info) const;
  virtual bool equals(const MyDirEntry* other) const;
  MyDirEntry *getParent() {return parent; };
  void setParent(MyDirEntry *dir) {parent=dir;};
  int row() const;
  virtual int childCount() const=0;
  virtual int columnCount() const;
  virtual QVariant data(int column) const;
  virtual MyDirEntry *entryAt(int index)=0;
  virtual int indexOf(const MyDirEntry* entry)=0;
  void setName(const QString& string) { name=string; };
  const QString& getName() const { return name; };
  virtual QString displayKey() const=0;
  const QString& getOwner() const { return owner; };
  void setOwner(const QString& string) { owner = string; };
  const QString& getGroup() const { return group; };
  void setGroup(const QString& string) { group = string; };
  const QFile::Permissions& getPermissions() const { return permissions; };
  void setPermissions(const QFile::Permissions& perm) { permissions = perm; };
  virtual QSet<QString> allArchiveFileNames() const = 0;
};

class MySymlink : public MyDirEntry {
private:
  QString linkTarget;
public:
  MySymlink() {};
  virtual bool isDir() const { return false; };
  virtual bool isFile() const { return false; };
  virtual bool isSymLink() const { return true; };
  virtual void writeToStream(QDataStream &out) const;
  virtual void readFromStream(QDataStream &in, int type);
  virtual bool equals(const MyDirEntry* other) const;
  virtual int childCount() const { return 0; };
  virtual MyDirEntry *entryAt(int) { return 0;};
  virtual int indexOf(const MyDirEntry*) { return -1;};
  virtual QVariant data(int column) const;
  const QString& getLinkTarget() const { return linkTarget; };
  void setLinkTarget(const QString& target) { linkTarget = target; };
  virtual QString displayKey() const;
  virtual bool equals(const QFileInfo& info) const;
  virtual QSet<QString> allArchiveFileNames() const;
};

class MyFile : public MyDirEntry {
private:
  QString archiveName;
  int compressedSize;
  int origSize;
  QByteArray md5;
  QList<MyFile*> history;
public:
  MyFile(const QFileInfo& info) : MyDirEntry(info) {};
  MyFile() {};
  void setArchiveName(const QString& aname) { archiveName = aname; };
  QString getArchiveName() const { return archiveName; };
  void setCompressedSize(int size) { compressedSize = size; };
  int getCompressedSize() const { return compressedSize; };
  void setOrigSize(int size) { origSize = size; };
  int getOrigSize() const { return origSize; };
  virtual bool isDir() const { return false; };
  virtual bool isFile() const { return true; };
  virtual bool isSymLink() const { return false; };
  virtual void writeToStream(QDataStream &out) const;
  virtual void readFromStream(QDataStream &in, int type);
  virtual bool equals(const MyDirEntry* other) const;
  void setMd5(QByteArray newMd5) { md5 = newMd5; };
  QByteArray getMd5() const { return md5; };
  virtual int childCount() const;
  virtual QVariant data(int column) const;
  virtual MyDirEntry *entryAt(int index);
  virtual int indexOf(const MyDirEntry* entry);
  void addHistoryFrom(MyFile* other, bool addSelf = false);
  QList<MyFile*>& getHistory() { return history; };
  virtual QString displayKey() const;
  virtual QSet<QString> allArchiveFileNames() const;
};

class MyDir : public MyDirEntry {
private:
  QMap<QString,MyDirEntry*> contents;
  QMap<QString,MyDirEntry*> displayContents;
  bool modified;
public:
  MyDir(QFileInfo info) : MyDirEntry(info), modified(false) {};
  MyDir() : modified(false) {};
  virtual ~MyDir();
//  void mkdir(const QString& name);
  bool contains(const QString& aname) const { return contents.contains(aname); };
  MyDirEntry* entryAt(const QString& aname) { return contents.value(aname, 0); };
  void addEntry(const QString& entryName, MyDirEntry* entry);
  void removeEntry(const QString& entryName);
  virtual bool isDir() const { return true; };
  virtual bool isSymLink() const { return false; };
  virtual bool isFile() const { return false; };
  bool isModified() const;
  void resetModified();
  void dump() const;
  virtual void writeToStream(QDataStream &out) const;
  virtual void readFromStream(QDataStream &in, int type);
  QList<QString> fileNames() const { return contents.keys(); };
  virtual bool equals(const MyDirEntry* other) const;
  virtual MyDirEntry *entryAt(int index);
  virtual int indexOf(const MyDirEntry* entry);
  virtual int childCount() const;
  void removeAllUnchanged(MyDir *other);
  virtual QString displayKey() const;
  virtual QSet<QString> allArchiveFileNames() const;
  MyDir* deletedItems();
};

class MyRoot : public MyDir {
private:
  QString lastRootName;
public:
  const QString& getLastRootName() const { return lastRootName; };
  void setLastRootName(const QString& rootName) { lastRootName = rootName; };
  virtual void writeToStream(QDataStream &out) const;
  virtual void readFromStream(QDataStream &in, int);
  virtual QString displayKey() const;
};


#endif