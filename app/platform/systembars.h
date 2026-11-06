#pragma once

class QColor;

// Android's status and navigation bars in the window's ground, with icons that read on it. Nothing
// on other platforms (a desktop draws its own frame).
void paintSystemBars(const QColor &ground);
