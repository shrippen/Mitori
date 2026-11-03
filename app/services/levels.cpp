#include "levels.h"

#include "devicelevels.h"

#include <QSettings>
#include <algorithm>

namespace {
constexpr auto kVolumeKey = "player/volume";    // the player's own volume (no device volume)
constexpr auto kSoundKey = "player/sound";
constexpr auto kAwakeKey = "player/awake";
}

Levels::Levels(QObject *parent)
    : QObject(parent)
    , m_ownVolume(QSettings().value(QLatin1String(kVolumeKey), 1.0).toDouble())
    , m_sound(QSettings().value(QLatin1String(kSoundKey), true).toBool())
    , m_awake(QSettings().value(QLatin1String(kAwakeKey), true).toBool())
{
    refresh();
}

double Levels::volume() const
{
    return m_deviceVolume >= 0 ? m_deviceVolume : m_ownVolume;
}

double Levels::playerVolume() const
{
    return m_deviceVolume >= 0 ? 1.0 : m_ownVolume;
}

void Levels::refresh()
{
    m_brightness = DeviceLevels::brightness();
    m_deviceVolume = DeviceLevels::volume();
    emit changed();
}

bool Levels::change(Level level, double delta)
{
    if (level == Level::Brightness) {
        if (m_brightness < 0 || !DeviceLevels::setBrightness(m_brightness + delta)) {
            return false;
        }
        m_brightness = std::clamp(m_brightness + delta, 0.0, 1.0);
        emit changed();
        return true;
    }

    const double next = std::clamp(volume() + delta, 0.0, 1.0);
    if (m_deviceVolume >= 0) {
        if (!DeviceLevels::setVolume(next)) {
            return false;
        }
        m_deviceVolume = next;
    } else {
        m_ownVolume = next;
        QSettings().setValue(QLatin1String(kVolumeKey), next);
    }
    emit changed();
    return true;
}

void Levels::restore()
{
    DeviceLevels::restoreBrightness();
    m_brightness = DeviceLevels::brightness();
    m_playing = false;
    hold();
    emit changed();
}

bool Levels::held() const
{
    return DeviceLevels::screenHeld();
}

void Levels::setPlayback(Playback state)
{
    m_playing = state == Playback::Playing;
    hold();
    emit changed();
}

void Levels::setSound(Switch state)
{
    m_sound = state == Switch::On;
    QSettings().setValue(QLatin1String(kSoundKey), m_sound);
    emit changed();
}

void Levels::setAwake(Switch state)
{
    m_awake = state == Switch::On;
    QSettings().setValue(QLatin1String(kAwakeKey), m_awake);
    hold();
    emit changed();
}

// The screen stays on while a video plays and the switch is on.
void Levels::hold()
{
    DeviceLevels::holdScreen(m_playing && m_awake ? DeviceLevels::Hold::Keep : DeviceLevels::Hold::Release);
}
