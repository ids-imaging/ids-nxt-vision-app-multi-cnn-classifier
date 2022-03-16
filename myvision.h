#ifndef MYVISION_H
#define MYVISION_H

#include "cnnmanager_v2.h"
#include <configurableroi.h>
#include <vision.h>

#include <QImage>

/**
 * @brief The app-specific vision object
 */
class MyVision : public IDS::NXT::Vision {
    Q_OBJECT

public:
    /**
     * @brief Helper class for the assignment of a CNN to a ROI
     */
    class RoiCnn {
    public:
        QRect roi;
        QString roiName;
        IDS::NXT::CNNv2::CnnData cnnData;

        bool operator<(const RoiCnn& rhs) const {
            return roiName < rhs.roiName;
        }
    };

    using RoiCnnList = QList<RoiCnn>;
    using RoiCnnResultMap = std::map<RoiCnn, std::unique_ptr<IDS::NXT::CNNv2::MultiBuffer>>;

    /**
     * @brief Constructor
     */
    MyVision() = default;

    /**
     * @brief Start the image processing
     */
    void process() override;
    /**
     * @brief Abort the image processing
     */
    void abort() override;

    /**
     * @brief Getter for the vision result
     * @return Result
     */
    RoiCnnResultMap result();

    /**
     * @brief Setter for ROI/CNN configuration
     * @param roiCnnConfig Configuration object
     */
    void setCnnData(const RoiCnnList& roiCnnConfig);

private:
    RoiCnnResultMap _result;
    RoiCnnList _cnnData;
};

#endif // MYVISION_H
