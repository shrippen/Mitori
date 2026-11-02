#include "exam.h"

#include <QSet>
#include <QSettings>

#include <algorithm>
#include <array>
#include <numeric>

namespace {

// Fixed sizes of a round besides "all" (clickdummy: alle / 8 / 12 / 15).
constexpr std::array kCounts{8, 12, 15};

const QString kShinkiToho = QStringLiteral("shinkitoho");
const QString kOdachiPrefix = QStringLiteral("shinkitoho/odachi/");

QString text(const QVariantMap &map, const char *key)
{
    return map.value(QLatin1String(key)).toString();
}

// A point of the exam from an entry row (Programme::entries) under its group heading.
QVariantMap point(const QVariantMap &row, const QString &group, const QVariantMap &grade)
{
    QVariantMap p;
    for (const char *key : {"id", "title", "jp", "extra", "nr", "attack", "attackExtra", "attackJp"}) {
        p[QLatin1String(key)] = row.value(QLatin1String(key));
    }
    p[QStringLiteral("group")] = group;
    p[QStringLiteral("grade")] = grade.value(QStringLiteral("id"));
    p[QStringLiteral("gradeName")] = grade.value(QStringLiteral("name"));
    p[QStringLiteral("sample")] = false;
    return p;
}

// What a point shows on the board. Grades ask some techniques again under their own id
// (Daitoryu: Aiki-otoshi in the 5. and 3. Kyu); a round shows each content once.
QString content(const QVariant &point)
{
    const QVariantMap map = point.toMap();
    QStringList parts;
    for (const char *key : {"group", "attack", "attackExtra", "title", "extra"}) {
        parts << text(map, key);
    }
    return parts.join(u'|');
}

} // namespace

Exam::Exam(QObject *parent)
    : Exam(Catalog::bundled(), QRandomGenerator::global()->generate(), parent)
{
}

Exam::Exam(const Catalog &catalog, quint32 seed, QObject *parent)
    : QObject(parent)
    , m_programme(catalog)
    , m_random(seed)
{
}

int Exam::count() const
{
    return QSettings().value(QStringLiteral("exam/count"), 0).toInt();
}

void Exam::setCount(int count)
{
    if (count == this->count()) {
        return;
    }
    QSettings().setValue(QStringLiteral("exam/count"), qMax(0, count));
    emit settingsChanged();
}

bool Exam::shuffled() const
{
    return QSettings().value(QStringLiteral("exam/shuffled"), false).toBool();
}

void Exam::setShuffled(bool shuffled)
{
    if (shuffled == this->shuffled()) {
        return;
    }
    QSettings().setValue(QStringLiteral("exam/shuffled"), shuffled);
    emit settingsChanged();
}

bool Exam::samples() const
{
    return QSettings().value(QStringLiteral("exam/samples"), true).toBool();
}

void Exam::setSamples(bool samples)
{
    if (samples == this->samples()) {
        return;
    }
    QSettings().setValue(QStringLiteral("exam/samples"), samples);
    emit settingsChanged();
}

QVariantList Exam::grades(const QString &discipline) const
{
    QVariantList result;
    for (const QVariant &g : m_programme.grades(discipline)) {
        QVariantMap grade = g.toMap();
        const int points = int(material(discipline, text(grade, "id")).size());
        if (points == 0) {
            continue;
        }
        grade[QStringLiteral("count")] = points;
        result << grade;
    }
    return result;
}

QString Exam::grade(const QString &discipline) const
{
    const QString chosen = m_programme.grade(discipline);
    const QVariantList available = grades(discipline);
    for (const QVariant &g : available) {
        if (text(g.toMap(), "id") == chosen) {
            return chosen;
        }
    }
    return text(available.value(0).toMap(), "id");
}

QVariantList Exam::material(const QString &discipline, const QString &gradeId) const
{
    // The grade as the data have it (Programme::grades would read every grade's entries).
    QVariantMap grade;
    for (const QVariant &d : m_programme.disciplines()) {
        if (text(d.toMap(), "id") != discipline) {
            continue;
        }
        for (const QVariant &g : d.toMap().value(QStringLiteral("grade")).toList()) {
            if (text(g.toMap(), "id") == gradeId) {
                grade = g.toMap();
            }
        }
    }
    if (grade.isEmpty()) {
        return {};
    }

    const bool shinkiToho = discipline == kShinkiToho;
    QVariantList result;

    // The grade's entries under their headings; Shinki Toho only the Kamae exercise (Shoden, as
    // one point) and its forms.
    QString group;
    for (const QVariant &r : m_programme.entries(discipline, gradeId)) {
        const QVariantMap row = r.toMap();
        const QString kind = text(row, "kind");
        if (kind == QLatin1String("group")) {
            group = text(row, "title");
            continue;
        }
        if (kind != QLatin1String("entry")) {
            continue;
        }
        if (shinkiToho && text(row, "id") != QLatin1String(kKamaeId) && !text(row, "id").startsWith(kOdachiPrefix)) {
            continue;
        }
        result << point(row, group, grade);
    }
    return result;
}

bool Exam::hasEarlier(const QString &discipline, const QString &grade) const
{
    return !earlierMaterial(discipline, grade).isEmpty();
}

QVariantList Exam::earlierMaterial(const QString &discipline, const QString &grade) const
{
    QVariantList result;
    for (const QVariant &g : m_programme.grades(discipline)) {
        const QString id = text(g.toMap(), "id");
        if (id == grade) {
            return result;
        }
        result << material(discipline, id);
    }
    return {};      // not a grade of the discipline
}

QVariantList Exam::countChoices(int size) const
{
    QVariantList result{0};
    for (int n : kCounts) {
        if (n < size) {
            result << n;
        }
    }
    return result;
}

QVariantList Exam::shuffle(QVariantList list)
{
    std::shuffle(list.begin(), list.end(), m_random);
    return list;
}

QVariantList Exam::draw(const QString &discipline, const QString &grade)
{
    return draw(discipline, grade, count(), shuffled() ? Order::Shuffled : Order::Programme,
                samples() ? Samples::Earlier : Samples::None);
}

QVariantList Exam::draw(const QString &discipline, const QString &grade, int count, Order order, Samples samples)
{
    const QVariantList all = material(discipline, grade);

    // The grade's points: a random choice of `count`, kept in programme order unless shuffled.
    QVariantList main = all;
    if (count > 0 && count < all.size()) {
        QList<int> picked(all.size());
        std::iota(picked.begin(), picked.end(), 0);
        std::shuffle(picked.begin(), picked.end(), m_random);
        picked.resize(count);
        if (order == Order::Programme) {
            std::sort(picked.begin(), picked.end());
        }
        main.clear();
        for (int i : picked) {
            main << all.at(i);
        }
    } else if (order == Order::Shuffled) {
        main = shuffle(main);
    }

    // Samples from earlier grades, always first (app-entscheidungen.md, Runde 4), marked.
    QVariantList result;
    const QVariantList earlier = samples == Samples::Earlier ? earlierMaterial(discipline, grade) : QVariantList();
    if (earlier.isEmpty()) {
        return main;
    }
    // Nothing the main part asks, and no sample twice.
    QSet<QString> shown;
    for (const QVariant &p : main) {
        shown.insert(content(p));
    }
    const int n = kMinSamples + int(m_random.bounded(kMaxSamples - kMinSamples + 1));
    for (const QVariant &s : shuffle(earlier)) {
        if (result.size() >= n) {
            break;
        }
        if (shown.contains(content(s))) {
            continue;
        }
        shown.insert(content(s));
        QVariantMap p = s.toMap();
        p[QStringLiteral("sample")] = true;
        result << p;
    }
    return result + main;
}
