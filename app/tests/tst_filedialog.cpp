#include <QDir>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QtTest>

#include "typedpath.h"

using namespace Qt::StringLiterals;

// QA round of 5 Nov 2026: a full path typed into the save dialog failed. This drives Qt's own
// dialog (the one the desktop gets, see platform/typedpath.h) with the keys and resolves what it
// answers, so a Qt that changes the answer shows here.
//
// The folders are set here, not left to TMPDIR: Qt adds the suffix only when the whole path has no
// dot, so the same run differs between /tmp and ~/.cache.
class TstFileDialog : public QObject
{
    Q_OBJECT

    static constexpr auto kSuffix = "zip";

    // The dialog's items hang on parent items, not on QObject children.
    static QQuickItem *findItem(QQuickItem *item, const QString &name)
    {
        if (item->objectName() == name) {
            return item;
        }
        for (QQuickItem *child : item->childItems()) {
            if (QQuickItem *found = findItem(child, name)) {
                return found;
            }
        }
        return nullptr;
    }

    // The dialog's name field in the window that shows it; nullptr under a native dialog.
    static QQuickItem *nameField(QQuickWindow **window)
    {
        for (QWindow *w : QGuiApplication::allWindows()) {
            auto *quick = qobject_cast<QQuickWindow *>(w);
            if (!quick || !quick->isVisible()) {
                continue;
            }
            if (QQuickItem *field = findItem(quick->contentItem(), u"fileNameTextField"_s)) {
                *window = quick;
                return field;
            }
        }
        return nullptr;
    }

private slots:
    void typedIntoNameField_data()
    {
        QTest::addColumn<QString>("folderName");
        QTest::addColumn<QString>("typed");
        QTest::addColumn<QString>("expected");     // "<folder>" and "<home>" stand for those

        const QString plain = u"plain"_s;
        QTest::newRow("name") << plain << u"x.zip"_s << u"<folder>/x.zip"_s;
        QTest::newRow("name, no suffix") << plain << u"x"_s << u"<folder>/x.zip"_s;
        QTest::newRow("subfolder") << plain << u"sub/x.zip"_s << u"<folder>/sub/x.zip"_s;
        QTest::newRow("full path") << plain << u"/var/x/y.zip"_s << u"/var/x/y.zip"_s;
        QTest::newRow("full path, no suffix") << plain << u"/var/x/y"_s << u"/var/x/y.zip"_s;
        QTest::newRow("home") << plain << u"~/y.zip"_s << u"<home>/y.zip"_s;
        QTest::newRow("home itself") << plain << u"~/"_s << u"<home>/"_s;

        // After Save with an empty name field, or typing "sub/" and Save: Qt's folder ends in "/".
        const QString slash = u"plain/"_s;
        QTest::newRow("slash: name") << slash << u"y.zip"_s << u"<folder>/y.zip"_s;
        QTest::newRow("slash: subfolder") << slash << u"sub/y.zip"_s << u"<folder>/sub/y.zip"_s;
        QTest::newRow("slash: full path") << slash << u"/var/x/y.zip"_s << u"/var/x/y.zip"_s;

        // A dot in the folder (~/.cache, ~/Dr.Weber): Qt adds no suffix at all.
        const QString dotted = u"dotted.folder"_s;
        QTest::newRow("dot: name") << dotted << u"y"_s << u"<folder>/y.zip"_s;
        QTest::newRow("dot: name with suffix") << dotted << u"y.zip"_s << u"<folder>/y.zip"_s;
        QTest::newRow("dot: other suffix") << dotted << u"y.alt"_s << u"<folder>/y.alt.zip"_s;
        QTest::newRow("dot: full path") << dotted << u"/var/x/y"_s << u"/var/x/y.zip"_s;
    }

    void typedIntoNameField()
    {
        QFETCH(QString, folderName);
        QFETCH(QString, typed);
        QFETCH(QString, expected);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(QDir(dir.path()).mkpath(folderName));
        const QUrl folder = QUrl::fromLocalFile(dir.filePath(folderName));
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(QStringLiteral("import QtQuick\nimport QtQuick.Dialogs\n"
                                         "Window {\n visible: true\n width: 800\n height: 600\n"
                                         " property alias dialog: fileDialog\n FileDialog {\n  id: fileDialog\n"
                                         "  fileMode: FileDialog.SaveFile\n defaultSuffix: 'zip'\n"
                                         "  nameFilters: ['ZIP (*.zip)']\n currentFolder: '%1'\n }\n}\n")
                              .arg(folder.toString())
                              .toUtf8(),
                          QUrl());
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        QVERIFY(QTest::qWaitForWindowExposed(qobject_cast<QWindow *>(root.get())));
        auto *dialog = root->property("dialog").value<QObject *>();
        QVERIFY(QMetaObject::invokeMethod(dialog, "open"));

        QQuickWindow *window = nullptr;
        QTRY_VERIFY(nameField(&window));
        QQuickItem *field = nameField(&window);
        QVERIFY(QTest::qWaitForWindowActive(window));
        field->forceActiveFocus();
        QTRY_VERIFY(field->hasActiveFocus());
        for (const QChar c : typed) {
            QTest::keyClick(static_cast<QWindow *>(window), c.toLatin1());
        }
        QCOMPARE(field->property("text").toString(), typed);
        QTest::keyClick(static_cast<QWindow *>(window), Qt::Key_Return);      // the text goes to the dialog

        QString name = folderName;
        if (name.endsWith(u'/')) {
            name.chop(1);
        }
        expected.replace(u"<folder>"_s, dir.filePath(name)).replace(u"<home>"_s, QDir::homePath());
        const QUrl picked = dialog->property("selectedFile").toUrl();
        QCOMPARE(TypedPath::resolve(picked, dialog->property("currentFolder").toUrl(), kSuffix), QUrl::fromLocalFile(expected));
        QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
    }

    // Without Qt's dialog: the path field's answers and what is not typed at all.
    void resolve_data()
    {
        QTest::addColumn<QUrl>("picked");
        QTest::addColumn<QUrl>("folder");
        QTest::addColumn<QUrl>("expected");

        const QUrl docs(u"file:///docs"_s);
        QTest::newRow("plain pick") << QUrl(u"file:///docs/x.zip"_s) << docs << QUrl(u"file:///docs/x.zip"_s);
        QTest::newRow("another folder") << QUrl(u"file:///else/x.zip"_s) << docs << QUrl(u"file:///else/x.zip"_s);
        QTest::newRow("path field, relative") << QUrl(u"file:x.zip"_s) << docs << QUrl(u"file:///docs/x.zip"_s);
        QTest::newRow("path field, home") << QUrl(u"file:~/x.zip"_s) << docs << QUrl::fromLocalFile(QDir::homePath() + u"/x.zip"_s);
        QTest::newRow("spaces") << QUrl(u"file:///docs//var/a%20b.zip"_s) << docs << QUrl(u"file:///var/a%20b.zip"_s);
        QTest::newRow("in the root") << QUrl(u"file:////var/x.zip"_s) << QUrl(u"file:///"_s) << QUrl(u"file:///var/x.zip"_s);
        QTest::newRow("name in the root") << QUrl(u"file:///x.zip"_s) << QUrl(u"file:///"_s) << QUrl(u"file:///x.zip"_s);
        QTest::newRow("a folder called so") << QUrl(u"file:///docs/~x/y.zip"_s) << docs << QUrl(u"file:///docs/~x/y.zip"_s);
        QTest::newRow("Android") << QUrl(u"content://a/b%2Fc"_s) << QUrl(u"content://a"_s) << QUrl(u"content://a/b%2Fc"_s);

        const QUrl slash(u"file:///docs/"_s);
        QTest::newRow("slash: a pick") << QUrl(u"file:///docs/x.zip"_s) << slash << QUrl(u"file:///docs/x.zip"_s);
        QTest::newRow("slash: name") << QUrl(u"file:///docs//y.zip"_s) << slash << QUrl(u"file:///docs/y.zip"_s);
        QTest::newRow("slash: full path") << QUrl(u"file:///docs///var/y.zip"_s) << slash << QUrl(u"file:///var/y.zip"_s);
        QTest::newRow("slash: in a subfolder") << QUrl(u"file:///docs/sub//y.zip"_s) << QUrl(u"file:///docs/sub/"_s)
                                                << QUrl(u"file:///docs/sub/y.zip"_s);
        QTest::newRow("capital suffix") << QUrl(u"file:///docs/x.ZIP"_s) << docs << QUrl(u"file:///docs/x.ZIP"_s);
        QTest::newRow("other suffix") << QUrl(u"file:///docs/x.alt"_s) << docs << QUrl(u"file:///docs/x.alt.zip"_s);
        QTest::newRow("no suffix, dot in the folder") << QUrl(u"file:///do.cs/x"_s) << QUrl(u"file:///do.cs"_s)
                                                       << QUrl(u"file:///do.cs/x.zip"_s);
        QTest::newRow("a folder, typed") << QUrl(u"file:///var/"_s) << docs << QUrl(u"file:///var/"_s);
    }

    void resolve()
    {
        QFETCH(QUrl, picked);
        QFETCH(QUrl, folder);
        QFETCH(QUrl, expected);

        QCOMPARE(TypedPath::resolve(picked, folder, kSuffix), expected);
    }

    // An existing folder stays one: no "folder.zip" next to it.
    void folderKeepsItsName()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QUrl folder = QUrl::fromLocalFile(dir.path());
        QVERIFY(QDir(dir.path()).mkdir(u"d"_s));
        const QUrl inside = QUrl::fromLocalFile(dir.filePath(u"d"_s));
        QCOMPARE(TypedPath::resolve(inside, folder, kSuffix), inside);
    }
};

QTEST_MAIN(TstFileDialog)
#include "tst_filedialog.moc"
