#include "treemodel.h"
#include "mydirentry.h"
#include "archive.h"

#include <QtConcurrentRun>
#include <QMenu>
#include <QTreeView>
#include <QFileDialog>


TreeModel::TreeModel(MyRoot* root, Archive *arch, QObject *parent) : QAbstractItemModel(parent), rootItem(root), archive(arch) {
  
}

QVariant TreeModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  if (role != Qt::DisplayRole) return QVariant();

  MyDirEntry *item = static_cast<MyDirEntry*>(index.internalPointer());

  return item->data(index.column());
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return 0;
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return QVariant();
  switch (section) {
    case 0: return "name";
    case 1: return "lastModified";
    case 2: return "by";
    case 3: return "size";
    case 4: return "compressed";
    case 5: return "archiveName";
    case 6: return "MD5";
    default: return QVariant();
  }    
}
			 
QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const {
  if(!hasIndex(row, column, parent)) return QModelIndex();
  MyDirEntry *parentEntry = parent.isValid() ? static_cast<MyDirEntry*>(parent.internalPointer()) : rootItem;
  MyDirEntry *entry = parentEntry->entryAt(row);
  if (entry)
    return createIndex(row, column, entry);
  else
    return QModelIndex();  
}

QModelIndex TreeModel::parent(const QModelIndex &index) const {
  if (!index.isValid()) return QModelIndex();

  MyDirEntry *child = static_cast<MyDirEntry*>(index.internalPointer());
  MyDirEntry *parentItem = child->getParent();

  if (parentItem == rootItem) return QModelIndex();

  return createIndex(parentItem->row(), 0, parentItem);  
}

int TreeModel::rowCount(const QModelIndex &parent) const {
  MyDirEntry *parentItem;
  if (parent.column() > 0) return 0;

  if (!parent.isValid()) parentItem = rootItem;
  else parentItem = static_cast<MyDirEntry*>(parent.internalPointer());

  return parentItem->childCount();
  
}

int TreeModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return static_cast<MyDirEntry*>(parent.internalPointer())->columnCount();
  else
    return rootItem->columnCount();
}

void TreeModel::showContextMenu(const QPoint &position) {
  QTreeView *view = ((QTreeView*)((QObject*)this)->parent());
  contextMenuIndex = view->indexAt(position);
  if (contextMenuIndex.isValid()) {
    QMenu menu;
    //menu.addAction(static_cast<MyDirEntry*>(index.internalPointer())->getName());
    menu.addAction("Download", this, SLOT(downloadEntry()));
    menu.exec(view->mapToGlobal(position));
  }
}

void TreeModel::downloadEntry() {
  if (!contextMenuIndex.isValid()) return;
  MyDirEntry *entry=static_cast<MyDirEntry*>(contextMenuIndex.internalPointer());
  if (entry->isDir()) {
    QString name = QFileDialog::getExistingDirectory(0, "Select file name for saving");
    if (name.isEmpty()) return;
    qDebug("saving in -%s-", name.toLatin1().constData());
  } else {
    QString name = QFileDialog::getSaveFileName(0, "Select file name for saving");
    if (name.isEmpty()) return;
    qDebug("saving as -%s-", name.toLatin1().constData());
    //archive->copyFileFromArchive(name, entry);
    QtConcurrent::run(archive, &Archive::download, name, entry);
  }
}