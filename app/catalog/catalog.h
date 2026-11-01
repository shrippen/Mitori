#pragma once

#include <QString>
#include <QVariantList>

// The programme data of all disciplines (mitori-daten.json, built by tools/daten.py from daten/).
// Read by services only, never by QML (agent.md, layers).
class Catalog
{
public:
    static constexpr int kFormat = 1;  // FORMAT in tools/daten.py
    static constexpr auto kResource = ":/daten/mitori-daten.json";

    bool load(const QString &path);
    QVariantList disciplines() const { return m_disciplines; }
    QString errorString() const { return m_error; }

private:
    bool fail(const QString &error);

    QVariantList m_disciplines;
    QString m_error;
};
