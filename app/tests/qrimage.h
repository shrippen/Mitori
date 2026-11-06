#pragma once

#include <QImage>
#include <QString>

#include "BitMatrix.h"
#include "MultiFormatWriter.h"

// A QR code as a picture, drawn in the test (the app itself has no writer). Nothing real in it:
// the codes come from the test data.
inline QImage qrImage(const QString &text, int size = 360)
{
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    const ZXing::BitMatrix matrix = ZXing::MultiFormatWriter(ZXing::BarcodeFormat::QRCode).setMargin(32).encode(text.toStdString(), size, size);
    QT_WARNING_POP

    QImage image(matrix.width(), matrix.height(), QImage::Format_Grayscale8);
    for (int y = 0; y < image.height(); ++y) {
        uchar *line = image.scanLine(y);
        for (int x = 0; x < image.width(); ++x) {
            line[x] = matrix.get(x, y) ? 0 : 255;
        }
    }
    return image;
}
