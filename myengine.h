#pragma once

#include <QJsonArray>
#include <memory>

#include <cnnmanager_v2.h>
#include <configurablebool.h>
#include <engine.h>
#include <resultsourcecollection.h>

#include "cnnroihandler.h"
#include "myresultimage.h"

/**
 * @brief The app-specific engine
 */
class MyEngine : public IDS::NXT::Engine {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param resultcollection The result collection object
     *
     * This function constructs the engine object, further parameters could be inserted if
     * app-global interaction objects should be used in a multi-engine vision app.
     */
    MyEngine(IDS::NXT::ResultSourceCollection& resultcollection);

    /**
     * @brief Getter for initialization attribute
     * @return True if CNN/ROI config is set
     */
    bool isInitialized() const;

protected:
    /**
     * @brief Factory function for vision objects
     * @return Factored vision object
     *
     * This method factors a vision object, which depends on our implementation. This method
     * is called whenever the framework needs a new vision object, these objects are reused.
     * Do set up changing variables inside of the vision object, this should be done in setupVision.
     */
    virtual std::shared_ptr<IDS::NXT::Vision> factoryVision() override;

    /**
     * @brief Setup of a vision object
     * @param vision Vision object to be set up
     *
     * This method sets up a vision object. This method is called whenever a vision objects needs to be
     * setup, i.e. when a new image should be processed. Inside this method, every variable of the vision
     * object should be set. As the vision object will run in a different thread context, use thread-safe
     * variables, e.g. simple copies.
     */
    virtual void setupVision(std::shared_ptr<IDS::NXT::Vision> vision) override;

    /**
     * @brief Handle the result of a vision object
     * @param vision Finished vision object
     *
     * This method handles a finished vision object and reads the results. These results can then be further
     * processed and/or written to the ResultSourceCollection. After that, the image can be removed from the
     * vision object.
     */
    virtual void handleResult(std::shared_ptr<IDS::NXT::Vision> vision) override;

private slots:
    /**
     * @brief Setter for result image status
     * @param enable Flag to enable/disable result image
     */
    void enableResultImage(bool enable);

private:
    QJsonArray resultToJson(const QList<QPair<QString, double>>& results, double expsum, bool limitResults = false);

    IDS::NXT::ResultSourceCollection& _resultCollection;
    CnnRoiHandler _cnnRoiHandler;
    IDS::NXT::ConfigurableBool _createResultImage;
    std::unique_ptr<MyResultImage> _resultImage;
};
