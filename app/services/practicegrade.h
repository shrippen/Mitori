#pragma once

#include <QHash>
#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class Courses;
class Programme;
class QJSEngine;
class QQmlEngine;

// The grade a student practises up to, per discipline: what the start menu, today's practice and
// the exam mode use. Browsing (programme page, technique map) keeps its own state on the page.
//
//   Courses::nextGrade ──► suggested ──┐
//   setGrade (by hand) ──► chosen ─────┴──► grade(discipline)
//
// A choice holds until a grade passed later moves the suggestion past it:
//
//   chosen 5. Kyu, suggested 3. Kyu   3. Kyu passed (suggests 2. Kyu)   practises 2. Kyu
//   chosen 1. Kyu, suggested 3. Kyu   3. Kyu passed (suggests 2. Kyu)   practises 1. Kyu
//
// So each choice keeps the suggestion it was made against, and grade() is worked out anew each
// time: a passed grade removed again (or a sync) gives the earlier answer back. Choosing what is
// suggested drops the choice: the grade follows "My grades" again.
class PracticeGrade : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    // Grows with every change of a practice grade, so bindings that read one follow.
    Q_PROPERTY(int revision READ revision NOTIFY changed)

public:
    // The app's: the engine's Programme and Courses.
    static PracticeGrade *create(QQmlEngine *engine, QJSEngine *);
    PracticeGrade(Programme *programme, Courses *courses, QObject *parent = nullptr);

    int revision() const { return m_revision; }

    // Always a grade with material ("" only for a discipline without any).
    Q_INVOKABLE QString grade(const QString &discipline) const;
    Q_INVOKABLE void setGrade(const QString &discipline, const QString &grade);
    // Whether grade() is the user's choice, not the suggestion from "My grades".
    Q_INVOKABLE bool manual(const QString &discipline) const;

signals:
    void changed();

private:
    QString suggested(const QString &discipline, const QStringList &available) const;
    QString held(const QString &discipline, const QStringList &available, const QString &suggestion) const;
    QStringList available(const QString &discipline) const;
    void migrate();
    void readNextGrades();

    Programme *m_programme;
    Courses *m_courses;
    QHash<QString, QString> m_nextGrades;
    int m_revision = 0;
};
