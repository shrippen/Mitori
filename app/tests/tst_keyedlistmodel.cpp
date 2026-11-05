#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QTest>

#include "keyedlistmodel.h"

using namespace Qt::StringLiterals;

// KeyedListModel: a new list of rows changes only what changed (rows removed, inserted, data
// changed), so that a view keeps its place and its current row; no reset after the first fill.
class TstKeyedListModel : public QObject
{
    Q_OBJECT

    static QVariantMap row(const QString &uid, const QString &text, const QString &day = QStringLiteral("2026-11-02"))
    {
        return {{QStringLiteral("uid"), uid}, {QStringLiteral("text"), text}, {QStringLiteral("day"), day}};
    }
    static QStringList uids(const KeyedListModel &m)
    {
        QStringList result;
        for (int i = 0; i < m.rowCount(); ++i) {
            result << m.data(m.index(i), KeyedListModel::RowRole).toMap().value(QStringLiteral("uid")).toString();
        }
        return result;
    }

private slots:
    void changesRowByRow()
    {
        KeyedListModel model;
        QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
        model.setRows({row(u"a"_s, u"1"_s), row(u"b"_s, u"2"_s), row(u"c"_s, u"3"_s)});
        QCOMPARE(uids(model), (QStringList{u"a"_s, u"b"_s, u"c"_s}));
        QCOMPARE(model.data(model.index(1), KeyedListModel::DayRole).toString(), u"2026-11-02"_s);

        QSignalSpy reset(&model, &QAbstractItemModel::modelReset);
        QSignalSpy removed(&model, &QAbstractItemModel::rowsRemoved);
        QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
        QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);

        // One removed in the middle.
        model.setRows({row(u"a"_s, u"1"_s), row(u"c"_s, u"3"_s)});
        QCOMPARE(removed.count(), 1);
        QCOMPARE(removed.first().at(1).toInt(), 1);
        // One changed.
        model.setRows({row(u"a"_s, u"1"_s), row(u"c"_s, u"drei"_s)});
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.first().at(0).toModelIndex().row(), 1);
        // One new at the top, one at the end.
        model.setRows({row(u"x"_s, u"0"_s), row(u"a"_s, u"1"_s), row(u"c"_s, u"drei"_s), row(u"z"_s, u"9"_s)});
        QCOMPARE(inserted.count(), 2);
        QCOMPARE(uids(model), (QStringList{u"x"_s, u"a"_s, u"c"_s, u"z"_s}));
        // The same again: nothing.
        changed.clear();
        model.setRows({row(u"x"_s, u"0"_s), row(u"a"_s, u"1"_s), row(u"c"_s, u"drei"_s), row(u"z"_s, u"9"_s)});
        QCOMPARE(changed.count(), 0);
        // A filter: most go, the rest stay where they were relative to each other.
        model.setRows({row(u"c"_s, u"drei"_s)});
        QCOMPARE(uids(model), QStringList{u"c"_s});
        model.setRows({});
        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(reset.count(), 0);
        QCOMPARE(model.rows().size(), 0);
    }

    // Thousands of notes: a view gets one signal per run of rows, not one per row (with 3000 notes
    // the first fill took seconds with the list attached).
    void changesInRuns()
    {
        KeyedListModel model;
        QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
        QVariantList all;
        for (int i = 0; i < 100; ++i) {
            all << row(QString::number(i), u"t"_s);
        }
        QSignalSpy removed(&model, &QAbstractItemModel::rowsRemoved);
        QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
        model.setRows(all);
        QCOMPARE(inserted.size(), 1);
        QCOMPARE(model.rowCount(), 100);

        // A filter: gaps go in runs.
        QVariantList some = all.mid(10, 10) + all.mid(50, 10);
        model.setRows(some);
        QCOMPARE(removed.size(), 3);
        QCOMPARE(uids(model).first(), u"10"_s);
        QCOMPARE(model.rowCount(), 20);

        // And back.
        inserted.clear();
        model.setRows(all);
        QCOMPARE(inserted.size(), 3);
        QCOMPARE(uids(model).size(), 100);
        for (int i = 0; i < 100; ++i) {
            QCOMPARE(uids(model).at(i), QString::number(i));
        }

        // A row moved: inserted again, the old place goes.
        QVariantList moved = all;
        moved.move(5, 0);
        model.setRows(moved);
        QCOMPARE(uids(model).first(), u"5"_s);
        QCOMPARE(model.rowCount(), 100);
    }

    void roleNames()
    {
        KeyedListModel model;
        const auto names = model.roleNames();
        QCOMPARE(names.value(KeyedListModel::RowRole), QByteArray("row"));
        QCOMPARE(names.value(KeyedListModel::DayRole), QByteArray("day"));
    }
};

QTEST_GUILESS_MAIN(TstKeyedListModel)
#include "tst_keyedlistmodel.moc"
