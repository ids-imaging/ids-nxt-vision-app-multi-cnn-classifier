#include "myapp.h"
#include <cnnmanager_v2.h>
#include <QFontDatabase>
#include <QLoggingCategory>

static QLoggingCategory lc{"multicnnclassifier.app"};

using namespace IDS::NXT::CNNv2;
using namespace IDS::NXT;

MyApp::MyApp(int& argc, char** argv)
  : IDS::NXT::VApp{argc, argv}
  , _engine{_resultcollection} {
    // Enable the Deep Ocean Core to load multiple CNNs simultaniously
    CnnManager::getInstance().enableLoadingMultipleCnns(true);

    // Create our result source collection
    _resultcollection.createSource("data", IDS::NXT::ResultType::String);

    // Load the font for our result image
    QFontDatabase::addApplicationFont(VApp::vappAppDirectory() + "DejaVuSans.ttf");
}

void MyApp::imageAvailable(std::shared_ptr<IDS::NXT::Hardware::Image> image) {
    if (_engine.isInitialized()) {
        _engine.handleImage(image);
    } else {
        _resultcollection.addResult("data",
                                    QStringLiteral("No CNN/ROI configuration set"),
                                    QStringLiteral("No CNN/ROI configuration set"),
                                    image);
        _resultcollection.finishedAllParts(image);
    }

    // Release the image explicitly to allow the NXT framework to capture the next image
    image = nullptr;
}

void MyApp::abortVision() {
    _engine.abortVision();
}
