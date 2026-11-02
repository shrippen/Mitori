#include "keyedlistmodel.h"

#include <QSet>

namespace {
QString uidOf(const QVariant &row)
{
    return row.toMap().value(QStringLiteral("uid")).toString();
}
}

void KeyedListModel::setRows(const QVariantList &rows)
{
    QSet<QString> next;
    for (const QVariant &r : rows) {
        next.insert(uidOf(r));
    }
    // Walk both lists in order: drop what is gone, keep or update what stays, insert what is new.
    qsizetype i = 0;
    for (const QVariant &r : rows) {
        while (i < m_rows.size() && !next.contains(uidOf(m_rows.at(i)))) {
            beginRemoveRows({}, int(i), int(i));
            m_rows.removeAt(i);
            endRemoveRows();
        }
        if (i < m_rows.size() && uidOf(m_rows.at(i)) == uidOf(r)) {
            if (m_rows.at(i) != r) {
                m_rows[i] = r;
                emit dataChanged(index(int(i)), index(int(i)));
            }
        } else {
            beginInsertRows({}, int(i), int(i));
            m_rows.insert(i, r);
            endInsertRows();
        }
        ++i;
    }
    if (i < m_rows.size()) {
        beginRemoveRows({}, int(i), int(m_rows.size() - 1));
        m_rows.remove(i, m_rows.size() - i);
        endRemoveRows();
    }
    emit rowsChanged();
}

int KeyedListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QVariant KeyedListModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }
    const QVariant &row = m_rows.at(index.row());
    switch (role) {
    case RowRole:
        return row;
    case DayRole:
        return row.toMap().value(QStringLiteral("day"));
    case Qt::DisplayRole:
        return row.toMap().value(QStringLiteral("text")).toString();
    default:
        return {};
    }
}

QHash<int, QByteArray> KeyedListModel::roleNames() const
{
    return {{RowRole, "row"}, {DayRole, "day"}};
}
