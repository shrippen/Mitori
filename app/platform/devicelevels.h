#pragma once

// The device's screen brightness and media volume, for the player's gestures (swipe up/down on
// the left half: brightness, on the right: volume). One implementation per platform:
//
//   Android         brightness of the app's window (no permission; the system's value returns
//                   when the app leaves), media volume over AudioManager (STREAM_MUSIC)
//   Linux (Plasma)  the screen's brightness over Solid's D-Bus service (PowerDevil); no device
//                   volume (the player turns its own sound down instead)
//   tests           MITORI_DEVICE_LEVELS=memory keeps both in memory, so no test dims the screen
//                   ("memory-screen": only the brightness, as on the desktop)
//
// Levels are 0..1; a negative value means "not available here".
namespace DeviceLevels {

double brightness();
// False when the platform has no brightness to set.
bool setBrightness(double level);
// The brightness the platform had before the first change (Android: the system's again).
void restoreBrightness();

double volume();
bool setVolume(double level);

}
