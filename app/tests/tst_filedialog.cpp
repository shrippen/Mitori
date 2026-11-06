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
class TstFileDialog : public QObject
{
    Q_OBJECT

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
        QTest::addColumn<QString>("typed");
        QTest::addColumn<QString>("expected");     // "<folder>" and "<home>" stand for those

        QTest::newRow("name") << u"x.zip"_s << u"<folder>/x.zip"_s;
        QTest::newRow("subfolder") << u"sub/x.zip"_s << u"<folder>/sub/x.zip"_s;
        QTest::newRow("full path") << u"/var/x/y.zip"_s << u"/var/x/y.zip"_s;
        QTest::newRow("full path, no suffix") << u"/var/x/y"_s << u"/var/x/y.zip"_s;
        QTest::newRow("home") << u"~/y.zip"_s << u"<home>/y.zip"_s;
    }

    void typedIntoNameField()
    {
        QFETCH(QString, typed);
        QFETCH(QString, expected);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QUrl folder = QUrl::fromLocalFile(dir.path());
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

        expected.replace(u"<folder>"_s, dir.path()).replace(u"<home>"_s, QDir::homePath());
        const QUrl picked = dialog->property("selectedFile").toUrl();
        QCOMPARE(TypedPath::resolve(picked, folder), QUrl::fromLocalFile(expected));
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
    }

    void resolve()
    {
        QFETCH(QUrl, picked);
        QFETCH(QUrl, folder);
        QFETCH(QUrl, expected);

        QCOMPARE(TypedPath::resolve(picked, folder), expected);
    }
};

QTEST_MAIN(TstFileDialog)
#include "tst_filedialog.moc"
