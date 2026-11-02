#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QTest>

#include "keyedlistmodel.h"

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
        model.setRows({row(u"a"_qs, u"1"_qs), row(u"b"_qs, u"2"_qs), row(u"c"_qs, u"3"_qs)});
        QCOMPARE(uids(model), (QStringList{u"a"_qs, u"b"_qs, u"c"_qs}));
        QCOMPARE(model.data(model.index(1), KeyedListModel::DayRole).toString(), u"2026-11-02"_qs);

        QSignalSpy reset(&model, &QAbstractItemModel::modelReset);
        QSignalSpy removed(&model, &QAbstractItemModel::rowsRemoved);
        QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
        QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);

        // One removed in the middle.
        model.setRows({row(u"a"_qs, u"1"_qs), row(u"c"_qs, u"3"_qs)});
        QCOMPARE(removed.count(), 1);
        QCOMPARE(removed.first().at(1).toInt(), 1);
        // One changed.
        model.setRows({row(u"a"_qs, u"1"_qs), row(u"c"_qs, u"drei"_qs)});
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.first().at(0).toModelIndex().row(), 1);
        // One new at the top, one at the end.
        model.setRows({row(u"x"_qs, u"0"_qs), row(u"a"_qs, u"1"_qs), row(u"c"_qs, u"drei"_qs), row(u"z"_qs, u"9"_qs)});
        QCOMPARE(inserted.count(), 2);
        QCOMPARE(uids(model), (QStringList{u"x"_qs, u"a"_qs, u"c"_qs, u"z"_qs}));
        // The same again: nothing.
        changed.clear();
        model.setRows({row(u"x"_qs, u"0"_qs), row(u"a"_qs, u"1"_qs), row(u"c"_qs, u"drei"_qs), row(u"z"_qs, u"9"_qs)});
        QCOMPARE(changed.count(), 0);
        // A filter: most go, the rest stay where they were relative to each other.
        model.setRows({row(u"c"_qs, u"drei"_qs)});
        QCOMPARE(uids(model), QStringList{u"c"_qs});
        model.setRows({});
        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(reset.count(), 0);
        QCOMPARE(model.rows().size(), 0);
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
