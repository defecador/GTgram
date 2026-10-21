/*
    This file is part of GTgram.

    GTgram is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    GTgram is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with GTgram. If not, see <http://www.gnu.org/licenses/>.
*/

#include "textfiltermodel.h"

#define DEBUG_MODULE TextFilterModel
#include "debuglog.h"

TextFilterModel::TextFilterModel(QObject *parent) : QAbstractListModel(parent), filterRole(Qt::DisplayRole)
{
}

QObject *TextFilterModel::sourceModel() const
{
    return source.data();
}

void TextFilterModel::setSource(QObject *model)
{
    QAbstractItemModel *newSource = qobject_cast<QAbstractItemModel*>(model);
    if (source.data() == newSource) {
        return;
    }
    LOG(newSource);
    beginResetModel();
    if (source) {
        source->disconnect(this);
    }
    source = newSource;
    if (source) {
        connect(source.data(), &QObject::destroyed, this, &TextFilterModel::sourceDestroyed);
        connect(source.data(), &QAbstractItemModel::modelReset, this, &TextFilterModel::sourceReset);
        connect(source.data(), &QAbstractItemModel::rowsInserted, this, &TextFilterModel::sourceStructureChanged);
        connect(source.data(), &QAbstractItemModel::rowsRemoved, this, &TextFilterModel::sourceStructureChanged);
        connect(source.data(), &QAbstractItemModel::rowsMoved, this, &TextFilterModel::sourceStructureChanged);
        connect(source.data(), &QAbstractItemModel::layoutChanged, this, &TextFilterModel::sourceStructureChanged);
        connect(source.data(), &QAbstractItemModel::dataChanged, this, &TextFilterModel::sourceDataChanged);
    }
    updateFilterRole();
    rebuild();
    endResetModel();
    emit sourceChanged();
    emit countChanged();
}

QString TextFilterModel::getFilterRoleName() const
{
    return filterRoleName;
}

void TextFilterModel::setFilterRoleName(QString role)
{
    if (filterRoleName != role) {
        filterRoleName = role;
        LOG(role);
        updateFilterRole();
        emit filterRoleNameChanged();
        synchronize();
    }
}

QString TextFilterModel::getFilterText() const
{
    return filterText;
}

void TextFilterModel::setFilterText(QString text)
{
    if (filterText.compare(text, Qt::CaseInsensitive) != 0) {
        filterText = text;
        LOG(text);
        emit filterTextChanged();
        synchronize();
    }
}

int TextFilterModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : rows.count();
}

QVariant TextFilterModel::data(const QModelIndex &index, int role) const
{
    if (source && index.isValid() && index.row() >= 0 && index.row() < rows.count()) {
        const QPersistentModelIndex &sourceIndex = rows.at(index.row());
        if (sourceIndex.isValid()) {
            return source->data(sourceIndex, role);
        }
    }
    return QVariant();
}

QHash<int,QByteArray> TextFilterModel::roleNames() const
{
    return source ? source->roleNames() : QHash<int,QByteArray>();
}

void TextFilterModel::sourceDestroyed()
{
    beginResetModel();
    source = Q_NULLPTR;
    rows.clear();
    endResetModel();
    emit sourceChanged();
    emit countChanged();
}

void TextFilterModel::sourceReset()
{
    beginResetModel();
    rebuild();
    endResetModel();
    emit countChanged();
}

void TextFilterModel::sourceStructureChanged()
{
    synchronize();
}

void TextFilterModel::sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    if (!source || !topLeft.isValid() || topLeft.parent().isValid()) {
        return;
    }
    // A change to the role we filter on (or an unspecified change) can make a row
    // enter or leave the filtered list.
    if (!filterText.isEmpty() && (roles.isEmpty() || roles.contains(filterRole))) {
        synchronize();
    }
    // Forward the change for rows that are (still) in the list.
    for (int i = 0; i < rows.count(); i++) {
        const QPersistentModelIndex &sourceIndex = rows.at(i);
        if (sourceIndex.isValid() && sourceIndex.row() >= topLeft.row() && sourceIndex.row() <= bottomRight.row()) {
            const QModelIndex proxyIndex(index(i, 0));
            emit dataChanged(proxyIndex, proxyIndex, roles);
        }
    }
}

int TextFilterModel::findRole(QAbstractItemModel *model, QString role)
{
    if (model && !role.isEmpty()) {
        const QByteArray roleName(role.toUtf8());
        const QHash<int,QByteArray> roleMap(model->roleNames());
        const QList<int> roles(roleMap.keys());
        const int n = roles.count();
        for (int i = 0; i < n; i++) {
            const QByteArray name(roleMap.value(roles.at(i)));
            if (name == roleName) {
                LOG(role << roles.at(i));
                return roles.at(i);
            }
        }
        LOG("Unknown role" << role);
    }
    return -1;
}

void TextFilterModel::updateFilterRole()
{
    const int role = findRole(source.data(), filterRoleName);
    filterRole = (role >= 0) ? role : Qt::DisplayRole;
}

bool TextFilterModel::matches(int sourceRow) const
{
    if (filterText.isEmpty()) {
        return true;
    }
    return source->data(source->index(sourceRow, 0), filterRole).toString().contains(filterText, Qt::CaseInsensitive);
}

void TextFilterModel::rebuild()
{
    rows.clear();
    if (!source) {
        return;
    }
    const int n = source->rowCount();
    rows.reserve(n);
    for (int i = 0; i < n; i++) {
        if (matches(i)) {
            rows.append(QPersistentModelIndex(source->index(i, 0)));
        }
    }
}

// Brings `rows` in line with the source using the smallest set of remove, move
// and insert notifications, so a view keeps every delegate that did not change.
void TextFilterModel::synchronize()
{
    if (!source) {
        return;
    }
    const int oldCount = rows.count();

    // Wanted rows, in source order.
    QVector<int> wanted;
    const int n = source->rowCount();
    wanted.reserve(n);
    for (int i = 0; i < n; i++) {
        if (matches(i)) {
            wanted.append(i);
        }
    }
    QSet<int> wantedSet;
    for (int r : wanted) {
        wantedSet.insert(r);
    }

    // 1. Remove rows that no longer pass (or whose source row is gone).
    for (int i = rows.count() - 1; i >= 0; i--) {
        if (!rows.at(i).isValid() || !wantedSet.contains(rows.at(i).row())) {
            beginRemoveRows(QModelIndex(), i, i);
            rows.remove(i);
            endRemoveRows();
        }
    }

    // 2. Walk the wanted order: move a row up from further down, or insert it.
    for (int i = 0; i < wanted.count(); i++) {
        const int wantedRow = wanted.at(i);
        if (i < rows.count() && rows.at(i).row() == wantedRow) {
            continue;
        }
        int found = -1;
        for (int j = i + 1; j < rows.count(); j++) {
            if (rows.at(j).row() == wantedRow) {
                found = j;
                break;
            }
        }
        if (found >= 0) {
            beginMoveRows(QModelIndex(), found, found, QModelIndex(), i);
            rows.move(found, i);
            endMoveRows();
        } else {
            beginInsertRows(QModelIndex(), i, i);
            rows.insert(i, QPersistentModelIndex(source->index(wantedRow, 0)));
            endInsertRows();
        }
    }

    if (rows.count() != oldCount) {
        emit countChanged();
    }
}
