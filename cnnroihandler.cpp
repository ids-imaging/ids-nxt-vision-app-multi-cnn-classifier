#include "cnnroihandler.h"

#include <QLoggingCategory>
#include <cmath>

#include <frameworkapplication.h>

using namespace IDS::NXT;
using namespace IDS::NXT::CNNv2;

static QLoggingCategory lc{"multicnnclassifier.cnnroihandler"};

CnnRoiHandler::CnnRoiHandler()
  : _cnnRoiConfigFile{"cnnconfig", false, true, "json"}
  , _cnnFile{"cnnfile", false, true, "cnn"}
  , _cnnInstallationRunning{false} {
    // Init members
    updateTotalCnnMemory();
    _cnnRoiConfigFile.setZIndex(0);
    _cnnFile.setZIndex(1);
    _roiManager.setMaxROIs(CnnRoiConfig::getMaxRois());
    _cnnRoiConfigFile.setFilter({"Json |*.json"});
    _cnnRoiConfigFile.setDeletable(true);
    QList<TranslatedText> filtercnn;
    filtercnn.append("IDS NXT rio/rome classification model |*.rcla; *.cnn");
    _cnnFile.setFilter(filtercnn);
    _cnnFile.setDeletable(true);
    // Connect framework signals to local slot
    connect(&CnnManager::getInstance(), &CnnManager::cnnChanged, this, &CnnRoiHandler::cnnChanged);
    connect(&CnnManager::getInstance(), &CnnManager::installedCnnsChanged, this, &CnnRoiHandler::installedCnnsChanged);
    connect(&_roiManager, &IDS::NXT::ROIManager::listOfManagedRoisChanged, this, &CnnRoiHandler::roiChanged);
    connect(&_roiManager, &IDS::NXT::ROIManager::managedRoiChanged, this, &CnnRoiHandler::roiChanged);
    connect(&_cnnRoiConfigFile, &ConfigurableFile::written, this, &CnnRoiHandler::loadRoiCnnConfig);
    connect(&_cnnRoiConfigFile, &ConfigurableFile::deleted, this, &CnnRoiHandler::deleteCnnConfig);
    connect(&_cnnFile, &ConfigurableFile::deleted, this, &CnnRoiHandler::deleteAllCNNs);
    connect(&_cnnFile, &ConfigurableFile::written, this, &CnnRoiHandler::installCnn);

    try {
        loadRoiCnnConfig();
    } catch (const std::runtime_error& e) {
        qCDebug(lc) << "can not load roiconfig" << e.what();
    }
}

void CnnRoiHandler::cnnChanged() {
    qCDebug(lc) << "cnnChanged";
    roiOrCnnOrConfigChanged();
}

void CnnRoiHandler::roiChanged() {
    qCDebug(lc) << "roiChanged";
    roiOrCnnOrConfigChanged();
}

void CnnRoiHandler::roiOrCnnOrConfigChanged() {
    qCDebug(lc) << "roiOrCnnOrConfigChanged";
    std::lock_guard<std::mutex> locker(_updateLock);

    // disable all signals temporary to prevent multiple function calls on every change
    QSignalBlocker blockerRoiManager(&_roiManager);
    QSignalBlocker blockerCnnManager(&CnnManager::getInstance());

    qCDebug(lc) << "roiOrCnnOrConfigChanged run";
    MyVision::RoiCnnList newList;
    QList<CnnData> activeCnns;
    try {
        activeCnns = CnnManager::getInstance().activeCnns();
    } catch (std::runtime_error& e) {
        qCDebug(lc) << "Error getting activeCnns:" << e.what();
    }
    QStringList installedCnns;
    try {
        installedCnns = CnnManager::getInstance().availableCnns();
    } catch (std::runtime_error& e) {
        qCDebug(lc) << "Error getting availableCnns" << e.what();
    }
    const auto loadedRoiCnns = _cnnRoiConfig.getCnnRois();
    const auto activeRois = _roiManager.managedROIs().keys();
    QStringList loadedRois;

    bool skipInit = false;
    for (const auto& loadedRoiCnn : loadedRoiCnns) {
        if (!installedCnns.contains(loadedRoiCnn.cnn())) {
            qCDebug(lc) << loadedRoiCnn.cnn() << "not installed. Skip init";
            skipInit = true;
        }
        loadedRois.append(loadedRoiCnn.roiName());
    }
    // rois of config already defined
    if (loadedRois != activeRois && !loadedRoiCnns.isEmpty()) {
        qCDebug(lc) << "Rois not active, activate";
        _roiManager.clearROIs();
        // only set rois here, because rois are defined in config
        for (const auto& loadedRoiCnn : loadedRoiCnns) {
            const auto rect = loadedRoiCnn.roiRect();
            _roiManager.addROI(loadedRoiCnn.roiName(), rect.x(), rect.y(), rect.width(), rect.height());
        }
    }

    if (skipInit) {
        updateInstalledCnnDescription();
        return;
    }

    // check CNNs
    QStringList activeCnnList;
    for (const auto& cnn : qAsConst(activeCnns)) {
        activeCnnList.append(cnn.name());
    }
    auto cnnsForDeactivation = installedCnns;
    QStringList cnnsForActivation;
    for (const auto& loadedRoi : loadedRoiCnns) {
        const auto cnn = loadedRoi.cnn();
        if (!installedCnns.contains(cnn)) {
            qCDebug(lc) << "CNN not installed" << cnn;
            continue;
        }
        cnnsForDeactivation.removeAll(cnn);
        if (!activeCnnList.contains(cnn)) {
            qCDebug(lc) << "CNN not active. Enable it." << cnn;
            cnnsForActivation.append(cnn);
        }
    }
    qCDebug(lc) << "CNN to deactivate" << cnnsForDeactivation;
    QStringList cnnsForDeactivationList;
    // deactivate not used cnns
    for (const auto& cnn : cnnsForDeactivation) {
        qCDebug(lc) << "Disable not used cnn" << cnn;
        if (activeCnnList.contains(cnn)) {
            cnnsForDeactivationList.append(cnn);
        } else {
            qCDebug(lc) << "not active";
        }
    }
    if (!cnnsForDeactivationList.isEmpty()) {
        CnnManager::getInstance().enableCnns(cnnsForDeactivationList, false);
    }
    qint64 neededCnnMemory = 0;
    for (const auto& cnn : cnnsForActivation) {
        neededCnnMemory += CnnManager::getInstance().neededCnnMemory(cnn) / 1024 / 1024;
    }
    if (neededCnnMemory > (_totalCnnMemory)) {
        qCritical(lc) << "can not activate CnnRoiConfig. CNNs need too much memory";
        updateInstalledCnnDescription();
        return;
    }
    try {
        if (!cnnsForActivation.isEmpty())
            CnnManager::getInstance().enableCnns(cnnsForActivation, true);
    } catch (std::runtime_error& e) {
        qCritical(lc) << "Error enabling cnns:" << e.what();
        return;
    }
    auto getCnnData = [](const QList<CnnData>& allCNNs, const QString& cnn) {
        for (const auto& oneCNN : allCNNs) {
            if (cnn == oneCNN.name()) {
                return oneCNN;
            }
        }
        return CnnData{};
    };

    for (const auto& cnnRoi : loadedRoiCnns) {
        // create Vision Data
        MyVision::RoiCnn thisRoiCNN;
        thisRoiCNN.cnnData = getCnnData(activeCnns, cnnRoi.cnn());
        thisRoiCNN.roiName = cnnRoi.roiName();
        const auto managedRois = _roiManager.managedROIs();
        if (managedRois.contains(thisRoiCNN.roiName)) {
            thisRoiCNN.roi = managedRois.value(thisRoiCNN.roiName)->getQRect();
        } else {
            thisRoiCNN.roi = cnnRoi.roiRect();
        }
        newList.append(thisRoiCNN);
    }

    _activeRoiCnnList.clear();
    _activeRoiCnnList = newList;

    updateInstalledCnnDescription();
}

void CnnRoiHandler::updateTotalCnnMemory() {
    _totalCnnMemory = CnnManager::getInstance().availableCnnMemory() / 1024 / 1024;
}

void CnnRoiHandler::loadRoiCnnConfig() {
    const auto jsonConfigFile = _cnnRoiConfigFile.absoluteFilePath();
    const auto jsonConfigFileBackup = jsonConfigFile + "_bak";

    try {
        _cnnRoiConfig.loadJsonFile(jsonConfigFile);
        roiOrCnnOrConfigChanged();

        static TranslatedText newDescription{
            QMap<QString, QString>{{QStringLiteral("en"), QStringLiteral("Config loaded succesfully.")},
                                   {QStringLiteral("de"), QStringLiteral("Config erfolgreich geladen.")}}};
        _cnnRoiConfigFile.setDescription(newDescription);
    } catch (const std::runtime_error& e) {
        qCDebug(lc) << "Error loading RoiCnnConfig" << e.what();

        QMap<QString, QString> translations;
        translations.insert(QStringLiteral("en"), QStringLiteral("Error loading config:\n\r%1").arg(e.what()));
        translations.insert(QStringLiteral("de"), QStringLiteral("Fehler beim Laden der Config:\n\r%1").arg(e.what()));

        TranslatedText newDescription(translations);
        _cnnRoiConfigFile.setDescription(newDescription);

        // restore old file
        QFile::copy(jsonConfigFileBackup, jsonConfigFile);
        _cnnRoiConfig.loadJsonFile(jsonConfigFileBackup);
        throw;
    }

    QFile::copy(jsonConfigFile, jsonConfigFileBackup);
}

void CnnRoiHandler::deleteCnnConfig() {
    const auto jsonConfigFile = _cnnRoiConfigFile.absoluteFilePath();
    const auto jsonConfigFileBackup = jsonConfigFile + "_bak";

    QFile::remove(jsonConfigFile);
    QFile::remove(jsonConfigFileBackup);

    try {
        loadRoiCnnConfig();
    } catch (const std::runtime_error& e) {
        qCDebug(lc) << "can not load roiconfig" << e.what();
    }
    _roiManager.clearROIs();
    _activeRoiCnnList.clear();
    updateInstalledCnnDescription();
}

void CnnRoiHandler::deleteAllCNNs() {
    _activeRoiCnnList.clear();
    {
        // disable all signals temporary to prevent multiple function calls on every change
        QSignalBlocker blockerRoiManager(&_roiManager);
        QSignalBlocker blockerCnnManager(&CnnManager::getInstance());

        const auto installedCnns = CnnManager::getInstance().availableCnns();

        for (const auto& cnn : installedCnns) {
            CnnManager::getInstance().removeCnn(cnn);
        }
    }

    updateInstalledCnnDescription();
}

MyVision::RoiCnnList CnnRoiHandler::activeRoiCnnList() const {
    return _activeRoiCnnList;
}

void CnnRoiHandler::installedCnnsChanged() {
    qCDebug(lc) << "installedCnnsChanged";

    roiOrCnnOrConfigChanged();
    updateInstalledCnnDescription();
}

void CnnRoiHandler::installCnn() {
    // Prevent installation of new cnn during installation of another
    if (!_cnnInstallationRunning) {
        _cnnInstallationRunning.store(true);
        try {
            CnnManager::getInstance().addCnn(_cnnFile.absoluteFilePath());
            _cnnInstallationRunning.store(false);
        } catch (std::runtime_error& e) {
            _cnnInstallationRunning.store(false);
            throw;
        }
    } else {
        // This message is written to the REST call as responce
        throw std::runtime_error("Can not install CNN. Installation still in progress");
    }

    updateInstalledCnnDescription();
}

void CnnRoiHandler::updateInstalledCnnDescription() {
    qCDebug(lc) << "updateInstalledCnnDescription";
    const auto installedCNNs = CnnManager::getInstance().availableCnns();
    const auto cnnRoiConfig = _cnnRoiConfig.getCnnRois();
    auto managedRois = _roiManager.managedROIs().keys();

    auto isActive = [&](const QString& cnn) -> bool {
        for (const auto& cnnRoi : qAsConst(_activeRoiCnnList)) {
            if (cnn == cnnRoi.cnnData.name()) {
                return true;
            }
        }

        return false;
    };

    auto getNeededMemoryOfCNN = [&](const QString& cnn) -> float {
        return static_cast<float>(CnnManager::getInstance().neededCnnMemory(cnn)) / 1024 / 1024;
    };

    CnnRoiTextCreator text;
    auto notUsedCnns = installedCNNs;
    qint64 neededCnnMemory = 0;

    for (const auto& cnn : cnnRoiConfig) {
        auto roiText = cnn.roiName();
        auto cnnText = cnn.cnn();

        if (isActive(cnn.cnn())) {
            roiText = QStringLiteral("[ %1 ]✔").arg(roiText);
        } else {
            roiText = QStringLiteral("[ %1 ]✖").arg(roiText);
        }

        if (installedCNNs.contains(cnn.cnn())) {
            const auto neededCnnMemoryInMByte = getNeededMemoryOfCNN(cnn.cnn());
            neededCnnMemory += static_cast<quint64>(std::ceil(neededCnnMemoryInMByte));
            qCDebug(lc) << "neededCnnMemoryInMByte" << cnn.cnn() << neededCnnMemoryInMByte;
            cnnText.append(QStringLiteral(" (%1MB)").arg(std::ceil(neededCnnMemoryInMByte), 0, 'f', 0));
        } else {
            cnnText.append(" ( - )");
        }
        text.addRow(cnnText, roiText);
        // create list of installed but not used cnns and rois
        notUsedCnns.removeAll(cnn.cnn());
        managedRois.removeAll(cnn.roiName());
    }

    // add not used cnns to list
    for (const auto& cnn : notUsedCnns) {
        const auto neededCnnMemoryInMByte = getNeededMemoryOfCNN(cnn);
        qCDebug(lc) << "neededCnnMemoryInMByte" << cnn << neededCnnMemoryInMByte;
        text.addRow(QStringLiteral("%1 (%2MB)").arg(cnn, QString::number(std::ceil(neededCnnMemoryInMByte), 'f', 0)),
                    QStringLiteral("[ - ]✖"));
    }

    // add not used rois to list
    for (const auto& roi : qAsConst(managedRois)) {
        text.addRow(QStringLiteral(" ( - )"), QStringLiteral("[ %1 ]✖").arg(roi));
    }

    const auto description = FrameworkApplication::manifest().getTranslatedText(
        QStringLiteral("Language.cnnfile.Description"));

    QString newDescriptionEN = description.translation(QStringLiteral("en"));
    QString newDescriptionDE = description.translation(QStringLiteral("de"));

    newDescriptionEN.append(text);
    newDescriptionDE.append(text);

    const auto freeMemory = _totalCnnMemory - neededCnnMemory;

    newDescriptionEN.append(QStringLiteral("-------------------------------------------------------------------\n\r"));
    newDescriptionEN.append(QStringLiteral("Available CNN Memory: %1MB / %2MB").arg(freeMemory).arg(_totalCnnMemory));
    newDescriptionDE.append(QStringLiteral("-------------------------------------------------------------------\n\r"));
    newDescriptionDE.append(
        QStringLiteral("Verfügbarer CNN Speicher: %1MB / %2MB").arg(freeMemory).arg(_totalCnnMemory)); //

    QMap<QString, QString> translations;
    translations.insert(QStringLiteral("en"), newDescriptionEN);
    translations.insert(QStringLiteral("de"), newDescriptionDE);
    TranslatedText newDescription(translations);
    _cnnFile.setDescription(newDescription);
}

void CnnRoiTextCreator::addRow(const QString& cnn, const QString& roi) {
    _text.append(QStringLiteral("%1 %2\n\r").arg(cnn, roi));
}

QString CnnRoiTextCreator::getText() const {
    return _text;
}

CnnRoiTextCreator::operator QString() const {
    return getText();
}
