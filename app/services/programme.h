#pragma once

#include <QHash>
#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include "catalog.h"

// The exam programme for QML: disciplines, grades and the entries of a grade, ready for a list.
class Programme : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList disciplines READ disciplines CONSTANT)
    Q_PROPERTY(QString errorString READ errorString CONSTANT)
    Q_PROPERTY(QString dataVersion READ dataVersion CONSTANT)
    // The discipline chosen last (start menu tile, programme page); kept across starts.
    Q_PROPERTY(QString discipline READ discipline WRITE setDiscipline NOTIFY selectionChanged)

public:
    explicit Programme(QObject *parent = nullptr);
    // For tests: another catalog.
    explicit Programme(const Catalog &catalog, QObject *parent = nullptr);

    QVariantList disciplines() const { return m_catalog.disciplines(); }
    QString errorString() const { return m_catalog.errorString(); }
    QString dataVersion() const { return m_catalog.dataVersion(); }

    QString discipline() const;
    void setDiscipline(const QString &discipline);
    // The grade chosen last in a discipline, else its first grade with material.
    Q_INVOKABLE QString grade(const QString &discipline) const;
    Q_INVOKABLE void setGrade(const QString &discipline, const QString &grade);

    // Grades of a discipline that have their own material: {id, name, jp, count}.
    Q_INVOKABLE QVariantList grades(const QString &discipline) const;
    // Rows of a grade in programme order. Each row has `kind`:
    //   "group"     title, jp            a heading (Kata, Tachi-waza · Mae-seme, Kamae, Ô-dachi …)
    //   "attack"    title, extra, jp     the attack the techniques below answer
    //   "entry"     id, title, extra, jp, nr, video {video, von, bis} or none, comment {titel, text} or none,
    //               steps [{title, jp}]: the positions of a sequence (Kamae exercise), else empty
    //   "info"      id, title, jp, text, comment   an introduction with a comment only (Kumidachi)
    //   "text"      id, title            a theory item of the exam (essay, oral exam): no video, no rating
    Q_INVOKABLE QVariantList entries(const QString &discipline, const QString &grade) const;
    // What today's practice draws from: the entry rows of every grade up to the one chosen in the
    // discipline, in programme order, each with `current` (part of the chosen grade) and
    // `gradeName`.
    Q_INVOKABLE QVariantList practiceEntries(const QString &discipline) const;
    // Techniques whose name, attack, Japanese, additions, group or steps contain every word of `query`
    // (see SearchText), across all disciplines and grades in programme order (at most `limit`).
    // Each row as in entries(), with `discipline`, `disciplineName`, `grade`, `gradeName` and
    // `groupName` (Tachi-waza · Mae-seme …).
    Q_INVOKABLE QVariantList search(const QString &query, int limit = 50) const;
    // One entry by id (the same map as in entries()), or an empty map.
    Q_INVOKABLE QVariantMap entry(const QString &id) const;
    // The technique map (clickdummy "Landkarte") of every grade up to `grade`: a grid per group
    // with attacks, in the order of the discipline's groups; groups of the same title (kyu and dan
    // grades) are one.
    // Each grid: kind "matrix", title, jp,
    //   columns  [{title, extra, jp}]  the attacks (with addition), in order of appearance
    //   rows     [{title, jp, cells}]  the techniques by name, in order of appearance; one cell
    //            per column: {} where the technique does not answer the attack, else
    //            {ids, entry, gradeName, gradeNames}: every entry of the combination (variants, a
    //            later grade), the first one's row (as in entries()) and grade, the grades of all.
    // Shinki Toho: one grid, kind "forms", title "Ô-dachi 1–N", jp, cells [{ids, entry, gradeName, gradeNames}]
    // one per form.
    Q_INVOKABLE QVariantList techniqueMap(const QString &discipline, const QString &grade) const;

signals:
    void selectionChanged();

private:
    // The rows of a grade without the grade's name (entries() adds it).
    QVariantList gradeRows(const QString &disciplineId, const QString &grade) const;
    QVariantMap findDiscipline(const QString &id) const;
    QVariantList shinkiTohoEntries(const QVariantMap &discipline, const QString &grade) const;
    QVariantList formMap(const QString &grade) const;

    Catalog m_catalog;
    mutable QHash<QString, QVariantMap> m_entryIndex;
};
