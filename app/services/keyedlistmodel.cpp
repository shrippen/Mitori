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
    // Walk both lists in order: drop what is gone, keep or update what stays, insert what is new;
    // a run of rows gets one signal (a view redoes its layout on each).
    qsizetype i = 0;
    for (qsizetype r = 0; r < rows.size(); ++r) {
        qsizetype gone = 0;
        while (i + gone < m_rows.size() && !next.contains(uidOf(m_rows.at(i + gone)))) {
            ++gone;
        }
        if (gone > 0) {
            beginRemoveRows({}, int(i), int(i + gone - 1));
            m_rows.remove(i, gone);
            endRemoveRows();
        }
        if (i < m_rows.size() && uidOf(m_rows.at(i)) == uidOf(rows.at(r))) {
            if (m_rows.at(i) != rows.at(r)) {
                m_rows[i] = rows.at(r);
                emit dataChanged(index(int(i)), index(int(i)));
            }
            ++i;
            continue;
        }
        // New up to the row that stands here.
        const QString here = i < m_rows.size() ? uidOf(m_rows.at(i)) : QString();
        qsizetype end = r + 1;
        while (end < rows.size() && uidOf(rows.at(end)) != here) {
            ++end;
        }
        beginInsertRows({}, int(i), int(i + end - r - 1));
        m_rows.insert(i, end - r, QVariant());
        for (qsizetype k = r; k < end; ++k) {
            m_rows[i + k - r] = rows.at(k);
        }
        endInsertRows();
        i += end - r;
        r = end - 1;
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
