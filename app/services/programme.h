#pragma once

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include "catalog.h"

// The exam programme for QML: disciplines, grades and techniques from the catalog.
class Programme : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList disciplines READ disciplines CONSTANT)
    Q_PROPERTY(QString errorString READ errorString CONSTANT)

public:
    explicit Programme(QObject *parent = nullptr);

    QVariantList disciplines() const { return m_catalog.disciplines(); }
    QString errorString() const { return m_catalog.errorString(); }

private:
    Catalog m_catalog;
};
