#pragma once

#include <QString>

class QImage;
class QVideoFrame;

// The QR code in a picture, as text (ZXing-C++ stays behind this). Takes a moment: any thread but
// the UI's. "" when the picture holds no readable QR code.
namespace QrReader {

QString read(const QImage &image);
QString read(const QVideoFrame &frame);

} // namespace QrReader
