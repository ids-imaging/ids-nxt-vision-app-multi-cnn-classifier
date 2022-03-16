#include "myresultimage.h"

#include <QLoggingCategory>
#include <QPainter>
#include <QRawFont>

static QLoggingCategory lc{"multicnnclassifier.customresultimage"};

static constexpr int FONT_PIXEL_SIZE = 30;
static constexpr int FONT_MINIMAL_PIXEL_SIZE = 5;
static constexpr int PEN_WIDTH = 3;
static constexpr float BOX_SCALING_FACTOR = 0.8;

MyResultImage::MyResultImage(const QByteArray& name)
  : ResultImage(name) {}

void MyResultImage::setImageWithOverlay(const QImage& fullImage,
                                        const std::shared_ptr<IDS::NXT::Hardware::Image>& image,
                                        const QList<overlayData>& overlay) {
    if (!image) {
        setModified(QLatin1String(""));
        return;
    }

    auto imageFormat = fullImage.format();
    if (imageFormat == QImage::Format_Mono or imageFormat == QImage::Format_Grayscale8) {
        imageFormat = QImage::Format_RGB888;
    }

    QImage outImage(fullImage.width(), fullImage.height(), imageFormat);
    try {
        QFont DejaVuSans(QStringLiteral("DejaVuSans"));
        DejaVuSans.setPixelSize(FONT_PIXEL_SIZE);

        QPainter painter;
        painter.begin(&outImage);
        painter.drawImage(0, 0, fullImage);
        auto pen = QPen();
        pen.setWidth(PEN_WIDTH);
        painter.setFont(DejaVuSans);

        auto fontStartFont = painter.font();
        auto fontStartPixelSize = fontStartFont.pixelSize();
        quint32 index = 1;
        if (!overlay.empty()) {
            for (const auto& val : overlay) {
                // Reset Font on every ROI
                fontStartFont.setPixelSize(fontStartPixelSize);
                painter.setFont(fontStartFont);

                QColor color;
                auto thisClass = val.result.first;
                thisClass.prepend(QString::number(index++) + ": ");
                const auto thisProbability = QString::number(val.result.second, 'f', 2).left(4);
                const auto currentRoi = val.roi;
                color = _idsBlueLight;
                pen.setColor(color);
                painter.setPen(pen);
                painter.drawRect(currentRoi); // Detected Box

                const auto roiText = thisClass + " " + thisProbability;
                auto textbox = painter.fontMetrics().boundingRect(roiText);
                qCDebug(lc) << "Textbox original" << textbox << "Text" << roiText;
                while (currentRoi.width() < textbox.width()) {
                    auto f = painter.font();
                    qCDebug(lc) << f.pixelSize();
                    if (f.pixelSize() < FONT_MINIMAL_PIXEL_SIZE) {
                        qCDebug(lc) << "Font too small";
                        f.setPixelSize(FONT_MINIMAL_PIXEL_SIZE);
                        painter.setFont(f);
                        textbox = painter.fontMetrics().boundingRect(roiText);
                        break;
                    }

                    f.setPixelSize(static_cast<int>(static_cast<float>(f.pixelSize()) * BOX_SCALING_FACTOR));
                    qCDebug(lc) << f.pixelSize();
                    painter.setFont(f);
                    textbox = painter.fontMetrics().boundingRect(roiText);
                }
                if (currentRoi.y() >= textbox.height()) {
                    textbox = QRect(currentRoi.x(),
                                    currentRoi.y() - textbox.height(),
                                    textbox.width(),
                                    textbox.height());
                } else {
                    textbox = QRect(currentRoi.x(),
                                    currentRoi.y() + currentRoi.height() - textbox.height(),
                                    textbox.width(),
                                    textbox.height());
                }
                if (!fullImage.rect().contains(textbox, true)) {
                    textbox = QRect(currentRoi.x(),
                                    currentRoi.y() + currentRoi.height() - textbox.height(),
                                    textbox.width(),
                                    textbox.height());
                }
                painter.fillRect(textbox, color);
                qCDebug(lc) << textbox << roiText;
                pen.setColor(QColor("black"));
                painter.setPen(pen);
                painter.drawText(textbox, Qt::AlignLeft, roiText);
            }
        }
        painter.end();
        _image = outImage;
    } catch (...) {
        qCDebug(lc) << "Drawing failed.";
    }

    setModified(image->key());
}

QImage MyResultImage::getImage() const {
    if (!_image.isNull()) {
        return _image;
    }

    return QImage{};
}

void MyResultImage::setImage(const QImage& image, const std::shared_ptr<IDS::NXT::Hardware::Image>& nxtImage) {
    _image = image;

    setModified(nxtImage->key());
}
