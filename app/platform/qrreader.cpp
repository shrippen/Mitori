#include "qrreader.h"

#include <QImage>
#include <QVideoFrame>

#include "ReadBarcode.h"

namespace {

QString readLuma(const uchar *data, int width, int height, int stride)
{
    // One symbol on a printed insert: no inverted or rotated tries (QR finds its own rotation).
    const auto options = ZXing::ReaderOptions()
                             .formats(ZXing::BarcodeFormat::QRCode)
                             .tryInvert(false)
                             .tryRotate(false)
                             .maxNumberOfSymbols(1);
    const ZXing::ImageView view(data, width, height, ZXing::ImageFormat::Lum, stride);
    const ZXing::Barcode barcode = ZXing::ReadBarcode(view, options);
    return barcode.isValid() ? QString::fromStdString(barcode.text()) : QString();
}

// Whether the frame's first plane holds the brightness: nothing to convert (most cameras).
bool startsWithLuma(QVideoFrameFormat::PixelFormat format)
{
    switch (format) {
    case QVideoFrameFormat::Format_NV12:
    case QVideoFrameFormat::Format_NV21:
    case QVideoFrameFormat::Format_YUV420P:
    case QVideoFrameFormat::Format_YV12:
    case QVideoFrameFormat::Format_Y8:
        return true;
    default:
        return false;
    }
}

} // namespace

namespace QrReader {

QString read(const QImage &image)
{
    if (image.isNull()) {
        return {};
    }
    const QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    return readLuma(gray.constBits(), gray.width(), gray.height(), int(gray.bytesPerLine()));
}

QString read(const QVideoFrame &input)
{
    QVideoFrame frame = input;      // map() is not const; the pixels stay shared
    if (!frame.isValid()) {
        return {};
    }
    if (!startsWithLuma(frame.pixelFormat())) {
        return read(frame.toImage());
    }
    if (!frame.map(QVideoFrame::ReadOnly)) {
        return {};
    }
    const QString text = readLuma(frame.bits(0), frame.width(), frame.height(), frame.bytesPerLine(0));
    frame.unmap();
    return text;
}

} // namespace QrReader
