#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>

class MyRoot;
class Archive;

class TreeModel : public QAbstractItemModel
{
Q_OBJECT

public:
  TreeModel(MyRoot* root, Archive *arch, QObject *parent = 0);

  QVariant data(const QModelIndex &index, int role) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;
  QVariant headerData(int section, Qt::Orientation orientation,
                         int role = Qt::DisplayRole) const;
  QModelIndex index(int row, int column,
                       const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &index) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  
public slots:
  void showContextMenu(const QPoint &position);
  void downloadEntry();

private:
//  void setupModelData(const QStringList &lines, TreeItem *parent);
//  TreeItem *rootItem;
  MyRoot *rootItem;
  Archive *archive;
  QModelIndex contextMenuIndex;
};


#endif