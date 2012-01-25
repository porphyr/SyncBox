#include "mydirentry.h"

#include <QSet>

void MySymlink::writeToStream(QDataStream &out) const {
  out << (qint8)4;
  out << linkTarget;
}

void MySymlink::readFromStream(QDataStream &in, int) {
 in >> linkTarget;
}

bool MySymlink::equals(const MyDirEntry* other) const {
  return (  MyDirEntry::equals(other)
    && linkTarget == ((MySymlink*)other)->linkTarget );
};

QVariant MySymlink::data(int column) const {
  if (column-3 == 2) return linkTarget;
  else  return MyDirEntry::data(column);
}

QString MySymlink::displayKey() const {
  return QString("2") + name.toLower();
}

bool MySymlink::equals(const QFileInfo& info) const {
  return info.isSymLink() && info.symLinkTarget() == linkTarget;
}

QSet<QString> MySymlink::allArchiveFileNames() const {
  QSet<QString> ret;
  return ret;
}