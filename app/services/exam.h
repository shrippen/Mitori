#pragma once

#include <QObject>
#include <QRandomGenerator>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include "programme.h"

// The exam mode (mock exam) for QML: what a grade's exam asks (its points), and a round drawn
// from it with the chosen settings, which are kept across starts.
//
//   material   the practicable entries of a grade, in the order of the exam programme.
//              Shinki Toho: Shoden asks the Kamae as one point and Ô-dachi 1–11, the
//              higher grades their own forms.
//   round      [samples from earlier grades (2–5, always first)] + [the grade's points:
//              all or `count` of them, in programme order or shuffled]
//
// Each point: id, title, jp, extra, nr, group, attack, attackExtra, attackJp, grade,
// gradeName, sample (from an earlier grade), ids (the entries its rating sets).
class Exam : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    // Points of the grade in a round; 0 = all.
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY settingsChanged)
    // Shuffled order instead of the exam programme's.
    Q_PROPERTY(bool shuffled READ shuffled WRITE setShuffled NOTIFY settingsChanged)
    // Samples from earlier grades at the start of a round.
    Q_PROPERTY(bool samples READ samples WRITE setSamples NOTIFY settingsChanged)

public:
    enum class Order { Programme, Shuffled };
    enum class Samples { None, Earlier };

    static constexpr int kMinSamples = 2;
    static constexpr int kMaxSamples = 5;
    // The Kamae exercise of Shinki Toho's Shoden (one entry, one point).
    static constexpr auto kKamaeId = "shinkitoho/kamae";

    explicit Exam(QObject *parent = nullptr);
    // For tests: another catalog and a fixed seed.
    Exam(const Catalog &catalog, quint32 seed, QObject *parent = nullptr);

    int count() const;
    void setCount(int count);
    bool shuffled() const;
    void setShuffled(bool shuffled);
    bool samples() const;
    void setSamples(bool samples);

    // Grades with exam material: {id, name, jp, count} (count = points).
    Q_INVOKABLE QVariantList grades(const QString &discipline) const;
    // The grade chosen in the programme if it has material, else the first that has.
    Q_INVOKABLE QString grade(const QString &discipline) const;
    Q_INVOKABLE QVariantList material(const QString &discipline, const QString &grade) const;
    // Whether grades before this one have material (samples possible).
    Q_INVOKABLE bool hasEarlier(const QString &discipline, const QString &grade) const;
    // Choices for the number of points: 0 (all) and the fixed counts below `size`.
    Q_INVOKABLE QVariantList countChoices(int size) const;
    // A round with the stored settings.
    Q_INVOKABLE QVariantList draw(const QString &discipline, const QString &grade);
    QVariantList draw(const QString &discipline, const QString &grade, int count, Order order, Samples samples);

signals:
    void settingsChanged();

private:
    QVariantList earlierMaterial(const QString &discipline, const QString &grade) const;
    QVariantList shuffle(QVariantList list);

    Programme m_programme;
    QRandomGenerator m_random;
};
