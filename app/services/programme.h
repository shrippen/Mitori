#pragma once

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
    // The discipline chosen last (start menu tile, programme page); kept across starts.
    Q_PROPERTY(QString discipline READ discipline WRITE setDiscipline NOTIFY selectionChanged)

public:
    explicit Programme(QObject *parent = nullptr);
    // For tests: another catalog.
    explicit Programme(const Catalog &catalog, QObject *parent = nullptr);

    QVariantList disciplines() const { return m_catalog.disciplines(); }
    QString errorString() const { return m_catalog.errorString(); }

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
    //   "entry"     id, title, extra, jp, nr, video {video, von, bis} or none, comment {titel, text} or none
    //   "info"      id, title, jp, text, comment   an introduction with a comment only (Kumidachi)
    //   "text"      id, title            a theory item of the exam (essay, oral exam): no video, no rating
    Q_INVOKABLE QVariantList entries(const QString &discipline, const QString &grade) const;
    // What today's practice draws from: the entry rows of every grade up to the one chosen in the
    // discipline, in programme order, each with `current` (part of the chosen grade) and
    // `gradeName`.
    Q_INVOKABLE QVariantList practiceEntries(const QString &discipline) const;
    // One entry by id (the same map as in entries()), or an empty map.
    Q_INVOKABLE QVariantMap entry(const QString &id) const;

signals:
    void selectionChanged();

private:
    QVariantMap findDiscipline(const QString &id) const;
    QVariantList shinkiTohoEntries(const QVariantMap &discipline, const QString &grade) const;

    Catalog m_catalog;
};
