#pragma once

#include <QColor>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QWindow>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;

// How Mitori looks and speaks: the style (shinki/README.md, section 6), the language and whether
// the first-start wizard ran. Both change at once, with one tap:
//
//   language   the catalog is swapped and the engine retranslates; every page stays (setLanguage).
//   style      the window is built anew: the file selector for app/qml/Look, the component cache
//              cleared, Look's roles read anew, the pages up to the current one back (rebuild).
//              Singletons and so the services stay. Qt's controls style is fixed for the run: a
//              device's styles all share one.
//
//   style      controls (QQC2)        KanteStyle
//   Breeze     org.kde.desktop        System    (only where qqc2-desktop-style is, on Plasma)
//   Material   Material               System    (only on Android, owner 06.11.2026)
//   Kante      Breeze or Material     Kante, dark
//   Leinen     Breeze or Material     Kante, light ("Leinen")
//   Shinki     Breeze or Material     Kante, light   (only with -DMITORI_SHINKI=ON; its own
//                                               controls and look come with app/qml/Look)
class Appearance : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    // The style chosen and the one the window is built in: they differ only until the rebuild,
    // right after the tap.
    Q_PROPERTY(Style style READ style NOTIFY changed)
    Q_PROPERTY(Style running READ running NOTIFY changed)
    Q_PROPERTY(Language language READ language NOTIFY languageChanged)
    // The styles this device offers, in order, in the language: [{style, name, text}].
    Q_PROPERTY(QVariantList styles READ styles NOTIFY languageChanged)
    Q_PROPERTY(bool welcomed READ welcomed NOTIFY changed)
    Q_PROPERTY(QString version READ version CONSTANT)
    // The app's window: Main hands it over (adopt), a rebuild replaces it.
    Q_PROPERTY(QObject *window READ window NOTIFY windowChanged)
    // The window's ground: Android's status and navigation bars follow it (Main binds it).
    Q_PROPERTY(QColor barColor READ barColor WRITE setBarColor NOTIFY barColorChanged)
    Q_PROPERTY(QString keychainLicence READ keychainLicence CONSTANT)

public:
    enum class Style { Breeze, Material, Kante, Leinen, Shinki };
    Q_ENUM(Style)
    enum class Language { System, German, English };
    Q_ENUM(Language)

    explicit Appearance(QObject *parent = nullptr);
    ~Appearance() override;

    Style style() const { return m_style; }
    Style running() const { return m_running; }
    Language language() const { return m_language; }
    bool welcomed() const { return m_welcomed; }
    QString version() const;
    QVariantList styles() const;
    QObject *window() const { return m_window; }
    QColor barColor() const { return m_barColor; }
    void setBarColor(const QColor &color);
    QString keychainLicence() const;

    // A tap: saved, and at once in the app.
    Q_INVOKABLE void setStyle(Style style);
    Q_INVOKABLE void setLanguage(Language language);
    Q_INVOKABLE void adopt(QObject *window);
    // The preview's texts in a language, from its catalog: {title, text, chip, button}.
    Q_INVOKABLE QVariantMap sample(Language language) const;
    // The wizard is done (or skipped): it does not come again.
    Q_INVOKABLE void welcome();

    // For main.cpp, before the first window: the saved style where the device has it, else the
    // default; its controls style; the saved language's locale and catalog.
    static Style startStyle(const QStringList &importPaths);
    static QString controlsStyle(Style style, const QStringList &importPaths);
    static void startLanguage();
    // The locale's name of a language ("de", "en"; empty: the user's).
    static QString localeName(Language language);
    // A window built from Main in the engine; null with a warning when that fails. Main adopts
    // it: the running window is Appearance's to delete.
    static QObject *openWindow(QQmlEngine *engine, const QVariantMap &properties = {});
    // For an engine, before it builds anything: Shinki's fonts and the file selector "shinki", so
    // that app/qml/Look answers with Shinki (its +shinki files); no selector for the others.
    static void prepare(Style style, QQmlEngine *engine);

signals:
    void changed();
    void languageChanged();
    void windowChanged();
    void barColorChanged();

private:
    Style m_style;
    Style m_running;
    Language m_language;
    bool m_welcomed;
    QPointer<QWindow> m_window;
    bool m_rebuildAsked = false;
    QColor m_barColor;

    void rebuild();
    QStringList importPaths() const;
    static bool offers(Style style, const QStringList &importPaths);
};
