#pragma once

#include <QAbstractListModel>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

// A list model for rows from QML (maps with a "uid"), as in "All notes": a new list changes
// only what changed (rows removed, inserted, data changed) instead of resetting, so that a
// ListView keeps its place and its current row after an edit. The rows keep their order
// relative to each other (newest first), as the notes do.
//   row  the whole map (delegate: required property var row)
//   day  its "day" (the ListView's section)
class KeyedListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList rows READ rows WRITE setRows NOTIFY rowsChanged)

public:
    enum Role { RowRole = Qt::UserRole + 1, DayRole };

    using QAbstractListModel::QAbstractListModel;

    QVariantList rows() const { return m_rows; }
    void setRows(const QVariantList &rows);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void rowsChanged();

private:
    QVariantList m_rows;
};
