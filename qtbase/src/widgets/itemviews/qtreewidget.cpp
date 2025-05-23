// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtreewidget.h"

#include <qheaderview.h>
#include <qpainter.h>
#include <qstack.h>
#include <qdebug.h>
#include <private/qtreewidget_p.h>
#include <private/qwidgetitemdata_p.h>
#include <private/qtreewidgetitemiterator_p.h>

#include <QtCore/private/qduplicatetracker_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

class QTreeModelLessThan
{
public:
    inline bool operator()(QTreeWidgetItem *i1, QTreeWidgetItem *i2) const
        { return *i1 < *i2; }
};

class QTreeModelGreaterThan
{
public:
    inline bool operator()(QTreeWidgetItem *i1, QTreeWidgetItem *i2) const
        { return *i2 < *i1; }
};

/*
    \class QTreeModel
    \brief The QTreeModel class manages the items stored in a tree view.

    \ingroup model-view
    \inmodule QtWidgets

*/

/*!
    \enum QTreeWidgetItem::ChildIndicatorPolicy
    \since 4.3

    \value ShowIndicator     The controls for expanding and collapsing will be shown for this item even if there are no children.
    \value DontShowIndicator   The controls for expanding and collapsing will never be shown even if there are children.  If the node is forced open the user will not be able to expand or collapse the item.
    \value DontShowIndicatorWhenChildless  The controls for expanding and collapsing will be shown if the item contains children.
*/

/*!
    \fn void QTreeWidgetItem::setDisabled(bool disabled)
    \since 4.3

    Disables the item if \a disabled is true; otherwise enables the item.

    \sa setFlags()
*/

/*!
    \fn bool QTreeWidgetItem::isDisabled() const
    \since 4.3

    Returns \c true if the item is disabled; otherwise returns \c false.

    \sa setFlags()
*/

/*!
  \internal

  Constructs a tree model with a \a parent object and the given
  number of \a columns.
*/

QTreeModel::QTreeModel(int columns, QTreeWidget *parent)
    : QAbstractItemModel(*new QTreeModelPrivate, parent),
      rootItem(new QTreeWidgetItem),
      headerItem(new QTreeWidgetItem)
{
    rootItem->view = parent;
    rootItem->itemFlags = Qt::ItemIsDropEnabled;
    headerItem->view = parent;
    setColumnCount(columns);
}

/*!
  \internal

*/

QTreeModel::QTreeModel(QTreeModelPrivate &dd, QTreeWidget *parent)
    : QAbstractItemModel(dd, parent), rootItem(new QTreeWidgetItem), headerItem(new QTreeWidgetItem)
{
    rootItem->view = parent;
    rootItem->itemFlags = Qt::ItemIsDropEnabled;
    headerItem->view = parent;
}

/*!
  \internal

  Destroys this tree model.
*/

QTreeModel::~QTreeModel()
{
    clear();
    headerItem->view = nullptr;
    delete headerItem;
    rootItem->view = nullptr;
    delete rootItem;
}

/*!
  \internal

  Removes all items in the model.
*/

void QTreeModel::clear()
{
    SkipSorting skipSorting(this);
    beginResetModel();
    for (int i = 0; i < rootItem->childCount(); ++i) {
        QTreeWidgetItem *item = rootItem->children.at(i);
        item->par = nullptr;
        item->view = nullptr;
        delete item;
    }
    rootItem->children.clear();
    sortPendingTimer.stop();
    endResetModel();
}

/*!
  \internal

  Sets the number of \a columns in the tree model.
*/

void QTreeModel::setColumnCount(int columns)
{
    SkipSorting skipSorting(this);
    if (columns < 0)
        return;
    if (!headerItem) {
        headerItem = new QTreeWidgetItem();
        headerItem->view = view();
    }
    int count = columnCount();
    if (count == columns)
        return;

    if (columns < count) {
        beginRemoveColumns(QModelIndex(), columns, count - 1);
        headerItem->values.resize(columns);
        endRemoveColumns();
    } else {
        beginInsertColumns(QModelIndex(), count, columns - 1);
        headerItem->values.resize(columns);
        for (int i = count; i < columns; ++i) {// insert data without emitting the dataChanged signal
            headerItem->values[i].append(QWidgetItemData(Qt::DisplayRole, QString::number(i + 1)));
            headerItem->d->display.append(QString::number(i + 1));
        }
        endInsertColumns();
    }
}

/*!
  \internal

  Returns the tree view item corresponding to the \a index given.

  \sa QModelIndex
*/

QTreeWidgetItem *QTreeModel::item(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;
    return static_cast<QTreeWidgetItem*>(index.internalPointer());
}

/*!
  \internal

  Returns the model index that refers to the
  tree view \a item and \a column.
*/

QModelIndex QTreeModel::index(const QTreeWidgetItem *item, int column) const
{
    executePendingSort();

    if (!item || (item == rootItem))
        return QModelIndex();
    const QTreeWidgetItem *par = item->parent();
    QTreeWidgetItem *itm = const_cast<QTreeWidgetItem*>(item);
    if (!par)
        par = rootItem;
    int row;
    int guess = item->d->rowGuess;
    if (guess >= 0
        && par->children.size() > guess
        && par->children.at(guess) == itm) {
        row = guess;
    } else {
        row = par->children.lastIndexOf(itm);
        itm->d->rowGuess = row;
    }
    return createIndex(row, column, itm);
}

/*!
  \internal
  \reimp

  Returns the model index with the given \a row,
  \a column and \a parent.
*/

QModelIndex QTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    executePendingSort();

    int c = columnCount(parent);
    if (row < 0 || column < 0 || column >= c)
        return QModelIndex();

    QTreeWidgetItem *parentItem = parent.isValid() ? item(parent) : rootItem;
    if (parentItem && row < parentItem->childCount()) {
        QTreeWidgetItem *itm = parentItem->child(row);
        if (itm)
            return createIndex(row, column, itm);
        return QModelIndex();
    }

    return QModelIndex();
}

/*!
  \internal
  \reimp

  Returns the parent model index of the index given as
  the \a child.
*/

QModelIndex QTreeModel::parent(const QModelIndex &child) const
{
    SkipSorting skipSorting(this); //The reason we don't sort here is that this might be called from a valid QPersistentModelIndex
                                   //We don't want it to become suddenly invalid

    if (!child.isValid())
        return QModelIndex();
    QTreeWidgetItem *itm = static_cast<QTreeWidgetItem *>(child.internalPointer());
    if (!itm || itm == rootItem)
        return QModelIndex();
    QTreeWidgetItem *parent = itm->parent();
    return index(parent, 0);
}

/*!
  \internal
  \reimp

  Returns the number of rows in the \a parent model index.
*/

int QTreeModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return rootItem->childCount();

    QTreeWidgetItem *parentItem = item(parent);
    if (parentItem)
        return parentItem->childCount();
    return 0;
}

/*!
  \internal
  \reimp

  Returns the number of columns in the item referred to by
  the given \a index.
*/

int QTreeModel::columnCount(const QModelIndex &index) const
{
    Q_UNUSED(index);
    if (!headerItem)
        return 0;
    return headerItem->columnCount();
}

bool QTreeModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return (rootItem->childCount() > 0);

    QTreeWidgetItem *itm = item(parent);
    if (!itm)
        return false;
    switch (itm->d->policy) {
    case QTreeWidgetItem::ShowIndicator:
        return true;
    case QTreeWidgetItem::DontShowIndicator:
        return false;
    case QTreeWidgetItem::DontShowIndicatorWhenChildless:
        return (itm->childCount() > 0);
    }
    return false;
}

/*!
  \internal
  \reimp

  Returns the data corresponding to the given model \a index
  and \a role.
*/

QVariant QTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    QTreeWidgetItem *itm = item(index);
    if (itm)
        return itm->data(index.column(), role);
    return QVariant();
}

/*!
  \internal
  \reimp

  Sets the data for the item specified by the \a index and \a role
  to that referred to by the \a value.

  Returns \c true if successful; otherwise returns \c false.
*/

bool QTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    QTreeWidgetItem *itm = item(index);
    if (itm) {
        itm->setData(index.column(), role, value);
        return true;
    }
    return false;
}

bool QTreeModel::clearItemData(const QModelIndex &index)
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid))
        return false;
    QTreeWidgetItem *itm = item(index);
    if (!itm)
        return false;
    const auto beginIter = itm->values.at(index.column()).cbegin();
    const auto endIter = itm->values.at(index.column()).cend();
    if (std::all_of(beginIter, endIter, [](const QWidgetItemData& data) -> bool { return !data.value.isValid(); })
        && !itm->d->display.at(index.column()).isValid()) {
        return true; //it's already cleared
    }
    itm->d->display[index.column()] = QVariant();
    itm->values[index.column()].clear();
    emit dataChanged(index, index, QList<int> {});
    return true;
}

QMap<int, QVariant> QTreeModel::itemData(const QModelIndex &index) const
{
    QMap<int, QVariant> roles;
    QTreeWidgetItem *itm = item(index);
    if (itm) {
        int column = index.column();
        if (column < itm->values.size()) {
            for (int i = 0; i < itm->values.at(column).size(); ++i) {
                roles.insert(itm->values.at(column).at(i).role,
                             itm->values.at(column).at(i).value);
            }
        }

        // the two special cases
        QVariant displayValue = itm->data(column, Qt::DisplayRole);
        if (displayValue.isValid())
            roles.insert(Qt::DisplayRole, displayValue);

        QVariant checkValue = itm->data(column, Qt::CheckStateRole);
        if (checkValue.isValid())
            roles.insert(Qt::CheckStateRole, checkValue);
    }
    return roles;
}

/*!
  \internal
  \reimp
*/
bool QTreeModel::insertRows(int row, int count, const QModelIndex &parent)
{
    SkipSorting skipSorting(this);
    if (count < 1 || row < 0 || row > rowCount(parent) || parent.column() > 0)
        return false;

    beginInsertRows(parent, row, row + count - 1);
    QTreeWidgetItem *par = item(parent);
    while (count > 0) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->view = view();
        item->par = par;
        if (par)
            par->children.insert(row++, item);
        else
            rootItem->children.insert(row++, item);
        --count;
    }
    endInsertRows();
    return true;
}

/*!
  \internal
  \reimp
*/
bool QTreeModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    SkipSorting skipSorting(this);
    if (count < 1 || column < 0 || column > columnCount(parent) || parent.column() > 0 || !headerItem)
        return false;

    beginInsertColumns(parent, column, column + count - 1);

    int oldCount = columnCount(parent);
    column = qBound(0, column, oldCount);
    headerItem->values.resize(oldCount + count);
    for (int i = oldCount; i < oldCount + count; ++i) {
        headerItem->values[i].append(QWidgetItemData(Qt::DisplayRole, QString::number(i + 1)));
        headerItem->d->display.append(QString::number(i + 1));
    }

    QStack<QTreeWidgetItem*> itemstack;
    itemstack.push(0);
    while (!itemstack.isEmpty()) {
        QTreeWidgetItem *par = itemstack.pop();
        QList<QTreeWidgetItem*> children = par ? par->children : rootItem->children;
        for (int row = 0; row < children.size(); ++row) {
            QTreeWidgetItem *child = children.at(row);
            if (child->children.size())
                itemstack.push(child);
            child->values.insert(column, count, QList<QWidgetItemData>());
        }
    }

    endInsertColumns();
    return true;
}

/*!
  \internal
  \reimp
*/
bool QTreeModel::removeRows(int row, int count, const QModelIndex &parent) {
    if (count < 1 || row < 0 || (row + count) > rowCount(parent))
        return false;
    QTreeWidgetItem *parentItem = item(parent);
    // if parentItem is valid, begin/end RemoveRows is handled by takeChild below
    if (!parentItem)
        beginRemoveRows(parent, row, row + count - 1);
    for (int i = row + count - 1; i >= row; --i) {
        QTreeWidgetItem *child = parentItem ? parentItem->takeChild(i) : rootItem->children.takeAt(i);
        Q_ASSERT(child);
        child->view = nullptr;
        delete child;
    }
    if (!parentItem)
        endRemoveRows();
    return true;
}

/*!
  \internal
  \reimp

  Returns the header data corresponding to the given header \a section,
  \a orientation and data \a role.
*/

QVariant QTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    if (headerItem)
        return headerItem->data(section, role);
    if (role == Qt::DisplayRole)
        return QString::number(section + 1);
    return QVariant();
}

/*!
  \internal
  \reimp

  Sets the header data for the item specified by the header \a section,
  \a orientation and data \a role to the given \a value.

  Returns \c true if successful; otherwise returns \c false.
*/

bool QTreeModel::setHeaderData(int section, Qt::Orientation orientation,
                               const QVariant &value, int role)
{
    if (section < 0 || orientation != Qt::Horizontal || !headerItem || section >= columnCount())
        return false;

    headerItem->setData(section, role, value);
    return true;
}

/*!
  \reimp

  Returns the flags for the item referred to the given \a index.

*/

Qt::ItemFlags QTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return rootItem->flags();
    QTreeWidgetItem *itm = item(index);
    Q_ASSERT(itm);
    return itm->flags();
}

/*!
  \internal

  Sorts the entire tree in the model in the given \a order,
  by the values in the given \a column.
*/

void QTreeModel::sort(int column, Qt::SortOrder order)
{
    SkipSorting skipSorting(this);
    sortPendingTimer.stop();

    if (column < 0 || column >= columnCount())
        return;

    //layoutAboutToBeChanged and layoutChanged will be called by sortChildren
    rootItem->sortChildren(column, order, true);
}

/*!
  \internal
*/
void QTreeModel::ensureSorted(int column, Qt::SortOrder order,
                              int start, int end, const QModelIndex &parent)
{
    if (isChanging())
        return;

    sortPendingTimer.stop();

    if (column < 0 || column >= columnCount())
        return;

    SkipSorting skipSorting(this);

    QTreeWidgetItem *itm = item(parent);
    if (!itm)
        itm = rootItem;
    QList<QTreeWidgetItem*> lst = itm->children;

    int count = end - start + 1;
    QList<QPair<QTreeWidgetItem *, int>> sorting(count);
    for (int i = 0; i < count; ++i) {
        sorting[i].first = lst.at(start + i);
        sorting[i].second = start + i;
    }

    const auto compare = (order == Qt::AscendingOrder ? &itemLessThan : &itemGreaterThan);
    std::stable_sort(sorting.begin(), sorting.end(), compare);

    QModelIndexList oldPersistentIndexes;
    QModelIndexList newPersistentIndexes;
    QList<QTreeWidgetItem*>::iterator lit = lst.begin();
    bool changed = false;

    for (int i = 0; i < count; ++i) {
        int oldRow = sorting.at(i).second;

        int tmpitepos = lit - lst.begin();
        QTreeWidgetItem *item = lst.takeAt(oldRow);
        if (tmpitepos > lst.size())
            --tmpitepos;
        lit = lst.begin() + tmpitepos;

        lit = sortedInsertionIterator(lit, lst.end(), order, item);
        int newRow = qMax<qsizetype>(lit - lst.begin(), 0);

        if ((newRow < oldRow) && !(*item < *lst.at(oldRow - 1)) && !(*lst.at(oldRow - 1) < *item ))
            newRow = oldRow;

        lit = lst.insert(lit, item);
        if (newRow != oldRow) {
            // we are going to change the persistent indexes, so we need to prepare
            if (!changed) { // this will only happen once
                changed = true;
                emit layoutAboutToBeChanged({parent}, QAbstractItemModel::VerticalSortHint); // the selection model needs to know
                oldPersistentIndexes = persistentIndexList();
                newPersistentIndexes = oldPersistentIndexes;
            }
            for (int j = i + 1; j < count; ++j) {
                int otherRow = sorting.at(j).second;
                if (oldRow < otherRow && newRow >= otherRow)
                    --sorting[j].second;
                else if (oldRow > otherRow && newRow <= otherRow)
                    ++sorting[j].second;
            }
            for (int k = 0; k < newPersistentIndexes.size(); ++k) {
                QModelIndex pi = newPersistentIndexes.at(k);
                if (pi.parent() != parent)
                    continue;
                int oldPersistentRow = pi.row();
                int newPersistentRow = oldPersistentRow;
                if (oldPersistentRow == oldRow)
                    newPersistentRow = newRow;
                else if (oldRow < oldPersistentRow && newRow >= oldPersistentRow)
                    newPersistentRow = oldPersistentRow - 1;
                else if (oldRow > oldPersistentRow && newRow <= oldPersistentRow)
                    newPersistentRow = oldPersistentRow + 1;
                if (newPersistentRow != oldPersistentRow)
                    newPersistentIndexes[k] = createIndex(newPersistentRow,
                                                          pi.column(), pi.internalPointer());
            }
        }
    }

    if (changed) {
        itm->children = lst;
        changePersistentIndexList(oldPersistentIndexes, newPersistentIndexes);
        emit layoutChanged({parent}, QAbstractItemModel::VerticalSortHint);
    }
}

/*!
  \internal

  Returns \c true if the value of the \a left item is
  less than the value of the \a right item.

  Used by the sorting functions.
*/

bool QTreeModel::itemLessThan(const QPair<QTreeWidgetItem*,int> &left,
                              const QPair<QTreeWidgetItem*,int> &right)
{
    return *(left.first) < *(right.first);
}

/*!
  \internal

  Returns \c true if the value of the \a left item is
  greater than the value of the \a right item.

  Used by the sorting functions.
*/

bool QTreeModel::itemGreaterThan(const QPair<QTreeWidgetItem*,int> &left,
                                 const QPair<QTreeWidgetItem*,int> &right)
{
    return *(right.first) < *(left.first);
}

/*!
  \internal
*/
QList<QTreeWidgetItem*>::iterator QTreeModel::sortedInsertionIterator(
    const QList<QTreeWidgetItem*>::iterator &begin,
    const QList<QTreeWidgetItem*>::iterator &end,
    Qt::SortOrder order, QTreeWidgetItem *item)
{
    if (order == Qt::AscendingOrder)
        return std::lower_bound(begin, end, item, QTreeModelLessThan());
    return std::lower_bound(begin, end, item, QTreeModelGreaterThan());
}

QStringList QTreeModel::mimeTypes() const
{
    auto v = view();
    if (v)
        return v->mimeTypes();
    return {};
}

QMimeData *QTreeModel::internalMimeData()  const
{
    return QAbstractItemModel::mimeData(cachedIndexes);
}

QMimeData *QTreeModel::mimeData(const QModelIndexList &indexes) const
{
    QList<QTreeWidgetItem *> items;
    std::transform(indexes.begin(), indexes.end(), std::back_inserter(items),
                   [this](const QModelIndex &idx) -> QTreeWidgetItem * { return item(idx); });

    // Ensure we only have one item as an item may have more than
    // one index selected if there is more than one column
    std::sort(items.begin(), items.end());
    items.erase(std::unique(items.begin(), items.end()), items.end());

    // cachedIndexes is a little hack to avoid copying from QModelIndexList to
    // QList<QTreeWidgetItem*> and back again in the view
    cachedIndexes = indexes;
    QMimeData *mimeData = view()->mimeData(items);
    cachedIndexes.clear();
    return mimeData;
}

bool QTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                              int row, int column, const QModelIndex &parent)
{
    if (row == -1 && column == -1)
        row = rowCount(parent); // append
    return view()->dropMimeData(item(parent), row, data, action);
}

Qt::DropActions QTreeModel::supportedDropActions() const
{
    return view()->supportedDropActions();
}

void QTreeModel::itemChanged(QTreeWidgetItem *item)
{
    SkipSorting skipSorting(this); //this is kind of wrong, but not doing this would kill performance
    QModelIndex left = index(item, 0);
    QModelIndex right = index(item, item->columnCount() - 1);
    emit dataChanged(left, right);
}

bool QTreeModel::isChanging() const
{
    Q_D(const QTreeModel);
    return !d->changes.isEmpty();
}

/*!
  \internal
    Emits the dataChanged() signal for the given \a item.
    if column is -1 then all columns have changed
*/

void QTreeModel::emitDataChanged(QTreeWidgetItem *item, int column, const QList<int> &roles)
{
    if (signalsBlocked())
        return;

    if (headerItem == item && column < item->columnCount()) {
        if (column == -1)
            emit headerDataChanged(Qt::Horizontal, 0, columnCount() - 1);
        else
            emit headerDataChanged(Qt::Horizontal, column, column);
        return;
    }

    SkipSorting skipSorting(this); //This is a little bit wrong, but not doing it would kill performance

    QModelIndex bottomRight, topLeft;
    if (column == -1) {
        topLeft = index(item, 0);
        bottomRight = createIndex(topLeft.row(), columnCount() - 1, item);
    } else {
        topLeft = index(item, column);
        bottomRight = topLeft;
    }
    emit dataChanged(topLeft, bottomRight, roles);
}

void QTreeModel::beginInsertItems(QTreeWidgetItem *parent, int row, int count)
{
    QModelIndex par = index(parent, 0);
    beginInsertRows(par, row, row + count - 1);
}

void QTreeModel::endInsertItems()
{
    endInsertRows();
}

void QTreeModel::beginRemoveItems(QTreeWidgetItem *parent, int row, int count)
{
    Q_ASSERT(row >= 0);
    Q_ASSERT(count > 0);
    beginRemoveRows(index(parent, 0), row, row + count - 1);
    if (!parent)
        parent = rootItem;
    // now update the iterators
    for (int i = 0; i < iterators.size(); ++i) {
        for (int j = 0; j < count; j++) {
            QTreeWidgetItem *c = parent->child(row + j);
            iterators[i]->d_func()->ensureValidIterator(c);
        }
    }
}

void QTreeModel::endRemoveItems()
{
    endRemoveRows();
}

void QTreeModel::sortItems(QList<QTreeWidgetItem*> *items, int column, Qt::SortOrder order)
{
    // see QTreeViewItem::operator<
    Q_UNUSED(column);
    if (isChanging())
        return;

    // store the original order of indexes
    QList<QPair<QTreeWidgetItem *, int>> sorting(items->size());
    for (int i = 0; i < sorting.size(); ++i) {
        sorting[i].first = items->at(i);
        sorting[i].second = i;
    }

    // do the sorting
    const auto compare = (order == Qt::AscendingOrder ? &itemLessThan : &itemGreaterThan);
    std::stable_sort(sorting.begin(), sorting.end(), compare);

    QModelIndexList fromList;
    QModelIndexList toList;
    int colCount = columnCount();
    for (int r = 0; r < sorting.size(); ++r) {
        int oldRow = sorting.at(r).second;
        if (oldRow == r)
            continue;
        QTreeWidgetItem *item = sorting.at(r).first;
        items->replace(r, item);
        for (int c = 0; c < colCount; ++c) {
            QModelIndex from = createIndex(oldRow, c, item);
            if (static_cast<QAbstractItemModelPrivate *>(d_ptr.data())->persistent.indexes.contains(from)) {
                QModelIndex to = createIndex(r, c, item);
                fromList << from;
                toList << to;
            }
        }
    }
    changePersistentIndexList(fromList, toList);
}

void QTreeModel::timerEvent(QTimerEvent *ev)
{
    if (ev->timerId() == sortPendingTimer.timerId()) {
        executePendingSort();
    } else {
        QAbstractItemModel::timerEvent(ev);
    }
}

/*!
  \class QTreeWidgetItem

  \brief The QTreeWidgetItem class provides an item for use with the
  QTreeWidget convenience class.

  \ingroup model-view
  \inmodule QtWidgets

  Tree widget items are used to hold rows of information for tree widgets.
  Rows usually contain several columns of data, each of which can contain
  a text label and an icon.

  The QTreeWidgetItem class is a convenience class that replaces the
  QListViewItem class in Qt 3. It provides an item for use with
  the QTreeWidget class.

  Items are usually constructed with a parent that is either a QTreeWidget
  (for top-level items) or a QTreeWidgetItem (for items on lower levels of
  the tree). For example, the following code constructs a top-level item
  to represent cities of the world, and adds a entry for Oslo as a child
  item:

  \snippet qtreewidget-using/mainwindow.cpp 3

  Items can be added in a particular order by specifying the item they
  follow when they are constructed:

  \snippet qtreewidget-using/mainwindow.cpp 5

  Each column in an item can have its own background brush which is set with
  the setBackground() function. The current background brush can be
  found with background().
  The text label for each column can be rendered with its own font and brush.
  These are specified with the setFont() and setForeground() functions,
  and read with font() and foreground().

  The main difference between top-level items and those in lower levels of
  the tree is that a top-level item has no parent(). This information
  can be used to tell the difference between items, and is useful to know
  when inserting and removing items from the tree.
  Children of an item can be removed with takeChild() and inserted at a
  given index in the list of children with the insertChild() function.

  By default, items are enabled, selectable, checkable, and can be the source
  of a drag and drop operation.
  Each item's flags can be changed by calling setFlags() with the appropriate
  value (see \l{Qt::ItemFlags}). Checkable items can be checked and unchecked
  with the setCheckState() function. The corresponding checkState() function
  indicates whether the item is currently checked.

  \section1 Subclassing

  When subclassing QTreeWidgetItem to provide custom items, it is possible to
  define new types for them so that they can be distinguished from standard
  items. The constructors for subclasses that require this feature need to
  call the base class constructor with a new type value equal to or greater
  than \l UserType.

  \sa QTreeWidget, QTreeWidgetItemIterator, {Model/View Programming},
  QListWidgetItem, QTableWidgetItem
*/

/*!
    \enum QTreeWidgetItem::ItemType

    This enum describes the types that are used to describe tree widget items.

    \value Type     The default type for tree widget items.
    \value UserType The minimum value for custom types. Values below UserType are
                    reserved by Qt.

    You can define new user types in QTreeWidgetItem subclasses to ensure that
    custom items are treated specially; for example, when items are sorted.

    \sa type()
*/

/*!
    \fn int QTreeWidgetItem::type() const

    Returns the type passed to the QTreeWidgetItem constructor.
*/

/*!
    \fn void QTreeWidgetItem::sortChildren(int column, Qt::SortOrder order)
    \since 4.2

    Sorts the children of the item using the given \a order,
    by the values in the given \a column.

    \note This function does nothing if the item is not associated with a
    QTreeWidget.
*/

/*!
    \fn QTreeWidget *QTreeWidgetItem::treeWidget() const

    Returns the tree widget that contains the item.
*/

/*!
  \fn void QTreeWidgetItem::setSelected(bool select)
  \since 4.2

  Sets the selected state of the item to \a select.

  \sa isSelected()
*/
void QTreeWidgetItem::setSelected(bool select)
{
    const QTreeModel *model = treeModel();
    if (!model || !view->selectionModel())
        return;
    const QModelIndex index = model->index(this, 0);
    view->selectionModel()->select(index, (select ? QItemSelectionModel::Select
                                                  : QItemSelectionModel::Deselect)
                                   | QItemSelectionModel::Rows);
    d->selected = select;
}

/*!
  \fn bool QTreeWidgetItem::isSelected() const
  \since 4.2

  Returns \c true if the item is selected, otherwise returns \c false.

  \sa setSelected()
*/
bool QTreeWidgetItem::isSelected() const
{
    return d->selected;
}

/*!
  \fn void QTreeWidgetItem::setHidden(bool hide)
  \since 4.2

  Hides the item if \a hide is true, otherwise shows the item.
  \note A call to this function has no effect if the item is not currently in a view. In particular,
        calling \c setHidden(true) on an item and only then adding it to a view will result in
        a visible item.

  \sa isHidden()
*/

void QTreeWidgetItem::setHidden(bool hide)
{
    const QTreeModel *model = treeModel();
    if (!model)
        return;
    if (this == model->headerItem) {
        view->header()->setHidden(hide);
    } else {
        const QModelIndex index = view->d_func()->index(this);
        view->setRowHidden(index.row(), index.parent(), hide);
    }
    d->hidden = hide;
}

/*!
  \fn bool QTreeWidgetItem::isHidden() const
  \since 4.2

  Returns \c true if the item is hidden, otherwise returns \c false.

  \sa setHidden()
*/

bool QTreeWidgetItem::isHidden() const
{
    const QTreeModel *model = treeModel();
    if (!model)
        return false;
    if (this == model->headerItem)
        return view->header()->isHidden();
    if (view->d_func()->hiddenIndexes.isEmpty())
        return false;
    QTreeModel::SkipSorting skipSorting(model);
    return view->d_func()->isRowHidden(view->d_func()->index(this));
}

/*!
  \fn void QTreeWidgetItem::setExpanded(bool expand)
  \since 4.2

  Expands the item if \a expand is true, otherwise collapses the item.
  \warning The QTreeWidgetItem must be added to the QTreeWidget before calling this function.

  \sa isExpanded()
*/
void QTreeWidgetItem::setExpanded(bool expand)
{
    const QTreeModel *model = treeModel();
    if (!model)
        return;
    QTreeModel::SkipSorting skipSorting(model);
    view->setExpanded(view->d_func()->index(this), expand);
}

/*!
  \fn bool QTreeWidgetItem::isExpanded() const
  \since 4.2

  Returns \c true if the item is expanded, otherwise returns \c false.

  \sa setExpanded()
*/
bool QTreeWidgetItem::isExpanded() const
{
    const QTreeModel *model = treeModel();
    if (!model)
        return false;
    QTreeModel::SkipSorting skipSorting(model);
    return view->isExpanded(view->d_func()->index(this));
}

/*!
  \fn void QTreeWidgetItem::setFirstColumnSpanned(bool span)
  \since 4.3

  Sets the first section to span all columns if \a span is true;
  otherwise all item sections are shown.

  \sa isFirstColumnSpanned()
*/
void QTreeWidgetItem::setFirstColumnSpanned(bool span)
{
    const QTreeModel *model = treeModel();
    if (!model || this == model->headerItem)
        return; // We can't set the header items to spanning
    const QModelIndex index = model->index(this, 0);
    view->setFirstColumnSpanned(index.row(), index.parent(), span);
}

/*!
  \fn bool QTreeWidgetItem::isFirstColumnSpanned() const
  \since 4.3

  Returns \c true if the item is spanning all the columns in a row; otherwise returns \c false.

  \sa setFirstColumnSpanned()
*/
bool QTreeWidgetItem::isFirstColumnSpanned() const
{
    const QTreeModel *model = treeModel();
    if (!model || this == model->headerItem)
        return false;
    const QModelIndex index = model->index(this, 0);
    return view->isFirstColumnSpanned(index.row(), index.parent());
}

/*!
    \fn QString QTreeWidgetItem::text(int column) const

    Returns the text in the specified \a column.

    \sa setText()
*/

/*!
    \fn void QTreeWidgetItem::setText(int column, const QString &text)

    Sets the text to be displayed in the given \a column to the given \a text.

    \sa text(), setFont(), setForeground()
*/

/*!
    \fn QIcon QTreeWidgetItem::icon(int column) const

    Returns the icon that is displayed in the specified \a column.

    \sa setIcon(), {QAbstractItemView::iconSize}{iconSize}
*/

/*!
    \fn void QTreeWidgetItem::setIcon(int column, const QIcon &icon)

    Sets the icon to be displayed in the given \a column to \a icon.

    \sa icon(), setText(), {QAbstractItemView::iconSize}{iconSize}
*/

/*!
    \fn QString QTreeWidgetItem::statusTip(int column) const

    Returns the status tip for the contents of the given \a column.

    \sa setStatusTip()
*/

/*!
    \fn void QTreeWidgetItem::setStatusTip(int column, const QString &statusTip)

    Sets the status tip for the given \a column to the given \a statusTip.
    QTreeWidget mouse tracking needs to be enabled for this feature to work.

    \sa statusTip(), setToolTip(), setWhatsThis()
*/

/*!
    \fn QString QTreeWidgetItem::toolTip(int column) const

    Returns the tool tip for the given \a column.

    \sa setToolTip()
*/

/*!
    \fn void QTreeWidgetItem::setToolTip(int column, const QString &toolTip)

    Sets the tooltip for the given \a column to \a toolTip.

    \sa toolTip(), setStatusTip(), setWhatsThis()
*/

/*!
    \fn QString QTreeWidgetItem::whatsThis(int column) const

    Returns the "What's This?" help for the contents of the given \a column.

    \sa setWhatsThis()
*/

/*!
    \fn void QTreeWidgetItem::setWhatsThis(int column, const QString &whatsThis)

    Sets the "What's This?" help for the given \a column to \a whatsThis.

    \sa whatsThis(), setStatusTip(), setToolTip()
*/

/*!
    \fn QFont QTreeWidgetItem::font(int column) const

    Returns the font used to render the text in the specified \a column.

    \sa setFont()
*/

/*!
    \fn void QTreeWidgetItem::setFont(int column, const QFont &font)

    Sets the font used to display the text in the given \a column to the given
    \a font.

    \sa font(), setText(), setForeground()
*/

/*!
    \fn QBrush QTreeWidgetItem::background(int column) const
    \since 4.2

    Returns the brush used to render the background of the specified \a column.

    \sa foreground()
*/

/*!
    \fn void QTreeWidgetItem::setBackground(int column, const QBrush &brush)
    \since 4.2

    Sets the background brush of the label in the given \a column to the
    specified \a brush.
    Setting a default-constructed brush will let the view use the
    default color from the style.

    \note If \l{Qt Style Sheets} are used on the same widget as setBackground(),
    style sheets will take precedence if the settings conflict.

    \sa setForeground()
*/


/*!
    \fn QBrush QTreeWidgetItem::foreground(int column) const
    \since 4.2

    Returns the brush used to render the foreground (e.g. text) of the
    specified \a column.
    Setting a default-constructed brush will let the view use the
    default color from the style.

    \sa background()
*/

/*!
    \fn void QTreeWidgetItem::setForeground(int column, const QBrush &brush)
    \since 4.2

    Sets the foreground brush of the label in the given \a column to the
    specified \a brush.

    \sa setBackground()
*/

/*!
    \fn Qt::CheckState QTreeWidgetItem::checkState(int column) const

    Returns the check state of the label in the given \a column.

    \sa Qt::CheckState
*/

/*!
    \fn void QTreeWidgetItem::setCheckState(int column, Qt::CheckState state)

    Sets the item in the given \a column check state to be \a state.

    \sa checkState()
*/

/*!
  \fn QSize QTreeWidgetItem::sizeHint(int column) const
  \since 4.1

  Returns the size hint set for the tree item in the given
  \a column (see \l{QSize}).
*/

/*!
  \fn void QTreeWidgetItem::setSizeHint(int column, const QSize &size)
  \since 4.1

  Sets the size hint for the tree item in the given \a column to be \a size.
  If no size hint is set or \a size is invalid, the item
  delegate will compute the size hint based on the item data.
*/

/*!
    \fn QTreeWidgetItem *QTreeWidgetItem::parent() const

    Returns the item's parent.

    \sa child()
*/

/*!
    \fn QTreeWidgetItem *QTreeWidgetItem::child(int index) const

    Returns the item at the given \a index in the list of the item's children.

    \sa parent()
*/

/*!
    \fn int QTreeWidgetItem::childCount() const

    Returns the number of child items.
*/

/*!
    \fn int QTreeWidgetItem::columnCount() const

    Returns the number of columns in the item.
*/

/*!
  \if defined(qt7)

  \fn Qt::Alignment QTreeWidgetItem::textAlignment(int column) const

  Returns the text alignment for the label in the given \a column.

  \else

  \fn int QTreeWidgetItem::textAlignment(int column) const

  Returns the text alignment for the label in the given \a column.

  \note This function returns an int for historical reasons. It will
  be corrected to return Qt::Alignment in Qt 7.

  \sa Qt::Alignment

  \endif
*/

/*!
  \obsolete [6.4] Use the overload that takes a Qt::Alignment argument.

  \fn void QTreeWidgetItem::setTextAlignment(int column, int alignment)

  Sets the text alignment for the label in the given \a column to
  the \a alignment specified.

  \sa Qt::Alignment
*/

/*!
  \since 6.4

  \fn void QTreeWidgetItem::setTextAlignment(int column, Qt::Alignment alignment)

  Sets the text alignment for the label in the given \a column to
  the \a alignment specified.
*/

/*!
  \fn void QTreeWidgetItem::setTextAlignment(int column, Qt::AlignmentFlag alignment)
  \internal
*/

/*!
    \fn int QTreeWidgetItem::indexOfChild(QTreeWidgetItem *child) const

    Returns the index of the given \a child in the item's list of children.
*/

/*!
    Constructs a tree widget item of the specified \a type. The item
    must be inserted into a tree widget.

    \sa type()
*/
QTreeWidgetItem::QTreeWidgetItem(int type) : rtti(type), d(new QTreeWidgetItemPrivate(this)) { }

/*!
    Constructs a tree widget item of the specified \a type. The item
    must be inserted into a tree widget.
    The given list of \a strings will be set as the item text for each
    column in the item.

    \sa type()
*/
QTreeWidgetItem::QTreeWidgetItem(const QStringList &strings, int type)
    : rtti(type), d(new QTreeWidgetItemPrivate(this))
{
    for (int i = 0; i < strings.size(); ++i)
        setText(i, strings.at(i));
}

/*!
    \fn QTreeWidgetItem::QTreeWidgetItem(QTreeWidget *parent, int type)

    Constructs a tree widget item of the specified \a type and appends it
    to the items in the given \a parent.

    \sa type()
*/

QTreeWidgetItem::QTreeWidgetItem(QTreeWidget *treeview, int type)
    : rtti(type), d(new QTreeWidgetItemPrivate(this))
{
    // do not set this->view here otherwise insertChild() will fail
    if (QTreeModel *model = treeModel(treeview)) {
        model->rootItem->addChild(this);
        values.reserve(model->headerItem->columnCount());
    }
}

/*!
  \fn QTreeWidgetItem::QTreeWidgetItem(QTreeWidget *parent, const QStringList &strings, int type)

  Constructs a tree widget item of the specified \a type and appends it
  to the items in the given \a parent. The given list of \a strings will be set as
  the item text for each column in the item.

  \sa type()
*/

QTreeWidgetItem::QTreeWidgetItem(QTreeWidget *treeview, const QStringList &strings, int type)
    : rtti(type), d(new QTreeWidgetItemPrivate(this))
{
    for (int i = 0; i < strings.size(); ++i)
        setText(i, strings.at(i));
    // do not set this->view here otherwise insertChild() will fail
    if (QTreeModel *model = treeModel(treeview)) {
        model->rootItem->addChild(this);
        values.reserve(model->headerItem->columnCount());
    }
}

/*!
    \fn QTreeWidgetItem::QTreeWidgetItem(QTreeWidget *parent, QTreeWidgetItem *preceding, int type)

    Constructs a tree widget item of the specified \a type and inserts it into
    the given \a parent after the \a preceding item.

    \sa type()
*/
QTreeWidgetItem::QTreeWidgetItem(QTreeWidget *treeview, QTreeWidgetItem *after, int type)
    : rtti(type), d(new QTreeWidgetItemPrivate(this))
{
    // do not set this->view here otherwise insertChild() will fail
    if (QTreeModel *model = treeModel(treeview)) {
        int i = model->rootItem->children.indexOf(after) + 1;
        model->rootItem->insertChild(i, this);
        values.reserve(model->headerItem->columnCount());
    }
}

/*!
    Constructs a tree widget item and append it to the given \a parent.

    \sa type()
*/
QTreeWidgetItem::QTreeWidgetItem(QTreeWidgetItem *parent, int type)
    : rtti(type), d(new QTreeWidgetItemPrivate(this))
{
    if (parent)
        parent->addChild(this);
}

/*!
    Constructs a tree widget item and append it to the given \a parent.
    The given list of \a strings will be set as the item text for each column in the item.

    \sa type()
*/
QTreeWidgetItem::QTreeWidgetItem(QTreeWidgetItem *parent, const QStringList &strings, int type)
    : rtti(type), d(new QTreeWidgetItemPrivate(this))
{
    for (int i = 0; i < strings.size(); ++i)
        setText(i, strings.at(i));
    if (parent)
        parent->addChild(this);
}

/*!
    \fn QTreeWidgetItem::QTreeWidgetItem(QTreeWidgetItem *parent, QTreeWidgetItem *preceding, int type)

    Constructs a tree widget item of the specified \a type that is inserted
    into the \a parent after the \a preceding child item.

    \sa type()
*/
QTreeWidgetItem::QTreeWidgetItem(QTreeWidgetItem *parent, QTreeWidgetItem *after, int type)
    : rtti(type), d(new QTreeWidgetItemPrivate(this))
{
    if (parent) {
        int i = parent->children.indexOf(after) + 1;
        parent->insertChild(i, this);
    }
}

/*!
  Destroys this tree widget item.

  The item will be removed from \l{QTreeWidget}s to which it has
  been added. This makes it safe to delete an item at any time.

*/

QTreeWidgetItem::~QTreeWidgetItem()
{
    QTreeModel *model = treeModel();
    QTreeModel::SkipSorting skipSorting(model);

    if (par) {
        int i = par->children.indexOf(this);
        if (i >= 0) {
            if (model) model->beginRemoveItems(par, i, 1);
            // users _could_ do changes when connected to rowsAboutToBeRemoved,
            // so we check again to make sure 'i' is valid
            if (!par->children.isEmpty() && par->children.at(i) == this)
                par->children.takeAt(i);
            if (model) model->endRemoveItems();
        }
    } else if (model) {
        if (this == model->headerItem) {
            model->headerItem = nullptr;
        } else {
            int i = model->rootItem->children.indexOf(this);
            if (i >= 0) {
                model->beginRemoveItems(nullptr, i, 1);
                // users _could_ do changes when connected to rowsAboutToBeRemoved,
                // so we check again to make sure 'i' is valid
                if (!model->rootItem->children.isEmpty() && model->rootItem->children.at(i) == this)
                    model->rootItem->children.takeAt(i);
                model->endRemoveItems();
            }
        }
    }
    // at this point the persistent indexes for the children should also be invalidated
    // since we invalidated the parent
    for (int i = 0; i < children.size(); ++i) {
        QTreeWidgetItem *child = children.at(i);
        // make sure the child does not try to remove itself from our children list
        child->par = nullptr;
        // make sure the child does not try to remove itself from the top level list
        child->view = nullptr;
        delete child;
    }

    children.clear();
    delete d;
}

/*!
    Creates a deep copy of the item and of its children.
*/
QTreeWidgetItem *QTreeWidgetItem::clone() const
{
    QTreeWidgetItem *copy = nullptr;

    QStack<const QTreeWidgetItem*> stack;
    QStack<QTreeWidgetItem*> parentStack;
    stack.push(this);
    parentStack.push(0);

    QTreeWidgetItem *root = nullptr;
    const QTreeWidgetItem *item = nullptr;
    QTreeWidgetItem *parent = nullptr;
    while (!stack.isEmpty()) {
        // get current item, and copied parent
        item = stack.pop();
        parent = parentStack.pop();

        // copy item
        copy = new QTreeWidgetItem(*item);
        if (!root)
            root = copy;

        // set parent and add to parents children list
        if (parent) {
            copy->par = parent;
            parent->children.insert(0, copy);
        }

        for (int i = 0; i < item->childCount(); ++i) {
            stack.push(item->child(i));
            parentStack.push(copy);
        }
    }
    return root;
}

/*!
   Sets the item indicator \a policy. This policy decides when the
   tree branch expand/collapse indicator is shown.
   The default value is DontShowIndicatorWhenChildless.

   \sa childIndicatorPolicy()
*/
void QTreeWidgetItem::setChildIndicatorPolicy(QTreeWidgetItem::ChildIndicatorPolicy policy)
{
    if (d->policy == policy)
        return;
    d->policy = policy;

    if (!view)
        return;

    view->scheduleDelayedItemsLayout();
}

/*!
   Returns the item indicator policy. This policy decides when the
   tree branch expand/collapse indicator is shown.

   \sa setChildIndicatorPolicy()
*/
QTreeWidgetItem::ChildIndicatorPolicy QTreeWidgetItem::childIndicatorPolicy() const
{
    return d->policy;
}

/*!
    \fn void QTreeWidgetItem::setFlags(Qt::ItemFlags flags)

    Sets the flags for the item to the given \a flags. These determine whether
    the item can be selected or modified.  This is often used to disable an item.

    \sa flags()
*/
void QTreeWidgetItem::setFlags(Qt::ItemFlags flags)
{
    const bool enable = (flags & Qt::ItemIsEnabled);
    const bool changedState = bool(itemFlags & Qt::ItemIsEnabled) != enable;
    const bool changedExplicit = d->disabled != !enable;

    d->disabled = !enable;

    if (enable && par && !(par->itemFlags & Qt::ItemIsEnabled)) // inherit from parent
        itemFlags = flags & ~Qt::ItemIsEnabled;
    else // this item is explicitly disabled or has no parent
        itemFlags = flags;

    if (changedState && changedExplicit) { // if the propagate the change to the children
        QStack<QTreeWidgetItem*> parents;
        parents.push(this);
        while (!parents.isEmpty()) {
            QTreeWidgetItem *parent = parents.pop();
            for (int i = 0; i < parent->children.size(); ++i) {
                QTreeWidgetItem *child = parent->children.at(i);
                if (!child->d->disabled) { // if not explicitly disabled
                    parents.push(child);
                    if (enable)
                        child->itemFlags = child->itemFlags | Qt::ItemIsEnabled;
                    else
                        child->itemFlags = child->itemFlags & ~Qt::ItemIsEnabled;
                    child->itemChanged(); // ### we may want to optimize this
                }
            }
        }
    }
    itemChanged();
}

void QTreeWidgetItemPrivate::updateHiddenStatus(QTreeWidgetItem *item, bool inserting)
{
    QTreeModel *model = item->treeModel();
    if (!model)
        return;
    QStack<QTreeWidgetItem *> parents;
    parents.push(item);
    while (!parents.isEmpty()) {
        QTreeWidgetItem *parent = parents.pop();
        if (parent->d->hidden) {
            const QModelIndex index = model->index(parent, 0);
            item->view->setRowHidden(index.row(), index.parent(), inserting);
        }
        for (int i = 0; i < parent->children.size(); ++i) {
            QTreeWidgetItem *child = parent->children.at(i);
            parents.push(child);
        }
    }
}

void QTreeWidgetItemPrivate::propagateDisabled(QTreeWidgetItem *item)
{
    Q_ASSERT(item);
    const bool enable = item->par ? (item->par->itemFlags.testFlag(Qt::ItemIsEnabled)) : true;

    QStack<QTreeWidgetItem*> parents;
    parents.push(item);
    while (!parents.isEmpty()) {
        QTreeWidgetItem *parent = parents.pop();
        if (!parent->d->disabled) { // if not explicitly disabled
            Qt::ItemFlags oldFlags = parent->itemFlags;
            if (enable)
                parent->itemFlags = parent->itemFlags | Qt::ItemIsEnabled;
            else
                parent->itemFlags = parent->itemFlags & ~Qt::ItemIsEnabled;
            if (parent->itemFlags != oldFlags)
                parent->itemChanged();
        }

        for (int i = 0; i < parent->children.size(); ++i) {
            QTreeWidgetItem *child = parent->children.at(i);
            parents.push(child);
        }
    }
}
/*!
    \fn Qt::ItemFlags QTreeWidgetItem::flags() const

    Returns the flags used to describe the item. These determine whether
    the item can be checked, edited, and selected.

    The default value for flags is
    Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled |
    Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled.

    \sa setFlags()
*/
Qt::ItemFlags QTreeWidgetItem::flags() const
{
    return itemFlags;
}

/*!
    Sets the value for the item's \a column and \a role to the given
    \a value.

    The \a role describes the type of data specified by \a value, and is defined by
    the Qt::ItemDataRole enum.

    \note The default implementation treats Qt::EditRole and Qt::DisplayRole as
    referring to the same data.
*/
void QTreeWidgetItem::setData(int column, int role, const QVariant &value)
{
    if (column < 0)
        return;

    QTreeModel *model = treeModel();
    switch (role) {
    case Qt::EditRole:
    case Qt::DisplayRole: {
        if (values.size() <= column) {
            if (model && this == model->headerItem)
                model->setColumnCount(column + 1);
            else
                values.resize(column + 1);
        }
        if (d->display.size() <= column) {
            for (int i = d->display.size() - 1; i < column - 1; ++i)
                d->display.append(QVariant());
            d->display.append(value);
        } else if (d->display[column] != value) {
            d->display[column] = value;
        } else {
            return; // value is unchanged
        }
    } break;
    case Qt::CheckStateRole:
        if ((itemFlags & Qt::ItemIsAutoTristate) && value != Qt::PartiallyChecked) {
            for (int i = 0; i < children.size(); ++i) {
                QTreeWidgetItem *child = children.at(i);
                if (child->data(column, role).isValid()) {// has a CheckState
                    Qt::ItemFlags f = itemFlags; // a little hack to avoid multiple dataChanged signals
                    itemFlags &= ~Qt::ItemIsAutoTristate;
                    child->setData(column, role, value);
                    itemFlags = f;
                }
            }
        }
        Q_FALLTHROUGH();
    default:
        if (column < values.size()) {
            bool found = false;
            const QList<QWidgetItemData> column_values = values.at(column);
            for (int i = 0; i < column_values.size(); ++i) {
                if (column_values.at(i).role == role) {
                    if (column_values.at(i).value == value)
                        return; // value is unchanged
                    values[column][i].value = value;
                    found = true;
                    break;
                }
            }
            if (!found)
                values[column].append(QWidgetItemData(role, value));
        } else {
            if (model && this == model->headerItem)
                model->setColumnCount(column + 1);
            else
                values.resize(column + 1);
            values[column].append(QWidgetItemData(role, value));
        }
    }

    if (model) {
        const QList<int> roles((role == Qt::DisplayRole || role == Qt::EditRole)
                                       ? QList<int>({ Qt::DisplayRole, Qt::EditRole })
                                       : QList<int>({ role }));
        model->emitDataChanged(this, column, roles);
        if (role == Qt::CheckStateRole) {
            QTreeWidgetItem *p;
            for (p = par; p && (p->itemFlags & Qt::ItemIsAutoTristate); p = p->par)
                model->emitDataChanged(p, column, roles);
        }
    }
}

/*!
    Returns the value for the item's \a column and \a role.
*/
QVariant QTreeWidgetItem::data(int column, int role) const
{
    switch (role) {
    case Qt::EditRole:
    case Qt::DisplayRole:
        if (column >= 0 && column < d->display.size())
            return d->display.at(column);
        break;
    case Qt::CheckStateRole:
        // special case for check state in tristate
        if (children.size() && (itemFlags & Qt::ItemIsAutoTristate))
            return childrenCheckState(column);
        Q_FALLTHROUGH();
   default:
        if (column >= 0 && column < values.size()) {
            const QList<QWidgetItemData> &column_values = values.at(column);
            for (const auto &column_value : column_values) {
                if (column_value.role == role)
                    return column_value.value;
            }
        }
    }
    return QVariant();
}

/*!
  Returns \c true if the text in the item is less than the text in the
  \a other item, otherwise returns \c false.
*/

bool QTreeWidgetItem::operator<(const QTreeWidgetItem &other) const
{
    int column = view ? view->sortColumn() : 0;
    const QVariant v1 = data(column, Qt::DisplayRole);
    const QVariant v2 = other.data(column, Qt::DisplayRole);
    return QAbstractItemModelPrivate::variantLessThan(v1, v2);
}

#ifndef QT_NO_DATASTREAM

/*!
    Reads the item from stream \a in. This only reads data into a single item.

    \sa write()
*/
void QTreeWidgetItem::read(QDataStream &in)
{
    // convert from streams written before we introduced display (4.2.0)
    if (in.version() < QDataStream::Qt_4_2) {
        d->display.clear();
        in >> values;
        // move the display value over to the display string list
        for (int column = 0; column < values.size(); ++column) {
            d->display << QVariant();
            for (int i = 0; i < values.at(column).size(); ++i) {
                if (values.at(column).at(i).role == Qt::DisplayRole) {
                    d->display[column] = values.at(column).at(i).value;
                    values[column].remove(i--);
                }
            }
        }
    } else {
        in >> values >> d->display;
    }
}

/*!
    Writes the item to stream \a out. This only writes data from one single item.

    \sa read()
*/
void QTreeWidgetItem::write(QDataStream &out) const
{
    out << values << d->display;
}
#endif // QT_NO_DATASTREAM

/*!
    \since 4.1

    Constructs a copy of \a other. Note that type() and treeWidget()
    are not copied.

    This function is useful when reimplementing clone().

    \sa data(), flags()
*/
QTreeWidgetItem::QTreeWidgetItem(const QTreeWidgetItem &other)
    : rtti(Type),
      values(other.values),
      d(new QTreeWidgetItemPrivate(this)),
      itemFlags(other.itemFlags)
{
    d->display = other.d->display;
}

/*!
    Assigns \a other's data and flags to this item. Note that type()
    and treeWidget() are not copied.

    This function is useful when reimplementing clone().

    \sa data(), flags()
*/
QTreeWidgetItem &QTreeWidgetItem::operator=(const QTreeWidgetItem &other)
{
    values = other.values;
    d->display = other.d->display;
    d->policy = other.d->policy;
    itemFlags = other.itemFlags;
    return *this;
}

/*!
  Appends the \a child item to the list of children.

  \sa insertChild(), takeChild()
*/
void QTreeWidgetItem::addChild(QTreeWidgetItem *child)
{
    if (child) {
        insertChild(children.size(), child);
        child->d->rowGuess = children.size() - 1;
    }
}

/*!
  Inserts the \a child item at \a index in the list of children.

  If the child has already been inserted somewhere else it won't be inserted again.
*/
void QTreeWidgetItem::insertChild(int index, QTreeWidgetItem *child)
{
    if (index < 0 || index > children.size() || child == nullptr || child->view != nullptr || child->par != nullptr)
        return;

    if (QTreeModel *model = treeModel()) {
        QTreeModel::SkipSorting skipSorting(model);
        if (model->rootItem == this)
            child->par = nullptr;
        else
            child->par = this;
        if (view->isSortingEnabled()) {
            // do a delayed sort instead
            if (!model->sortPendingTimer.isActive())
                model->sortPendingTimer.start(0, model);
        }
        model->beginInsertItems(this, index, 1);
        int cols = model->columnCount();
        QStack<QTreeWidgetItem*> stack;
        stack.push(child);
        while (!stack.isEmpty()) {
            QTreeWidgetItem *i = stack.pop();
            i->view = view;
            i->values.reserve(cols);
            for (int c = 0; c < i->children.size(); ++c)
                stack.push(i->children.at(c));
        }
        children.insert(index, child);
        d->updateHiddenStatus(child, true);
        model->endInsertItems();
    } else {
        child->par = this;
        children.insert(index, child);
    }
    if (child->par)
        d->propagateDisabled(child);
}

/*!
  Removes the given item indicated by \a child.
  The removed item will not be deleted.
*/
void QTreeWidgetItem::removeChild(QTreeWidgetItem *child)
{
    (void)takeChild(children.indexOf(child));
}

/*!
  Removes the item at \a index and returns it, otherwise return 0.
*/
QTreeWidgetItem *QTreeWidgetItem::takeChild(int index)
{
    // we move this outside the check of the index to allow executing
    // pending sorts from inline functions, using this function (hack)
    QTreeModel *model = treeModel();
    if (model) {
        // This will trigger a layoutChanged signal, thus we might want to optimize
        // this function by not emitting the rowsRemoved signal etc to the view.
        // On the other hand we also need to make sure that the selectionmodel
        // is updated in case we take an item that is selected.
        model->skipPendingSort = false;
        model->executePendingSort();
    }
    if (index >= 0 && index < children.size()) {
        if (model) model->beginRemoveItems(this, index, 1);
        d->updateHiddenStatus(children.at(index), false);
        QTreeWidgetItem *item = children.takeAt(index);
        item->par = nullptr;
        QStack<QTreeWidgetItem*> stack;
        stack.push(item);
        while (!stack.isEmpty()) {
            QTreeWidgetItem *i = stack.pop();
            i->view = nullptr;
            for (int c = 0; c < i->children.size(); ++c)
                stack.push(i->children.at(c));
        }
        d->propagateDisabled(item);
        if (model) model->endRemoveRows();
        return item;
    }
    return nullptr;
}

/*!
  \since 4.1

  Appends the given list of \a children to the item.

  \sa insertChildren(), takeChildren()
*/
void QTreeWidgetItem::addChildren(const QList<QTreeWidgetItem*> &children)
{
    insertChildren(this->children.size(), children);
}

/*!
  \since 4.1

  Inserts the given list of \a children into the list of the item children at \a index .

  Children that have already been inserted somewhere else won't be inserted.
*/
void QTreeWidgetItem::insertChildren(int index, const QList<QTreeWidgetItem*> &children)
{
    if (index < 0 || index > this->children.size() || children.isEmpty())
        return;

    if (view && view->isSortingEnabled()) {
        for (int n = 0; n < children.size(); ++n)
            insertChild(index, children.at(n));
        return;
    }
    QTreeModel *model = treeModel();
    QStack<QTreeWidgetItem*> stack;
    QList<QTreeWidgetItem*> itemsToInsert;
    for (int n = 0; n < children.size(); ++n) {
        QTreeWidgetItem *child = children.at(n);
        if (child->view || child->par)
            continue;
        itemsToInsert.append(child);
        if (view && model) {
            if (child->childCount() == 0)
                child->view = view;
            else
                stack.push(child);
        }
        if (model && (model->rootItem == this))
            child->par = nullptr;
        else
            child->par = this;
    }
    if (!itemsToInsert.isEmpty()) {
        while (!stack.isEmpty()) {
            QTreeWidgetItem *i = stack.pop();
            i->view = view;
            for (int c = 0; c < i->children.size(); ++c)
                stack.push(i->children.at(c));
        }
        if (model) model->beginInsertItems(this, index, itemsToInsert.size());
        for (int n = 0; n < itemsToInsert.size(); ++n) {
            QTreeWidgetItem *child = itemsToInsert.at(n);
            this->children.insert(index + n, child);
            if (child->par)
                d->propagateDisabled(child);
            d->updateHiddenStatus(child, true);
        }
        if (model) model->endInsertItems();
    }
}

/*!
  \since 4.1

  Removes the list of children and returns it, otherwise returns an empty list.
*/
QList<QTreeWidgetItem*> QTreeWidgetItem::takeChildren()
{
    QList<QTreeWidgetItem*> removed;
    if (children.size() > 0) {
        QTreeModel *model = treeModel();
        if (model) {
            // This will trigger a layoutChanged signal, thus we might want to optimize
            // this function by not emitting the rowsRemoved signal etc to the view.
            // On the other hand we also need to make sure that the selectionmodel
            // is updated in case we take an item that is selected.
            model->executePendingSort();
        }
        if (model) model->beginRemoveItems(this, 0, children.size());
        for (int n = 0; n < children.size(); ++n) {
            QTreeWidgetItem *item = children.at(n);
            item->par = nullptr;
            QStack<QTreeWidgetItem*> stack;
            stack.push(item);
            while (!stack.isEmpty()) {
                QTreeWidgetItem *i = stack.pop();
                i->view = nullptr;
                for (int c = 0; c < i->children.size(); ++c)
                    stack.push(i->children.at(c));
            }
            d->propagateDisabled(item);
        }
        removed = children;
        children.clear(); // detach
        if (model) model->endRemoveItems();
    }
    return removed;
}


void QTreeWidgetItemPrivate::sortChildren(int column, Qt::SortOrder order, bool climb)
{
    QTreeModel *model = q->treeModel();
    if (!model)
        return;
    model->sortItems(&q->children, column, order);
    if (climb) {
        QList<QTreeWidgetItem*>::iterator it = q->children.begin();
        for (; it != q->children.end(); ++it) {
            //here we call the private object's method to avoid emitting
            //the layoutAboutToBeChanged and layoutChanged signals
            (*it)->d->sortChildren(column, order, climb);
        }
    }
}

/*!
  \internal

  Sorts the children by the value in the given \a column, in the \a order
  specified. If \a climb is true, the items below each of the children will
  also be sorted.
*/
void QTreeWidgetItem::sortChildren(int column, Qt::SortOrder order, bool climb)
{
    QTreeModel *model = treeModel();
    if (!model)
        return;
    if (model->isChanging())
        return;
    QTreeModel::SkipSorting skipSorting(model);
    int oldSortColumn = view->d_func()->explicitSortColumn;
    view->d_func()->explicitSortColumn = column;
    emit model->layoutAboutToBeChanged({}, QAbstractItemModel::VerticalSortHint);
    d->sortChildren(column, order, climb);
    emit model->layoutChanged({}, QAbstractItemModel::VerticalSortHint);
    view->d_func()->explicitSortColumn = oldSortColumn;
}

/*!
  \internal

  Calculates the checked state of the item based on the checked state
  of its children. E.g. if all children checked => this item is also
  checked; if some children checked => this item is partially checked;
  if no children checked => this item is unchecked.
*/
QVariant QTreeWidgetItem::childrenCheckState(int column) const
{
    if (column < 0)
        return QVariant();
    bool checkedChildren = false;
    bool uncheckedChildren = false;
    for (const auto *child : children) {
        QVariant value = child->data(column, Qt::CheckStateRole);
        if (!value.isValid())
            return QVariant();

        switch (static_cast<Qt::CheckState>(value.toInt()))
        {
        case Qt::Unchecked:
            uncheckedChildren = true;
            break;
        case Qt::Checked:
            checkedChildren = true;
            break;
        case Qt::PartiallyChecked:
        default:
            return Qt::PartiallyChecked;
        }

        if (uncheckedChildren && checkedChildren)
            return Qt::PartiallyChecked;
    }

    if (uncheckedChildren)
        return Qt::Unchecked;
    else if (checkedChildren)
        return Qt::Checked;
    else
        return QVariant(); // value was not defined
}

/*!
  \since 4.5

  Causes the model associated with this item to emit a
  \l{QAbstractItemModel::dataChanged()}{dataChanged}() signal for this
  item.

  You normally only need to call this function if you have subclassed
  QTreeWidgetItem and reimplemented data() and/or setData().

  \sa setData()
*/
void QTreeWidgetItem::emitDataChanged()
{
    itemChanged();
}

/*!
  \internal
*/
void QTreeWidgetItem::itemChanged()
{
    if (QTreeModel *model = treeModel())
        model->itemChanged(this);
}

/*!
  \internal
*/
void QTreeWidgetItem::executePendingSort() const
{
    if (QTreeModel *model = treeModel())
        model->executePendingSort();
}

/*!
  \internal
  returns the QTreeModel if a view is set
*/
QTreeModel *QTreeWidgetItem::treeModel(QTreeWidget *v) const
{
    if (!v)
        v = view;
    return (v ? qobject_cast<QTreeModel*>(v->model()) : nullptr);
}


#ifndef QT_NO_DATASTREAM
/*!
    \relates QTreeWidgetItem

    Writes the tree widget item \a item to stream \a out.

    This operator uses QTreeWidgetItem::write().

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator<<(QDataStream &out, const QTreeWidgetItem &item)
{
    item.write(out);
    return out;
}

/*!
    \relates QTreeWidgetItem

    Reads a tree widget item from stream \a in into \a item.

    This operator uses QTreeWidgetItem::read().

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator>>(QDataStream &in, QTreeWidgetItem &item)
{
    item.read(in);
    return in;
}
#endif // QT_NO_DATASTREAM


void QTreeWidgetPrivate::clearConnections()
{
    for (const QMetaObject::Connection &connection : connections)
        QObject::disconnect(connection);
}

void QTreeWidgetPrivate::emitItemPressed(const QModelIndex &index)
{
    Q_Q(QTreeWidget);
    emit q->itemPressed(item(index), index.column());
}

void QTreeWidgetPrivate::emitItemClicked(const QModelIndex &index)
{
    Q_Q(QTreeWidget);
    emit q->itemClicked(item(index), index.column());
}

void QTreeWidgetPrivate::emitItemDoubleClicked(const QModelIndex &index)
{
    Q_Q(QTreeWidget);
    emit q->itemDoubleClicked(item(index), index.column());
}

void QTreeWidgetPrivate::emitItemActivated(const QModelIndex &index)
{
    Q_Q(QTreeWidget);
    emit q->itemActivated(item(index), index.column());
}

void QTreeWidgetPrivate::emitItemEntered(const QModelIndex &index)
{
    Q_Q(QTreeWidget);
    emit q->itemEntered(item(index), index.column());
}

void QTreeWidgetPrivate::emitItemChanged(const QModelIndex &index)
{
    Q_Q(QTreeWidget);
    QTreeWidgetItem *indexItem = item(index);
    if (indexItem)
        emit q->itemChanged(indexItem, index.column());
}

void QTreeWidgetPrivate::emitItemExpanded(const QModelIndex &index)
{
    Q_Q(QTreeWidget);
    emit q->itemExpanded(item(index));
}

void QTreeWidgetPrivate::emitItemCollapsed(const QModelIndex &index)
{
    Q_Q(QTreeWidget);
    emit q->itemCollapsed(item(index));
}

void QTreeWidgetPrivate::emitCurrentItemChanged(const QModelIndex &current,
                                                const QModelIndex &previous)
{
    Q_Q(QTreeWidget);
    QTreeWidgetItem *currentItem = item(current);
    QTreeWidgetItem *previousItem = item(previous);
    emit q->currentItemChanged(currentItem, previousItem);
}

void QTreeWidgetPrivate::sort()
{
    if (sortingEnabled) {
        int column = header->sortIndicatorSection();
        Qt::SortOrder order = header->sortIndicatorOrder();
        treeModel()->sort(column, order);
    }
}

void QTreeWidgetPrivate::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_Q(QTreeWidget);
    QModelIndexList indices = selected.indexes();
    int i;
    QTreeModel *m = treeModel();
    for (i = 0; i < indices.size(); ++i) {
        QTreeWidgetItem *item = m->item(indices.at(i));
        item->d->selected = true;
    }

    indices = deselected.indexes();
    for (i = 0; i < indices.size(); ++i) {
        QTreeWidgetItem *item = m->item(indices.at(i));
        item->d->selected = false;
    }

    emit q->itemSelectionChanged();
}

void QTreeWidgetPrivate::dataChanged(const QModelIndex &topLeft,
                                     const QModelIndex &bottomRight)
{
    if (sortingEnabled && topLeft.isValid() && bottomRight.isValid()
        && !treeModel()->sortPendingTimer.isActive()) {
        int column = header->sortIndicatorSection();
        if (column >= topLeft.column() && column <= bottomRight.column()) {
            Qt::SortOrder order = header->sortIndicatorOrder();
            treeModel()->ensureSorted(column, order, topLeft.row(),
                                  bottomRight.row(), topLeft.parent());
        }
    }
}

/*!
  \class QTreeWidget

  \brief The QTreeWidget class provides a tree view that uses a predefined
  tree model.

  \ingroup model-view
  \inmodule QtWidgets

  \image windows-treeview.png

  The QTreeWidget class is a convenience class that provides a standard
  tree widget with a classic item-based interface similar to that used by
  the QListView class in Qt 3.
  This class is based on Qt's Model/View architecture and uses a default
  model to hold items, each of which is a QTreeWidgetItem.

  Developers who do not need the flexibility of the Model/View framework
  can use this class to create simple hierarchical lists very easily. A more
  flexible approach involves combining a QTreeView with a standard item model.
  This allows the storage of data to be separated from its representation.

  In its simplest form, a tree widget can be constructed in the following way:

  \snippet code/src_gui_itemviews_qtreewidget.cpp 0

  Before items can be added to the tree widget, the number of columns must
  be set with setColumnCount(). This allows each item to have one or more
  labels or other decorations. The number of columns in use can be found
  with the columnCount() function.

  The tree can have a header that contains a section for each column in
  the widget. It is easiest to set up the labels for each section by
  supplying a list of strings with setHeaderLabels(), but a custom header
  can be constructed with a QTreeWidgetItem and inserted into the tree
  with the setHeaderItem() function.

  The items in the tree can be sorted by column according to a predefined
  sort order. If sorting is enabled, the user can sort the items by clicking
  on a column header. Sorting can be enabled or disabled by calling
  \l{QTreeView::setSortingEnabled()}{setSortingEnabled()}. The
  \l{QTreeView::isSortingEnabled()}{isSortingEnabled()} function indicates
  whether sorting is enabled.

  \sa QTreeWidgetItem, QTreeWidgetItemIterator, QTreeView,
  {Model/View Programming}
*/

/*!
    \property QTreeWidget::columnCount
    \brief the number of columns displayed in the tree widget

    By default, this property has a value of 1.
*/

/*!
    \fn void QTreeWidget::itemActivated(QTreeWidgetItem *item, int column)

    This signal is emitted when the user activates an item by single-
    or double-clicking (depending on the platform, i.e. on the
    QStyle::SH_ItemView_ActivateItemOnSingleClick style hint) or
    pressing a special key (e.g., \uicontrol Enter).

    The specified \a item is the item that was clicked, or \nullptr if
    no item was clicked. The \a column is the item's column that was
    clicked, or -1 if no item was clicked.
*/

/*!
    \fn void QTreeWidget::itemPressed(QTreeWidgetItem *item, int column)

    This signal is emitted when the user presses a mouse button inside
    the widget.

    The specified \a item is the item that was clicked, or \nullptr if
    no item was clicked. The \a column is the item's column that was
    clicked, or -1 if no item was clicked.
*/

/*!
    \fn void QTreeWidget::itemClicked(QTreeWidgetItem *item, int column)

    This signal is emitted when the user clicks inside the widget.

    The specified \a item is the item that was clicked. The \a column is the
    item's column that was clicked. If no item was clicked, no signal will be
    emitted.
*/

/*!
    \fn void QTreeWidget::itemDoubleClicked(QTreeWidgetItem *item, int column)

    This signal is emitted when the user double clicks inside the
    widget.

    The specified \a item is the item that was clicked, or \nullptr if
    no item was clicked. The \a column is the item's column that was
    clicked. If no item was double clicked, no signal will be emitted.
*/

/*!
    \fn void QTreeWidget::itemExpanded(QTreeWidgetItem *item)

    This signal is emitted when the specified \a item is expanded so that
    all of its children are displayed.

    \sa QTreeWidgetItem::isExpanded(), itemCollapsed(), expandItem()
*/

/*!
    \fn void QTreeWidget::itemCollapsed(QTreeWidgetItem *item)

    This signal is emitted when the specified \a item is collapsed so that
    none of its children are displayed.

    \note This signal will not be emitted if an item changes its state when
    collapseAll() is invoked.

    \sa QTreeWidgetItem::isExpanded(), itemExpanded(), collapseItem()
*/

/*!
    \fn void QTreeWidget::currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)

    This signal is emitted when the current item changes. The current
    item is specified by \a current, and this replaces the \a previous
    current item.

    \sa setCurrentItem()
*/

/*!
    \fn void QTreeWidget::itemSelectionChanged()

    This signal is emitted when the selection changes in the tree widget.
    The current selection can be found with selectedItems().
*/

/*!
    \fn void QTreeWidget::itemEntered(QTreeWidgetItem *item, int column)

    This signal is emitted when the mouse cursor enters an \a item over the
    specified \a column.
    QTreeWidget mouse tracking needs to be enabled for this feature to work.
*/

/*!
    \fn void QTreeWidget::itemChanged(QTreeWidgetItem *item, int column)

    This signal is emitted when the contents of the \a column in the specified
    \a item changes.
*/

/*!
  \since 4.3

  \fn void QTreeWidget::removeItemWidget(QTreeWidgetItem *item, int column)

  Removes the widget set in the given \a item in the given \a column.

  \sa itemWidget(), setItemWidget()
*/

/*!
  Constructs a tree widget with the given \a parent.
*/
QTreeWidget::QTreeWidget(QWidget *parent)
    : QTreeView(*new QTreeWidgetPrivate(), parent)
{
    Q_D(QTreeWidget);
    QTreeView::setModel(new QTreeModel(1, this));
    d->connections = {
        QObjectPrivate::connect(this, &QTreeWidget::pressed,
                                d, &QTreeWidgetPrivate::emitItemPressed),
        QObjectPrivate::connect(this, &QTreeWidget::clicked,
                                d, &QTreeWidgetPrivate::emitItemClicked),
        QObjectPrivate::connect(this, &QTreeWidget::doubleClicked,
                                d, &QTreeWidgetPrivate::emitItemDoubleClicked),
        QObjectPrivate::connect(this, &QTreeWidget::activated,
                                d, &QTreeWidgetPrivate::emitItemActivated),
        QObjectPrivate::connect(this, &QTreeWidget::entered,
                                d, &QTreeWidgetPrivate::emitItemEntered),
        QObjectPrivate::connect(this, &QTreeWidget::expanded,
                                d, &QTreeWidgetPrivate::emitItemExpanded),
        QObjectPrivate::connect(this, &QTreeWidget::collapsed,
                                d, &QTreeWidgetPrivate::emitItemCollapsed),
        QObjectPrivate::connect(model(), &QAbstractItemModel::dataChanged,
                                d, &QTreeWidgetPrivate::emitItemChanged),
        QObjectPrivate::connect(model(), &QAbstractItemModel::dataChanged,
                                d, &QTreeWidgetPrivate::dataChanged),
        QObjectPrivate::connect(model(), &QAbstractItemModel::columnsRemoved,
                                d, &QTreeWidgetPrivate::sort),
        QObjectPrivate::connect(selectionModel(), &QItemSelectionModel::currentChanged,
                                d, &QTreeWidgetPrivate::emitCurrentItemChanged),
        QObjectPrivate::connect(selectionModel(), &QItemSelectionModel::selectionChanged,
                                d, &QTreeWidgetPrivate::selectionChanged)
    };
    header()->setSectionsClickable(false);
}

/*!
    Destroys the tree widget and all its items.
*/

QTreeWidget::~QTreeWidget()
{
    Q_D(QTreeWidget);
    d->clearConnections();
}

/*
  Returns the number of header columns in the view.

  \sa sortColumn(), currentColumn(), topLevelItemCount()
*/

int QTreeWidget::columnCount() const
{
    Q_D(const QTreeWidget);
    return d->model->columnCount();
}

/*
  Sets the number of header \a columns in the tree widget.
*/

void QTreeWidget::setColumnCount(int columns)
{
    Q_D(QTreeWidget);
    if (columns < 0)
        return;
    d->treeModel()->setColumnCount(columns);
}

/*!
  \since 4.2

  Returns the tree widget's invisible root item.

  The invisible root item provides access to the tree widget's top-level items
  through the QTreeWidgetItem API, making it possible to write functions that
  can treat top-level items and their children in a uniform way; for example,
  recursive functions.
*/

QTreeWidgetItem *QTreeWidget::invisibleRootItem() const
{
    Q_D(const QTreeWidget);
    return d->treeModel()->rootItem;
}

/*!
  Returns the top level item at the given \a index, or \nullptr if the
  item does not exist.

  \sa topLevelItemCount(), insertTopLevelItem()
*/

QTreeWidgetItem *QTreeWidget::topLevelItem(int index) const
{
    Q_D(const QTreeWidget);
    return d->treeModel()->rootItem->child(index);
}

/*!
    \property QTreeWidget::topLevelItemCount
    \brief the number of top-level items

    By default, this property has a value of 0.

    \sa columnCount(), currentItem()
*/

int QTreeWidget::topLevelItemCount() const
{
    Q_D(const QTreeWidget);
    return d->treeModel()->rootItem->childCount();
}

/*!
  Inserts the \a item at \a index in the top level in the view.

  If the item has already been inserted somewhere else it won't be inserted.

  \sa addTopLevelItem(), columnCount()
*/

void QTreeWidget::insertTopLevelItem(int index, QTreeWidgetItem *item)
{
    Q_D(QTreeWidget);
    d->treeModel()->rootItem->insertChild(index, item);
}

/*!
    \since 4.1

    Appends the \a item as a top-level item in the widget.

    \sa insertTopLevelItem()
*/
void QTreeWidget::addTopLevelItem(QTreeWidgetItem *item)
{
    insertTopLevelItem(topLevelItemCount(), item);
}

/*!
  Removes the top-level item at the given \a index in the tree and
  returns it, otherwise returns \nullptr;

  \sa insertTopLevelItem(), topLevelItem(), topLevelItemCount()
*/

QTreeWidgetItem *QTreeWidget::takeTopLevelItem(int index)
{
    Q_D(QTreeWidget);
    return d->treeModel()->rootItem->takeChild(index);
}

/*!
  Returns the index of the given top-level \a item, or -1 if the item
  cannot be found.

  \sa sortItems(), topLevelItemCount()
 */
int QTreeWidget::indexOfTopLevelItem(QTreeWidgetItem *item) const
{
    Q_D(const QTreeWidget);
    d->treeModel()->executePendingSort();
    return d->treeModel()->rootItem->children.indexOf(item);
}

/*!
  \since 4.1

  Inserts the list of \a items at \a index in the top level in the view.

  Items that have already been inserted somewhere else won't be inserted.

  \sa addTopLevelItems()
*/
void QTreeWidget::insertTopLevelItems(int index, const QList<QTreeWidgetItem*> &items)
{
    Q_D(QTreeWidget);
    d->treeModel()->rootItem->insertChildren(index, items);
}

/*!
  Appends the list of \a items as a top-level items in the widget.

  \sa insertTopLevelItems()
*/
void QTreeWidget::addTopLevelItems(const QList<QTreeWidgetItem*> &items)
{
    insertTopLevelItems(topLevelItemCount(), items);
}

/*!
    Returns the item used for the tree widget's header.

    \sa setHeaderItem()
*/

QTreeWidgetItem *QTreeWidget::headerItem() const
{
    Q_D(const QTreeWidget);
    return d->treeModel()->headerItem;
}

/*!
    Sets the header \a item for the tree widget. The label for each column in
    the header is supplied by the corresponding label in the item.

    The tree widget takes ownership of the item.

    \sa headerItem(), setHeaderLabels()
*/

void QTreeWidget::setHeaderItem(QTreeWidgetItem *item)
{
    Q_D(QTreeWidget);
    if (!item)
        return;
    item->view = this;

    int oldCount = columnCount();
    if (oldCount < item->columnCount())
         d->treeModel()->beginInsertColumns(QModelIndex(), oldCount, item->columnCount() - 1);
    else if (oldCount > item->columnCount())
         d->treeModel()->beginRemoveColumns(QModelIndex(), item->columnCount(), oldCount - 1);
    delete d->treeModel()->headerItem;
    d->treeModel()->headerItem = item;
    if (oldCount < item->columnCount())
        d->treeModel()->endInsertColumns();
    else if (oldCount > item->columnCount())
        d->treeModel()->endRemoveColumns();
    d->treeModel()->headerDataChanged(Qt::Horizontal, 0, oldCount);
}


/*!
  Adds a column in the header for each item in the \a labels list, and sets
  the label for each column.

  Note that setHeaderLabels() won't remove existing columns.

  \sa setHeaderItem(), setHeaderLabel()
*/
void QTreeWidget::setHeaderLabels(const QStringList &labels)
{
    Q_D(QTreeWidget);
    if (columnCount() < labels.size())
        setColumnCount(labels.size());
    QTreeWidgetItem *item = d->treeModel()->headerItem;
    for (int i = 0; i < labels.size(); ++i)
        item->setText(i, labels.at(i));
}

/*!
    \fn void QTreeWidget::setHeaderLabel(const QString &label)
    \since 4.2

    Same as setHeaderLabels(QStringList(\a label)).
*/

/*!
    Returns the current item in the tree widget.

    \sa setCurrentItem(), currentItemChanged()
*/
QTreeWidgetItem *QTreeWidget::currentItem() const
{
    Q_D(const QTreeWidget);
    return d->item(currentIndex());
}

/*!
    \since 4.1
    Returns the current column in the tree widget.

    \sa setCurrentItem(), columnCount()
*/
int QTreeWidget::currentColumn() const
{
    return currentIndex().column();
}

/*!
  Sets the current \a item in the tree widget.

  Unless the selection mode is \l{QAbstractItemView::}{NoSelection},
  the item is also selected.

  \sa currentItem(), currentItemChanged()
*/
void QTreeWidget::setCurrentItem(QTreeWidgetItem *item)
{
    setCurrentItem(item, 0);
}

/*!
  \since 4.1
  Sets the current \a item in the tree widget and the current column to \a column.

  \sa currentItem()
*/
void QTreeWidget::setCurrentItem(QTreeWidgetItem *item, int column)
{
    Q_D(QTreeWidget);
    setCurrentIndex(d->index(item, column));
}

/*!
  \since 4.4
  Sets the current \a item in the tree widget and the current column to \a column,
  using the given \a command.

  \sa currentItem()
*/
void QTreeWidget::setCurrentItem(QTreeWidgetItem *item, int column,
                                 QItemSelectionModel::SelectionFlags command)
{
    Q_D(QTreeWidget);
    d->selectionModel->setCurrentIndex(d->index(item, column), command);
}


/*!
  Returns a pointer to the item at the coordinates \a p. The coordinates
  are relative to the tree widget's \l{QAbstractScrollArea::}{viewport()}.

  \sa visualItemRect()
*/
QTreeWidgetItem *QTreeWidget::itemAt(const QPoint &p) const
{
    Q_D(const QTreeWidget);
    return d->item(indexAt(p));
}

/*!
    \fn QTreeWidgetItem *QTreeWidget::itemAt(int x, int y) const
    \overload

    Returns a pointer to the item at the coordinates (\a x, \a y). The coordinates
    are relative to the tree widget's \l{QAbstractScrollArea::}{viewport()}.
*/

/*!
  Returns the rectangle on the viewport occupied by the item at \a item.

  \sa itemAt()
*/
QRect QTreeWidget::visualItemRect(const QTreeWidgetItem *item) const
{
    Q_D(const QTreeWidget);
    //the visual rect for an item is across all columns. So we need to determine
    //what is the first and last column and get their visual index rects
    const QModelIndex base = d->index(item);
    const int firstVisiblesection = header()->logicalIndexAt(- header()->offset());
    const int lastVisibleSection = header()->logicalIndexAt(header()->length() - header()->offset() - 1);
    const QModelIndex first = base.sibling(base.row(), firstVisiblesection);
    const QModelIndex last = base.sibling(base.row(), lastVisibleSection);
    return visualRect(first) | visualRect(last);
}

/*!
  \since 4.1

  Returns the column used to sort the contents of the widget.

  \sa sortItems()
*/
int QTreeWidget::sortColumn() const
{
    Q_D(const QTreeWidget);
    return (d->explicitSortColumn != -1
            ? d->explicitSortColumn
            : header()->sortIndicatorSection());
}

/*!
  Sorts the items in the widget in the specified \a order by the values in
  the given \a column.

  \sa sortColumn()
*/

void QTreeWidget::sortItems(int column, Qt::SortOrder order)
{
    Q_D(QTreeWidget);
    header()->setSortIndicator(column, order);
    d->model->sort(column, order);
}

/*!
  Starts editing the \a item in the given \a column if it is editable.
*/

void QTreeWidget::editItem(QTreeWidgetItem *item, int column)
{
    Q_D(QTreeWidget);
    edit(d->index(item, column));
}

/*!
  Opens a persistent editor for the \a item in the given \a column.

  \sa closePersistentEditor(), isPersistentEditorOpen()
*/

void QTreeWidget::openPersistentEditor(QTreeWidgetItem *item, int column)
{
    Q_D(QTreeWidget);
    QAbstractItemView::openPersistentEditor(d->index(item, column));
}

/*!
  Closes the persistent editor for the \a item in the given \a column.

  This function has no effect if no persistent editor is open for this
  combination of item and column.

  \sa openPersistentEditor(), isPersistentEditorOpen()
*/

void QTreeWidget::closePersistentEditor(QTreeWidgetItem *item, int column)
{
    Q_D(QTreeWidget);
    QAbstractItemView::closePersistentEditor(d->index(item, column));
}

/*!
    \since 5.10

    Returns whether a persistent editor is open for item \a item in
    column \a column.

    \sa openPersistentEditor(), closePersistentEditor()
*/

bool QTreeWidget::isPersistentEditorOpen(QTreeWidgetItem *item, int column) const
{
    Q_D(const QTreeWidget);
    return QAbstractItemView::isPersistentEditorOpen(d->index(item, column));
}

/*!
    \since 4.1

    Returns the widget displayed in the cell specified by \a item and the given \a column.

    \sa setItemWidget(), removeItemWidget()
*/
QWidget *QTreeWidget::itemWidget(QTreeWidgetItem *item, int column) const
{
    Q_D(const QTreeWidget);
    return QAbstractItemView::indexWidget(d->index(item, column));
}

/*!
    \since 4.1

    Sets the given \a widget to be displayed in the cell specified by the given
    \a item and \a column.

    The given \a widget's \l {QWidget::}{autoFillBackground} property must be
    set to true, otherwise the widget's background will be transparent, showing
    both the model data and the tree widget item.

    This function should only be used to display static content in the place of
    a tree widget item. If you want to display custom dynamic content or
    implement a custom editor widget, use QTreeView and subclass QStyledItemDelegate
    instead.

    This function cannot be called before the item hierarchy has been set up,
    i.e., the QTreeWidgetItem that will hold \a widget must have been added to
    the view before \a widget is set.

    \note The tree takes ownership of \a widget.

    \sa itemWidget(), removeItemWidget(), {Delegate Classes}
*/
void QTreeWidget::setItemWidget(QTreeWidgetItem *item, int column, QWidget *widget)
{
    Q_D(QTreeWidget);
    QAbstractItemView::setIndexWidget(d->index(item, column), widget);
}

/*!
  Returns a list of all selected non-hidden items.

  \sa itemSelectionChanged()
*/
QList<QTreeWidgetItem*> QTreeWidget::selectedItems() const
{
    Q_D(const QTreeWidget);
    const QModelIndexList indexes = selectionModel()->selectedIndexes();
    QList<QTreeWidgetItem*> items;
    items.reserve(indexes.size());
    QDuplicateTracker<QTreeWidgetItem *> seen(indexes.size());
    for (const auto &index : indexes) {
        QTreeWidgetItem *item = d->item(index);
        if (item->isHidden() || seen.hasSeen(item))
            continue;
        items.append(item);
    }
    return items;
}

/*!
  Returns a list of items that match the given \a text, using the given \a flags, in the given \a column.
*/
QList<QTreeWidgetItem*> QTreeWidget::findItems(const QString &text, Qt::MatchFlags flags, int column) const
{
    Q_D(const QTreeWidget);
    QModelIndexList indexes = d->model->match(model()->index(0, column, QModelIndex()),
                                                Qt::DisplayRole, text, -1, flags);
    QList<QTreeWidgetItem*> items;
    const int indexesSize = indexes.size();
    items.reserve(indexesSize);
    for (int i = 0; i < indexesSize; ++i)
        items.append(d->item(indexes.at(i)));
    return items;
}


/*!
  \since 4.3

  Returns the item above the given \a item.
*/
QTreeWidgetItem *QTreeWidget::itemAbove(const QTreeWidgetItem *item) const
{
    Q_D(const QTreeWidget);
    if (item == d->treeModel()->headerItem)
        return nullptr;
    const QModelIndex index = d->index(item);
    const QModelIndex above = indexAbove(index);
    return d->item(above);
}

/*!
  \since 4.3

  Returns the item visually below the given \a item.
*/
QTreeWidgetItem *QTreeWidget::itemBelow(const QTreeWidgetItem *item) const
{
    Q_D(const QTreeWidget);
    if (item == d->treeModel()->headerItem)
        return nullptr;
    const QModelIndex index = d->index(item);
    const QModelIndex below = indexBelow(index);
    return d->item(below);
}

/*!
   \reimp
 */
void QTreeWidget::setSelectionModel(QItemSelectionModel *selectionModel)
{
    Q_D(QTreeWidget);
    QTreeView::setSelectionModel(selectionModel);
    QItemSelection newSelection = selectionModel->selection();
    if (!newSelection.isEmpty())
        d->selectionChanged(newSelection, QItemSelection());
}

/*!
    Ensures that the \a item is visible, scrolling the view if necessary using
    the specified \a hint.

    \sa currentItem(), itemAt(), topLevelItem()
*/
void QTreeWidget::scrollToItem(const QTreeWidgetItem *item, QAbstractItemView::ScrollHint hint)
{
    Q_D(QTreeWidget);
    QTreeView::scrollTo(d->index(item), hint);
}

/*!
    Expands the \a item. This causes the tree containing the item's children
    to be expanded.

    \sa collapseItem(), currentItem(), itemAt(), topLevelItem(), itemExpanded()
*/
void QTreeWidget::expandItem(const QTreeWidgetItem *item)
{
    Q_D(QTreeWidget);
    QTreeModel::SkipSorting skipSorting(d->treeModel());
    expand(d->index(item));
}

/*!
    Closes the \a item. This causes the tree containing the item's children
    to be collapsed.

    \sa expandItem(), currentItem(), itemAt(), topLevelItem()
*/
void QTreeWidget::collapseItem(const QTreeWidgetItem *item)
{
    Q_D(QTreeWidget);
    QTreeModel::SkipSorting skipSorting(d->treeModel());
    collapse(d->index(item));
}

/*!
    Clears the tree widget by removing all of its items and selections.

    \b{Note:} Since each item is removed from the tree widget before being
    deleted, the return value of QTreeWidgetItem::treeWidget() will be invalid
    when called from an item's destructor.

    \sa takeTopLevelItem(), topLevelItemCount(), columnCount()
*/
void QTreeWidget::clear()
{
    Q_D(QTreeWidget);
    selectionModel()->clear();
    d->treeModel()->clear();
}

/*!
    Returns a list of MIME types that can be used to describe a list of
    treewidget items.

    \sa mimeData()
*/
QStringList QTreeWidget::mimeTypes() const
{
    return model()->QAbstractItemModel::mimeTypes();
}

/*!
    Returns an object that contains a serialized description of the specified
    \a items. The format used to describe the items is obtained from the
    mimeTypes() function.

    If the list of items is empty, \nullptr is returned rather than a
    serialized empty list.
*/
QMimeData *QTreeWidget::mimeData(const QList<QTreeWidgetItem *> &items) const
{
    Q_D(const QTreeWidget);
    if (d->treeModel()->cachedIndexes.isEmpty()) {
        QList<QModelIndex> indexes;
        for (const auto *item : items) {
            if (Q_UNLIKELY(!item)) {
                qWarning("QTreeWidget::mimeData: Null-item passed");
                return nullptr;
            }

            for (int c = 0; c < item->values.size(); ++c) {
                const QModelIndex index = indexFromItem(item, c);
                if (Q_UNLIKELY(!index.isValid())) {
                    qWarning() << "QTreeWidget::mimeData: No index associated with item :" << item;
                    return nullptr;
                }
                indexes << index;
            }
        }
        return d->model->QAbstractItemModel::mimeData(indexes);
    }
    return d->treeModel()->internalMimeData();
}

/*!
    Handles the \a data supplied by a drag and drop operation that ended with
    the given \a action in the \a index in the given \a parent item.

    The default implementation returns \c true if the drop was
    successfully handled by decoding the mime data and inserting it
    into the model; otherwise it returns \c false.

    \sa supportedDropActions()
*/
bool QTreeWidget::dropMimeData(QTreeWidgetItem *parent, int index,
                               const QMimeData *data, Qt::DropAction action)
{
    QModelIndex idx;
    if (parent) idx = indexFromItem(parent);
    return model()->QAbstractItemModel::dropMimeData(data, action , index, 0, idx);
}

/*!
  Returns the drop actions supported by this view.

  \sa Qt::DropActions
*/
Qt::DropActions QTreeWidget::supportedDropActions() const
{
    return model()->QAbstractItemModel::supportedDropActions() | Qt::MoveAction;
}

/*!
    Returns the QModelIndex associated with the given \a item in the given \a column.

    \note In Qt versions prior to 5.7, this function took a non-\c{const} \a item.

    \sa itemFromIndex(), topLevelItem()
*/
QModelIndex QTreeWidget::indexFromItem(const QTreeWidgetItem *item, int column) const
{
    Q_D(const QTreeWidget);
    return d->index(item, column);
}

/*!
    Returns a pointer to the QTreeWidgetItem associated with the given \a index.

    \sa indexFromItem()
*/
QTreeWidgetItem *QTreeWidget::itemFromIndex(const QModelIndex &index) const
{
    Q_D(const QTreeWidget);
    return d->item(index);
}

#if QT_CONFIG(draganddrop)
/*! \reimp */
void QTreeWidget::dropEvent(QDropEvent *event) {
    Q_D(QTreeWidget);
    if (event->source() == this && (event->dropAction() == Qt::MoveAction ||
                                    dragDropMode() == QAbstractItemView::InternalMove)) {
        QModelIndex topIndex;
        int col = -1;
        int row = -1;
        // check whether a subclass has already accepted the event, ie. moved the data
        if (!event->isAccepted() && d->dropOn(event, &row, &col, &topIndex)) {
            const QList<QModelIndex> idxs = selectedIndexes();
            QList<QPersistentModelIndex> indexes;
            const int indexesCount = idxs.size();
            indexes.reserve(indexesCount);
            for (const auto &idx : idxs)
                indexes.append(idx);

            if (indexes.contains(topIndex))
                return;

            // When removing items the drop location could shift
            QPersistentModelIndex dropRow = model()->index(row, col, topIndex);

            // Remove the items
            QList<QTreeWidgetItem *> taken;
            for (const auto &index : indexes) {
                QTreeWidgetItem *parent = itemFromIndex(index);
                if (!parent || !parent->parent()) {
                    taken.append(takeTopLevelItem(index.row()));
                } else {
                    taken.append(parent->parent()->takeChild(index.row()));
                }
            }

            // insert them back in at their new positions
            for (int i = 0; i < indexes.size(); ++i) {
                // Either at a specific point or appended
                if (row == -1) {
                    if (topIndex.isValid()) {
                        QTreeWidgetItem *parent = itemFromIndex(topIndex);
                        parent->insertChild(parent->childCount(), taken.takeFirst());
                    } else {
                        insertTopLevelItem(topLevelItemCount(), taken.takeFirst());
                    }
                } else {
                    int r = dropRow.row() >= 0 ? dropRow.row() : row;
                    if (topIndex.isValid()) {
                        QTreeWidgetItem *parent = itemFromIndex(topIndex);
                        parent->insertChild(qMin(r, parent->childCount()), taken.takeFirst());
                    } else {
                        insertTopLevelItem(qMin(r, topLevelItemCount()), taken.takeFirst());
                    }
                }
            }

            event->accept();
        }
        // either we or a subclass accepted the move event, so assume that the data was
        // moved and that QAbstractItemView shouldn't remove the source when QDrag::exec returns
        if (event->isAccepted())
            d->dropEventMoved = true;
    }

    QTreeView::dropEvent(event);
}
#endif

/*!
  \reimp
*/

void QTreeWidget::setModel(QAbstractItemModel * /*model*/)
{
    Q_ASSERT(!"QTreeWidget::setModel() - Changing the model of the QTreeWidget is not allowed.");
}

/*!
    \reimp
*/
bool QTreeWidget::event(QEvent *e)
{
    Q_D(QTreeWidget);
    if (e->type() == QEvent::Polish)
        d->treeModel()->executePendingSort();
    return QTreeView::event(e);
}

/*!
    see QTBUG-94546
*/
void QTreeModelPrivate::executePendingOperations() const
{
    q_func()->executePendingSort();
}

QT_END_NAMESPACE

#include "moc_qtreewidget.cpp"
#include "moc_qtreewidget_p.cpp"
