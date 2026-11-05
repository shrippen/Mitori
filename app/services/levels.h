#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

// Brightness and volume for the player's gestures (platform/devicelevels). Where the device has
// no volume of its own to set (the desktop), the player's sound goes up and down instead:
// `playerVolume` is what its AudioOutput plays at (1 where the device volume does the work).
//
//   swipe on the left half   change(Level::Brightness, ±share of the picture's height)
//   swipe on the right half  change(Level::Volume, …)
//   video plays / stops      setPlayback()  (keeps the screen on while it plays, if `awake`)
//   leaving the player       restore()   (the screen as before)
//
// The player's switches stay for the next start: `sound` (off: silent also at full tempo) and
// `awake` (the screen stays on while a video plays). Both are on by default.
class Levels : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    // 0..1, or -1 where the platform has none (then the gesture does nothing).
    Q_PROPERTY(double brightness READ brightness NOTIFY changed)
    Q_PROPERTY(double volume READ volume NOTIFY changed)
    Q_PROPERTY(double playerVolume READ playerVolume NOTIFY changed)
    Q_PROPERTY(bool sound READ sound NOTIFY changed)
    Q_PROPERTY(bool awake READ awake NOTIFY changed)
    // The screen is kept on now.
    Q_PROPERTY(bool held READ held NOTIFY changed)

public:
    enum class Level { Brightness, Volume };
    Q_ENUM(Level)
    enum class Switch { Off, On };
    Q_ENUM(Switch)
    enum class Playback { Stopped, Playing };
    Q_ENUM(Playback)

    explicit Levels(QObject *parent = nullptr);

    double brightness() const { return m_brightness; }
    double volume() const;
    double playerVolume() const;
    bool sound() const { return m_sound; }
    bool awake() const { return m_awake; }
    bool held() const;

    // Reads the device's levels anew (the volume keys may have changed them).
    Q_INVOKABLE void refresh();
    // Moves a level by `delta` (−1..1); false where the platform has none.
    Q_INVOKABLE bool change(Level level, double delta);
    Q_INVOKABLE void setPlayback(Playback state);
    Q_INVOKABLE void restore();
    Q_INVOKABLE void setSound(Switch state);
    Q_INVOKABLE void setAwake(Switch state);
    // Full screen player: the screen sideways (Android).
    Q_INVOKABLE void setSideways(Switch state);

signals:
    void changed();

private:
    double m_brightness = -1;
    double m_deviceVolume = -1;
    double m_ownVolume = 1;
    bool m_sound = true;
    bool m_awake = true;
    bool m_playing = false;     // a video plays (until it stops or the player is left)

    void hold();
};
