// Include the own header
#include "myvision.h"
#include "cnnmanager_v2.h"

#include <QImage>
#include <QLoggingCategory>

static QLoggingCategory lc{"multicnnclassifier.vision"};

void MyVision::process() {
    try {
        // Get the image data
        auto img = image();

        if (!_cnnData.empty()) {
            for (auto& cnn : _cnnData) {
                // process image with deep ocean core.
                // Scale the image to the input size of the cnn. If you don't scale it the NXT Framework will do scaling
                // which can lower performance
                _result[cnn] = cnn.cnnData.processImage(img->getQImage().copy(cnn.roi).scaled(cnn.cnnData.inputSize()),
                                                        QStringLiteral("Classification"));
            }

            img->visionOK("", "");
        } else // Deep ocean core is not initialized. This can happen if no cnn is ativated.
        {
            image()->visionFailed("Deep ocean core is not initialized", "Deep ocean core is not initialized");
        }
    } catch (std::exception& e) {
        qCCritical(lc) << "Error: " << e.what();

        image()->visionFailed("CNN evaluation failed", "CNN evaluation failed");
    }
}

void MyVision::abort() {
    // Abort the vision process
    Vision::abort();
}

void MyVision::setCnnData(const RoiCnnList& roiCnnConfig) {
    _cnnData = roiCnnConfig;
}

MyVision::RoiCnnResultMap MyVision::result() {
    // we need to move the result because it is stored in an unique pointer.
    return std::move(_result);
}
