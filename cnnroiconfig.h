#pragma once

#include <QRect>
#include <QVariantMap>

/**
 * @brief This class handles the mapping between rois and cnns
 */
class CnnRoiConfig {
public:
    class CnnRoiMap {
    public:
        /**
         * @brief C'tor
         * @param roiName Name of the ROI
         * @param cnn Name of the CNN
         * @param rect ROI coordinates
         */
        CnnRoiMap(const QString& roiName, const QString& cnn, QRect rect);

        /**
         * @brief C'tor
         * @param map CNN/ROI Map
         */
        CnnRoiMap(const QVariantMap& map);

        QVariantMap toMap() const;
        void loadMap(const QVariantMap& map);
        QString roiName() const;
        QRect roiRect() const;
        QString cnn() const;
        void setRoiName(const QString& roiName);
        void setRoiRect(QRect rect);
        void setCnn(const QString& cnn);

    private:
        QString _roiName;
        QRect _roiRect;
        QString _cnn;
    };

    CnnRoiConfig() = default;

    /**
     * @brief Loads a JSON ROI/CNN configuration file
     * @param Configuration file
     */
    void loadJsonFile(const QString& file);

    /**
     * @brief Getter for the needed CNNs for the given configuration
     * @return Needed CNNs
     */
    QStringList getNeededCNNs() const;

    void saveConfig();
    QList<CnnRoiMap> getCnnRois() const;
    CnnRoiMap getCnnRoi(const QString& cnn);

    /**
     * @brief Getter for maximum supported ROIs
     * @return Number of maximum supported ROIs
     */
    static int getMaxRois();

private:
    void parseConfig(const QJsonObject& config);
    QString _roiConfigFile;
    QVariantMap _cnnRois;
};
