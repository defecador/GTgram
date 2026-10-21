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

#ifndef TEXTFILTERMODEL_H
#define TEXTFILTERMODEL_H
#include <QAbstractListModel>
#include <QPersistentModelIndex>
#include <QPointer>
#include <QSet>
#include <QVector>

// Filters a flat list model by substring match on one role.
//
// This used to be a QSortFilterProxyModel, but that class turns every source
// rowsMoved() into layoutChanged(), which makes the QML delegate model throw
// away and re-create ALL the delegates. The chat list moves rows constantly
// (a chat jumps to the top whenever a message arrives), so with a filter active
// the app kept rebuilding every row of the list and hung. This model instead
// follows the source with exact row insert/remove/move notifications, so the
// view only touches the rows that really changed.
class TextFilterModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString filterRoleName READ getFilterRoleName WRITE setFilterRoleName NOTIFY filterRoleNameChanged)
    Q_PROPERTY(QString filterText READ getFilterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(QObject* sourceModel READ sourceModel WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    TextFilterModel(QObject *parent = Q_NULLPTR);

    QObject *sourceModel() const;
    void setSource(QObject* model);

    QString getFilterRoleName() const;
    void setFilterRoleName(QString role);

    QString getFilterText() const;
    void setFilterText(QString text);

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QHash<int,QByteArray> roleNames() const Q_DECL_OVERRIDE;

signals:
    void sourceChanged();
    void filterRoleNameChanged();
    void filterTextChanged();
    void countChanged();

private slots:
    void sourceDestroyed();
    void sourceReset();
    void sourceStructureChanged();
    void sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);

private:
    static int findRole(QAbstractItemModel *model, QString role);
    void updateFilterRole();
    bool matches(int sourceRow) const;
    void rebuild();
    void synchronize();

private:
    QPointer<QAbstractItemModel> source;
    QString filterRoleName;
    QString filterText;
    int filterRole;
    // Source rows that pass the filter, in source order. Persistent indexes
    // follow the rows when the source inserts, removes or moves them.
    QVector<QPersistentModelIndex> rows;
};

#endif // TEXTFILTERMODEL_H
