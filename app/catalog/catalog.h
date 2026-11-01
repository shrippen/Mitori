#pragma once

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

// The programme data of all disciplines (mitori-daten.json, built by tools/daten.py from daten/).
class Catalog : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList disciplines READ disciplines NOTIFY loaded)
    Q_PROPERTY(QString errorString READ errorString NOTIFY loaded)

public:
    static constexpr int kFormat = 1;  // FORMAT in tools/daten.py
    static constexpr auto kResource = ":/daten/mitori-daten.json";

    explicit Catalog(QObject *parent = nullptr);

    bool load(const QString &path);
    QVariantList disciplines() const { return m_disciplines; }
    QString errorString() const { return m_error; }

signals:
    void loaded();

private:
    bool fail(const QString &error);

    QVariantList m_disciplines;
    QString m_error;
};
