#include "cnnroiconfig.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QRegularExpression>

static constexpr auto CONFIG_MAX_ROIS = 20;
static constexpr auto CONFIG_TAG_CNN = "Cnn";
static constexpr auto CONFIG_TAG_ROINAME = "RoiName";
static constexpr auto CONFIG_TAG_OFFSETX = "OffsetX";
static constexpr auto CONFIG_TAG_OFFSETY = "OffsetY";
static constexpr auto CONFIG_TAG_HEIGHT = "Height";
static constexpr auto CONFIG_TAG_WIDTH = "Width";
static const auto CONFIG_TAGS = QStringList{CONFIG_TAG_CNN,
                                            CONFIG_TAG_ROINAME,
                                            CONFIG_TAG_OFFSETX,
                                            CONFIG_TAG_OFFSETY,
                                            CONFIG_TAG_HEIGHT,
                                            CONFIG_TAG_WIDTH};

static QLoggingCategory lc{"multicnnclassifier.cnnroiconfig"};

void CnnRoiConfig::loadJsonFile(const QString& file) {
    qCDebug(lc) << "open File" << file;

    try {
        _roiConfigFile = file;
        QFile roiFile(_roiConfigFile);
        if (!roiFile.exists()) {
            qCDebug(lc) << "Error loading roi File: File does not exist";
            throw(std::runtime_error("File does not exist"));
        }
        if (!roiFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qCritical(lc) << "Error loading roi File: Can not open file";
            throw(std::runtime_error("Can not open file"));
        }
        const auto data = roiFile.readAll();
        roiFile.close();

        QJsonParseError err{};
        const auto loadedJson = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError) {
            qCritical(lc) << "Error loading config:" << err.errorString();
            throw(std::runtime_error(err.errorString().toStdString()));
        }

        const auto roiConfigs = loadedJson.array();
        qCDebug(lc) << "loaded ROI config:" << roiConfigs;
        if (roiConfigs.size() > CONFIG_MAX_ROIS) {
            const auto error = QStringLiteral("Error loading config. maximum number is %1").arg(CONFIG_MAX_ROIS);
            qCritical(lc) << error;
            throw(std::runtime_error(error.toStdString()));
        }

        QVariantMap loadedCnnRois;
        for (const auto& roiConfig : roiConfigs) {
            const auto map = roiConfig.toObject().toVariantMap();
            const auto name = map[CONFIG_TAG_ROINAME].toString();
            loadedCnnRois[name] = map;
        }

        _cnnRois = loadedCnnRois;
    } catch (...) {
        _cnnRois.clear();
        _roiConfigFile.clear();
        throw;
    }
}

QStringList CnnRoiConfig::getNeededCNNs() const {
    QStringList cnns;

    for (auto keyIter = _cnnRois.keyBegin(); keyIter != _cnnRois.keyEnd(); ++keyIter) {
        cnns.append(*keyIter);
    }

    return cnns;
}

void CnnRoiConfig::saveConfig() {
    QJsonArray config;

    for (auto iter = _cnnRois.begin(); iter != _cnnRois.end(); ++iter) {
        QVariantMap map = iter.value().toMap();
        map[CONFIG_TAG_ROINAME] = iter.key();
        config.append(QJsonObject::fromVariantMap(map));
    }

    QFile file(_roiConfigFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
}

QList<CnnRoiConfig::CnnRoiMap> CnnRoiConfig::getCnnRois() const {
    QList<CnnRoiConfig::CnnRoiMap> output;

    for (auto iter = _cnnRois.begin(); iter != _cnnRois.end(); ++iter) {
        output.append(CnnRoiConfig::CnnRoiMap(iter.value().toMap()));
    }

    return output;
}

CnnRoiConfig::CnnRoiMap CnnRoiConfig::getCnnRoi(const QString& cnn) {
    return CnnRoiConfig::CnnRoiMap(_cnnRois[cnn].toMap());
}

int CnnRoiConfig::getMaxRois() {
    return CONFIG_MAX_ROIS;
}

CnnRoiConfig::CnnRoiMap::CnnRoiMap(const QString& roiName, const QString& cnn, QRect rect)
  : _roiName{roiName}
  , _roiRect{rect}
  , _cnn{cnn} {}

CnnRoiConfig::CnnRoiMap::CnnRoiMap(const QVariantMap& map) {
    loadMap(map);
}

void CnnRoiConfig::CnnRoiMap::loadMap(const QVariantMap& map) {
    auto checkKeys = [](const QVariantMap& map) {
        const auto mapKeys = map.keys();
        for (const auto& neededKey : CONFIG_TAGS) {
            if (!mapKeys.contains(neededKey)) {
                throw std::runtime_error(neededKey.toStdString() + " not in map");
            }
        }
    };
    checkKeys(map);

    auto checkName = [](const QString& name) {
        // check naming
        QRegularExpression regex{QStringLiteral("[a-z]+[\\_a-z0-9-]*")};
        auto match = regex.match(name);
        if (match.hasMatch()) {
            const auto capture = match.captured();
            if (capture == name) {
                return;
            }
        }
        throw std::runtime_error{"\'" + name.toStdString()
                                 + "\' contains an unsupported char. "
                                   "Use lower case chars, numbers and _"};
    };
    const auto name = map.value(CONFIG_TAG_ROINAME).toString();
    checkName(name);

    auto checkRoiParameter = [&map](const QString& key) {
        bool ok = true;
        map[key].toInt(&ok);
        if (!ok) {
            throw std::runtime_error("Rect not valid");
        }
    };
    for (const auto& param : QStringList{CONFIG_TAG_OFFSETX, CONFIG_TAG_OFFSETY, CONFIG_TAG_WIDTH, CONFIG_TAG_HEIGHT}) {
        checkRoiParameter(param);
    }
    _roiName = name;
    _cnn = map.value(CONFIG_TAG_CNN).toString();
    _roiRect = QRect(map[CONFIG_TAG_OFFSETX].toInt(),
                     map[CONFIG_TAG_OFFSETY].toInt(),
                     map[CONFIG_TAG_WIDTH].toInt(),
                     map[CONFIG_TAG_HEIGHT].toInt());
}

QVariantMap CnnRoiConfig::CnnRoiMap::toMap() const {
    QVariantMap thisCnn;
    thisCnn[CONFIG_TAG_CNN] = _cnn;
    thisCnn[CONFIG_TAG_ROINAME] = _roiName;
    thisCnn[CONFIG_TAG_OFFSETX] = _roiRect.x();
    thisCnn[CONFIG_TAG_OFFSETY] = _roiRect.y();
    thisCnn[CONFIG_TAG_HEIGHT] = _roiRect.height();
    thisCnn[CONFIG_TAG_WIDTH] = _roiRect.width();

    return thisCnn;
}

void CnnRoiConfig::CnnRoiMap::setCnn(const QString& cnn) {
    _cnn = cnn;
}

void CnnRoiConfig::CnnRoiMap::setRoiRect(QRect rect) {
    _roiRect = rect;
}

void CnnRoiConfig::CnnRoiMap::setRoiName(const QString& roiName) {
    _roiName = roiName;
}

QString CnnRoiConfig::CnnRoiMap::cnn() const {
    return _cnn;
}

QRect CnnRoiConfig::CnnRoiMap::roiRect() const {
    return _roiRect;
}

QString CnnRoiConfig::CnnRoiMap::roiName() const {
    return _roiName;
}
