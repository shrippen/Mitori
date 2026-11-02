#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

// Brightness and volume for the player's gestures (platform/devicelevels). Where the device has
// no volume of its own to set (the desktop), the player's sound goes up and down instead:
// `playerVolume` is what its AudioOutput plays at (1 where the device volume does the work).
//
//   swipe on the left half   change(Level::Brightness, ±share of the picture's height)
//   swipe on the right half  change(Level::Volume, …)
//   leaving the player       restore()   (the screen as before)
class Levels : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    // 0..1, or -1 where the platform has none (then the gesture does nothing).
    Q_PROPERTY(double brightness READ brightness NOTIFY changed)
    Q_PROPERTY(double volume READ volume NOTIFY changed)
    Q_PROPERTY(double playerVolume READ playerVolume NOTIFY changed)

public:
    enum class Level { Brightness, Volume };
    Q_ENUM(Level)

    explicit Levels(QObject *parent = nullptr);

    double brightness() const { return m_brightness; }
    double volume() const;
    double playerVolume() const;

    // Reads the device's levels anew (the volume keys may have changed them).
    Q_INVOKABLE void refresh();
    // Moves a level by `delta` (−1..1); false where the platform has none.
    Q_INVOKABLE bool change(Level level, double delta);
    Q_INVOKABLE void restore();

signals:
    void changed();

private:
    double m_brightness = -1;
    double m_deviceVolume = -1;
    double m_ownVolume = 1;
};
