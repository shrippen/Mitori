#include "programme.h"

#include "searchkey.h"

#include <QSet>
#include <QSettings>

#include <algorithm>

namespace {

QVariantMap row(const QString &kind, const QString &title, const QString &jp)
{
    return {{QStringLiteral("kind"), kind}, {QStringLiteral("title"), title}, {QStringLiteral("jp"), jp}};
}

// An entry row from a technique, kamae, suburi or form of the catalog.
QVariantMap entryRow(const QVariantMap &item, const QString &title, const QString &jp)
{
    QVariantMap r = row(QStringLiteral("entry"), title, jp);
    r[QStringLiteral("id")] = item.value(QStringLiteral("id"));
    r[QStringLiteral("extra")] = item.value(QStringLiteral("zusatz"));
    r[QStringLiteral("nr")] = item.value(QStringLiteral("nr"));
    r[QStringLiteral("video")] = item.value(QStringLiteral("video"));
    r[QStringLiteral("comment")] = item.value(QStringLiteral("kommentar"));
    return r;
}

QString name(const QVariantMap &item) { return item.value(QStringLiteral("name")).toString(); }
QString kanji(const QVariantMap &item) { return item.value(QStringLiteral("kanji")).toString(); }

} // namespace

Programme::Programme(QObject *parent)
    : Programme(Catalog::bundled(), parent)
{
}

Programme::Programme(const Catalog &catalog, QObject *parent)
    : QObject(parent)
    , m_catalog(catalog)
{
}

QVariantMap Programme::findDiscipline(const QString &id) const
{
    for (const QVariant &d : m_catalog.disciplines()) {
        if (d.toMap().value(QStringLiteral("id")).toString() == id) {
            return d.toMap();
        }
    }
    return {};
}

QString Programme::discipline() const
{
    const QString first = m_catalog.disciplines().value(0).toMap().value(QStringLiteral("id")).toString();
    const QString stored = QSettings().value(QStringLiteral("programme/discipline"), first).toString();
    return findDiscipline(stored).isEmpty() ? first : stored;
}

void Programme::setDiscipline(const QString &discipline)
{
    if (discipline == this->discipline() || findDiscipline(discipline).isEmpty()) {
        return;
    }
    QSettings().setValue(QStringLiteral("programme/discipline"), discipline);
    emit selectionChanged();
}

QString Programme::grade(const QString &discipline) const
{
    const QVariantList available = grades(discipline);
    const QString stored = QSettings().value(QStringLiteral("programme/grade/") + discipline).toString();
    for (const QVariant &g : available) {
        if (g.toMap().value(QStringLiteral("id")).toString() == stored) {
            return stored;
        }
    }
    return available.value(0).toMap().value(QStringLiteral("id")).toString();
}

void Programme::setGrade(const QString &discipline, const QString &grade)
{
    if (grade == this->grade(discipline)) {
        return;
    }
    QSettings().setValue(QStringLiteral("programme/grade/") + discipline, grade);
    emit selectionChanged();
}

QVariantList Programme::grades(const QString &disciplineId) const
{
    QVariantList result;
    const QVariantMap d = findDiscipline(disciplineId);
    for (const QVariant &g : d.value(QStringLiteral("grade")).toList()) {
        QVariantMap grade = g.toMap();
        int count = 0;
        for (const QVariant &e : entries(disciplineId, grade.value(QStringLiteral("id")).toString())) {
            count += e.toMap().value(QStringLiteral("kind")) == QLatin1String("entry");
        }
        if (count == 0) {
            continue;
        }
        grade[QStringLiteral("count")] = count;
        result << grade;
    }
    return result;
}

QVariantList Programme::practiceEntries(const QString &disciplineId) const
{
    QVariantList result;
    const QString chosen = grade(disciplineId);
    for (const QVariant &g : grades(disciplineId)) {
        const QVariantMap gradeMap = g.toMap();
        const QString id = gradeMap.value(QStringLiteral("id")).toString();
        for (const QVariant &e : entries(disciplineId, id)) {
            QVariantMap row = e.toMap();
            if (row.value(QStringLiteral("kind")) != QLatin1String("entry")) {
                continue;
            }
            row[QStringLiteral("current")] = id == chosen;
            row[QStringLiteral("gradeName")] = gradeMap.value(QStringLiteral("name"));
            result << row;
        }
        if (id == chosen) {
            break;
        }
    }
    return result;
}

QVariantList Programme::entries(const QString &disciplineId, const QString &grade) const
{
    const QVariantMap d = findDiscipline(disciplineId);
    if (disciplineId == QLatin1String("shinkitoho")) {
        return shinkiTohoEntries(d, grade);
    }
    QHash<QString, QVariantMap> groups;
    for (const QVariant &g : d.value(QStringLiteral("gruppen")).toList()) {
        groups.insert(g.toMap().value(QStringLiteral("id")).toString(), g.toMap());
    }
    QVariantList result;
    QString lastGroup;
    QString lastAttack;
    for (const QVariant &t : d.value(QStringLiteral("techniken")).toList()) {
        const QVariantMap technique = t.toMap();
        if (technique.value(QStringLiteral("grad")).toString() != grade) {
            continue;
        }
        const QString group = technique.value(QStringLiteral("gruppe")).toString();
        if (group != lastGroup) {
            const QVariantMap g = groups.value(group);
            result << row(QStringLiteral("group"), g.value(QStringLiteral("titel")).toString(), g.value(QStringLiteral("jp")).toString());
            lastGroup = group;
            lastAttack.clear();
        }
        const QVariantMap attack = technique.value(QStringLiteral("angriff")).toMap();
        const QString attackKey = name(attack) + QLatin1Char('|') + attack.value(QStringLiteral("zusatz")).toString();
        if (!attack.isEmpty() && attackKey != lastAttack) {
            QVariantMap a = row(QStringLiteral("attack"), name(attack), attack.value(QStringLiteral("jp")).toString());
            a[QStringLiteral("extra")] = attack.value(QStringLiteral("zusatz"));
            result << a;
            lastAttack = attackKey;
        }
        QVariantMap r = entryRow(technique, name(technique), technique.value(QStringLiteral("jp")).toString());
        r[QStringLiteral("attack")] = name(attack);
        r[QStringLiteral("attackExtra")] = attack.value(QStringLiteral("zusatz"));
        r[QStringLiteral("attackJp")] = attack.value(QStringLiteral("jp"));
        if (groups.value(group).value(QStringLiteral("art")) == QLatin1String("text")) {
            r[QStringLiteral("kind")] = QStringLiteral("text");  // theory: an essay, an oral exam
        }
        result << r;
    }
    return result;
}

QVariantList Programme::shinkiTohoEntries(const QVariantMap &d, const QString &grade) const
{
    int from = 0;
    int to = 0;
    int previous = 0;
    for (const QVariant &g : d.value(QStringLiteral("grade")).toList()) {
        const QVariantMap map = g.toMap();
        const int upTo = map.value(QStringLiteral("bis")).toInt();
        if (map.value(QStringLiteral("id")).toString() == grade) {
            from = previous + 1;
            to = upTo;
            break;
        }
        previous = upTo > 0 ? upTo : previous;
    }
    const QVariantMap sections = d.value(QStringLiteral("abschnitte")).toMap();
    auto section = [&sections](const QString &key) {
        const QVariantMap s = sections.value(key).toMap();
        return row(QStringLiteral("group"), name(s), kanji(s));
    };
    QVariantList result;
    if (grade == QLatin1String("shoden")) {
        result << section(QStringLiteral("suburi"));
        for (const QVariant &s : d.value(QStringLiteral("suburi")).toList()) {
            result << entryRow(s.toMap(), name(s.toMap()), kanji(s.toMap()));
        }
        const QVariantMap kiriotoshi = d.value(QStringLiteral("kiriotoshi")).toMap();
        result << row(QStringLiteral("group"), name(kiriotoshi), kanji(kiriotoshi));
        result << entryRow(kiriotoshi, name(kiriotoshi), kanji(kiriotoshi));
        result << section(QStringLiteral("kamae"));
        for (const QVariant &k : d.value(QStringLiteral("kamae")).toList()) {
            result << entryRow(k.toMap(), name(k.toMap()), kanji(k.toMap()));
        }
    }
    if (to == 0) {
        return result;
    }
    QVariantMap odachi = section(QStringLiteral("odachi"));
    odachi[QStringLiteral("title")] = QStringLiteral("%1 %2–%3").arg(odachi.value(QStringLiteral("title")).toString()).arg(from).arg(to);
    result << odachi;
    if (grade == QLatin1String("shoden")) {
        const QVariantMap kumidachi = d.value(QStringLiteral("kumidachi")).toMap();
        QVariantMap info = row(QStringLiteral("info"), name(kumidachi), kanji(kumidachi));
        info[QStringLiteral("id")] = kumidachi.value(QStringLiteral("id"));
        info[QStringLiteral("text")] = kumidachi.value(QStringLiteral("text"));
        info[QStringLiteral("comment")] = kumidachi.value(QStringLiteral("kommentar"));
        result << info;
    }
    for (const QVariant &o : d.value(QStringLiteral("odachi")).toList()) {
        const QVariantMap form = o.toMap();
        const int nr = form.value(QStringLiteral("nr")).toInt();
        if (nr < from || nr > to) {
            continue;
        }
        QVariantMap r = entryRow(form, name(form), kanji(form));
        r[QStringLiteral("extra")] = QStringLiteral("%1 · %2 → %3").arg(form.value(QStringLiteral("uchidachi")).toString(),
                                                                      form.value(QStringLiteral("shidachi")).toString(),
                                                                      form.value(QStringLiteral("treffer")).toString());
        result << r;
    }
    return result;
}

QVariantList Programme::search(const QString &query, int limit) const
{
    QVariantList result;
    const QStringList words = searchWords(query);
    if (words.isEmpty()) {
        return result;
    }
    QSet<QString> seen;
    for (const QVariant &d : m_catalog.disciplines()) {
        const QVariantMap discipline = d.toMap();
        const QString disciplineId = discipline.value(QStringLiteral("id")).toString();
        for (const QVariant &g : grades(disciplineId)) {
            const QVariantMap grade = g.toMap();
            QString group;
            for (const QVariant &e : entries(disciplineId, grade.value(QStringLiteral("id")).toString())) {
                QVariantMap row = e.toMap();
                if (row.value(QStringLiteral("kind")) == QLatin1String("group")) {
                    group = row.value(QStringLiteral("title")).toString();
                    continue;
                }
                const QString id = row.value(QStringLiteral("id")).toString();
                if (row.value(QStringLiteral("kind")) != QLatin1String("entry") || seen.contains(id)) {
                    continue;
                }
                const QStringList fields{row.value(QStringLiteral("title")).toString(),
                                         row.value(QStringLiteral("jp")).toString(),
                                         row.value(QStringLiteral("extra")).toString(),
                                         row.value(QStringLiteral("attack")).toString(),
                                         row.value(QStringLiteral("attackExtra")).toString(),
                                         row.value(QStringLiteral("attackJp")).toString(),
                                         group};
                if (!SearchText(searchKey(fields.join(u' '))).matches(words)) {
                    continue;
                }
                seen.insert(id);
                row[QStringLiteral("discipline")] = disciplineId;
                row[QStringLiteral("disciplineName")] = discipline.value(QStringLiteral("kurzname"));
                row[QStringLiteral("grade")] = grade.value(QStringLiteral("id"));
                row[QStringLiteral("gradeName")] = grade.value(QStringLiteral("name"));
                row[QStringLiteral("groupName")] = group;
                result << row;
                if (result.size() >= limit) {
                    return result;
                }
            }
        }
    }
    return result;
}

QVariantMap Programme::entry(const QString &id) const
{
    for (const QVariant &d : m_catalog.disciplines()) {
        const QVariantMap map = d.toMap();
        const QString disciplineId = map.value(QStringLiteral("id")).toString();
        for (const QVariant &g : map.value(QStringLiteral("grade")).toList()) {
            for (const QVariant &e : entries(disciplineId, g.toMap().value(QStringLiteral("id")).toString())) {
                if (e.toMap().value(QStringLiteral("id")).toString() == id) {
                    QVariantMap found = e.toMap();
                    found[QStringLiteral("discipline")] = disciplineId;
                    found[QStringLiteral("grade")] = g.toMap().value(QStringLiteral("id"));
                    return found;
                }
            }
        }
    }
    return {};
}
