#include "myengine.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QStringList>

#include <cmath>

static QLoggingCategory lc{"multicnnclassifier.engine"};

using namespace IDS::NXT;
using namespace IDS::NXT::CNNv2;

static constexpr int MAX_RESULT_VALUES = 5;

MyEngine::MyEngine(IDS::NXT::ResultSourceCollection& resultcollection)
  : _resultCollection{resultcollection}
  , _createResultImage{"createresultimage", false}
  , _resultImage{nullptr} {
    // connect configurable bool (switch) changed-event
    connect(&_createResultImage, &IDS::NXT::ConfigurableBool::changed, this, &MyEngine::enableResultImage);
}

bool MyEngine::isInitialized() const {
    return !_cnnRoiHandler.activeRoiCnnList().isEmpty();
}

std::shared_ptr<IDS::NXT::Vision> MyEngine::factoryVision() {
    // Simply construct a vision object, we may give further parameters, such as not-changing
    // parameters or shared (thread-safe!) objects.
    return std::make_shared<MyVision>();
}

void MyEngine::setupVision(std::shared_ptr<IDS::NXT::Vision> vision) {
    // Here we could set changing parameters, such as current configurable values
    if (vision) {
        auto obj = std::static_pointer_cast<MyVision>(vision);

        // set current activated CNNs because they could change during runtime.
        obj->setCnnData(_cnnRoiHandler.activeRoiCnnList());
    }
}

void MyEngine::handleResult(std::shared_ptr<IDS::NXT::Vision> vision) {
    // Get the finished vision object
    const auto obj = std::static_pointer_cast<MyVision>(vision);

    try {
        // extracting the inference result of the CNN ...
        const auto allResults = obj->result();

        QList<MyResultImage::overlayData> drawData;
        if (_resultImage) {
            drawData.reserve(static_cast<size_t>(allResults.size()));
        }

        for (const auto& oneResult : allResults) {
            const auto& cnnDataStruct = oneResult.first;
            const auto& thisCnnResult = oneResult.second;
            const auto& cnnData = cnnDataStruct.cnnData;
            const std::unique_ptr<IDS::NXT::CNNv2::MultiBuffer>& cnnResult = thisCnnResult;

            // Get the output buffers of the CNN
            const auto& allBuffers = cnnResult->allBuffers();

            // Check if result is suited for classification
            if (cnnData.inferenceType() != CnnData::InferenceType::Classification || allBuffers.size() != 1) {
                throw std::runtime_error(
                    "Error: CNN is not suited for classification or output buffer set is not specified correctly.");
            }

            // Convert result to double and map corresponding class;
            QList<QPair<QString, double>> resultClasses;
            resultClasses.reserve(cnnData.classes().size());
            auto expSum = 0.;

            // Classification CNNs have only one output Buffer
            auto currentCnnOutputBuffer = allBuffers.at(0);
            for (auto cnt = 0; cnt < cnnData.classes().size(); cnt++) {
                const auto currentClass = cnnData.classes().at(cnt);
                // Apply exponential function for softmax calculation;
                const auto currentVal = std::exp(static_cast<double>(currentCnnOutputBuffer.data[cnt]));
                // Sum up values for softmax dividor
                expSum += currentVal;
                resultClasses.append(qMakePair(currentClass, currentVal));
            }

            // sort classes
            qSort(resultClasses.begin(), resultClasses.end(), ProbabilityComparer());

            // create result for resultSourceCollection
            QJsonObject thisJsonResult;
            thisJsonResult.insert(QStringLiteral("CNN"), cnnData.name());
            thisJsonResult.insert(QStringLiteral("ROI"), cnnDataStruct.roiName);
            thisJsonResult.insert(QStringLiteral("Result"), resultToJson(resultClasses, expSum, true));
            _resultCollection.addResult("data",
                                        QJsonDocument(thisJsonResult).toJson(QJsonDocument::Compact),
                                        cnnDataStruct.roiName,
                                        vision->image());

            // create result image
            if (_resultImage) {
                MyResultImage::overlayData overlay;
                overlay.result = qMakePair(resultClasses.at(0).first, resultClasses.at(0).second / expSum);
                overlay.roi = cnnDataStruct.roi;

                drawData.append(overlay);
            }
        }

        if (_resultImage) {
            _resultImage->setImageWithOverlay(obj->image()->getQImage(), obj->image(), drawData);
        }
    } catch (const std::runtime_error& e) {
        qCCritical(lc) << "Error handling result: " << e.what();
        _resultCollection.addResult("data", e.what(), QStringLiteral("Content1"), vision->image());
    }
    // signal that all parts of the image are finished
    _resultCollection.finishedAllParts(obj->image());

    // Remove the image pointer from the vision object and thereby allow the framework to
    // reuse the image buffer.
    obj->setImage(nullptr);
}

void MyEngine::enableResultImage(bool enable) {
    if (enable) {
        _resultImage = std::make_unique<MyResultImage>("resultimage");
    } else {
        _resultImage = nullptr;
    }
}

QJsonArray MyEngine::resultToJson(const QList<QPair<QString, double>>& results, double expsum, bool limitResults) {
    QJsonArray output;

    auto resultCnt = 0;
    for (const auto& result : results) {
        QJsonObject resultJson;
        resultJson.insert(QStringLiteral("Class"), result.first);
        resultJson.insert(QStringLiteral("Probability"), QString::number(result.second / expsum, 'f', 2).toDouble());
        output.append(resultJson);

        if (limitResults && ++resultCnt > MAX_RESULT_VALUES) {
            break;
        }
    }

    return output;
}
