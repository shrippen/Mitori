#pragma once

#include <QQuickItem>
#include <QSharedPointer>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include "appearance.h"

class QQmlEngine;

// A small sample of a style, drawn by the style's own Look components (qml/LookSample.qml), not by
// a picture. Look answers per engine (file selector, Look and KanteStyle singletons), so each style
// has an engine of its own, shared by the previews on screen and gone with the last of them. The
// sample's item lives in this item's window:
//
//   window's engine     AppearancePage ── LookPreview
//                                              │ parent item
//   the style's engine                    LookSample ── LookCard, LookButton, LookChip …
//
// Nothing in the sample reaches a service: it gets its texts from the page.
class LookPreview : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Appearance::Style look READ look WRITE setLook NOTIFY lookChanged)
    Q_PROPERTY(QVariantMap texts READ texts WRITE setTexts NOTIFY textsChanged)
    // The sample stands (it is built just after the page, one tile at a time).
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    Q_PROPERTY(QObject *sample READ sample NOTIFY readyChanged)

public:
    explicit LookPreview(QQuickItem *parent = nullptr);
    ~LookPreview() override;

    Appearance::Style look() const { return m_look; }
    void setLook(Appearance::Style look);
    QVariantMap texts() const { return m_texts; }
    void setTexts(const QVariantMap &texts);
    bool ready() const { return m_sample != nullptr; }
    QObject *sample() const;

signals:
    void lookChanged();
    void textsChanged();
    void readyChanged();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &now, const QRectF &before) override;

private:
    Appearance::Style m_look = Appearance::Style::Leinen;
    QVariantMap m_texts;
    QSharedPointer<QQmlEngine> m_engine;
    QQuickItem *m_sample = nullptr;
    bool m_buildAsked = false;

    void askBuild();
    void build();
    void drop();
};
