#pragma once
#include <QImage>

#include "image.h"
#include "resultimage.h"

class MyResultImage : public IDS::NXT::ResultImage {
    Q_OBJECT
public:
    struct overlayData {
        QPair<QString, float> result;
        QRect roi;
        int color = 0;
    };

    MyResultImage(const QByteArray& name);

    void setImageWithOverlay(const QImage& fullImage,
                             const std::shared_ptr<IDS::NXT::Hardware::Image>& image,
                             const QList<overlayData>& overlay);
    void setImage(const QImage& image, const std::shared_ptr<IDS::NXT::Hardware::Image>& nxtImage);
    QImage getImage() const override;

private:
    QImage _image;

    const QColor _idsBlueLight = QColor(119, 203, 210);
    const QColor _idsBlue = QColor(0, 138, 150);
    const QColor _idsGreen = QColor(81, 192, 119);
    const QColor _idsGray = QColor(202, 202, 201);
    const QColor _idsYellow = QColor(255, 204, 96);
    const QColor _idsOrange = QColor(255, 173, 100);
    const QColor _errorRed = QColor("red");
};
