#include "lookpreview.h"

#include <QHash>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QWeakPointer>

namespace {

// The style's engine; a new one when no preview holds it any more.
QSharedPointer<QQmlEngine> engineOf(Appearance::Style look)
{
    static QHash<int, QWeakPointer<QQmlEngine>> engines;
    QSharedPointer<QQmlEngine> engine = engines.value(int(look)).toStrongRef();
    if (engine) {
        return engine;
    }
    // Later: a sample's item may have deletions posted that still touch it.
    engine = QSharedPointer<QQmlEngine>(new QQmlEngine, &QObject::deleteLater);
    Appearance::prepare(look, engine.data());
    engines.insert(int(look), engine);
    return engine;
}

} // namespace

LookPreview::LookPreview(QQuickItem *parent)
    : QQuickItem(parent)
{
}

LookPreview::~LookPreview()
{
    drop();
}

void LookPreview::setLook(Appearance::Style look)
{
    if (look == m_look) {
        return;
    }
    m_look = look;
    emit lookChanged();
    askBuild();
}

void LookPreview::setTexts(const QVariantMap &texts)
{
    if (texts == m_texts) {
        return;
    }
    m_texts = texts;
    emit textsChanged();
    if (m_sample) {
        m_sample->setProperty("texts", m_texts);
    }
}

QObject *LookPreview::sample() const
{
    return m_sample;
}

void LookPreview::componentComplete()
{
    QQuickItem::componentComplete();
    askBuild();
}

void LookPreview::geometryChange(const QRectF &now, const QRectF &before)
{
    QQuickItem::geometryChange(now, before);
    if (m_sample) {
        m_sample->setWidth(now.width());
    }
}

// Not within the page's own creation: the page stands first, the samples follow.
void LookPreview::askBuild()
{
    if (m_buildAsked || !isComponentComplete()) {
        return;
    }
    m_buildAsked = true;
    QMetaObject::invokeMethod(this, &LookPreview::build, Qt::QueuedConnection);
}

void LookPreview::build()
{
    m_buildAsked = false;
    drop();
    m_engine = engineOf(m_look);

    QQmlComponent component(m_engine.data());
    component.loadFromModule("Mitori", "LookSample");
    const bool kante = m_look == Appearance::Style::Kante;
    const bool leinen = m_look == Appearance::Style::Leinen;
    QObject *made = component.createWithInitialProperties({{QStringLiteral("texts"), m_texts},
                                                           {QStringLiteral("kanteDark"), kante},
                                                           {QStringLiteral("kanteLight"), leinen}});
    m_sample = qobject_cast<QQuickItem *>(made);
    if (!m_sample) {
        qWarning().noquote() << component.errorString();
        delete made;
        return;
    }
    m_sample->setParentItem(this);
    m_sample->setWidth(width());
    const auto follow = [this] { setImplicitHeight(m_sample->implicitHeight()); };
    connect(m_sample, &QQuickItem::implicitHeightChanged, this, follow);
    follow();
    emit readyChanged();
}

void LookPreview::drop()
{
    delete m_sample;
    m_sample = nullptr;
    m_engine.reset();
}
