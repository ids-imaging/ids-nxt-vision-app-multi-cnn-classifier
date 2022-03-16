#pragma once
#include "cnnroiconfig.h"

#include <QObject>
#include <mutex>

#include <cnnmanager_v2.h>
#include <configurablefile.h>
#include <myvision.h>
#include <roimanager.h>

/**
 * @brief This class is used to create the description of the set configuration
 * which is displayed in the NXT Cockpit
 */
class CnnRoiTextCreator {
public:
    CnnRoiTextCreator() = default;

    void addRow(const QString& cnn, const QString& roi);
    QString getText() const;
    operator QString() const;

private:
    QString _text;
};

/**
 * @brief This class handles the current ROI/CNN configuration
 */
class CnnRoiHandler : public QObject {
    Q_OBJECT

public:
    CnnRoiHandler();

    MyVision::RoiCnnList activeRoiCnnList() const;

private slots:
    void cnnChanged();
    void roiChanged();
    void installedCnnsChanged();
    void installCnn();
    void loadRoiCnnConfig();
    void deleteCnnConfig();

private:
    void roiOrCnnOrConfigChanged();
    void updateTotalCnnMemory();
    void updateInstalledCnnDescription();
    void deleteAllCNNs();

    IDS::NXT::ROIManager _roiManager;
    qint64 _totalCnnMemory = 0;
    IDS::NXT::ConfigurableFile _cnnRoiConfigFile;
    IDS::NXT::ConfigurableFile _cnnFile;
    std::atomic_bool _cnnInstallationRunning;
    QStringList _installedCNNs;
    CnnRoiConfig _cnnRoiConfig;
    MyVision::RoiCnnList _activeRoiCnnList;
    std::mutex _updateLock;
};
