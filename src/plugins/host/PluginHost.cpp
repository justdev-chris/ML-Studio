#include "PluginHost.h"
#include "vst3/VST3Host.h"
#include "lv2/LV2Host.h"
#include "clap/CLAPHost.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>

PluginHost::PluginHost(QObject* parent)
    : QObject(parent) {
}

PluginHost::~PluginHost() {
    for (auto* instance : m_activeInstances) {
        instance->shutdown();
        delete instance;
    }
    m_activeInstances.clear();
}

void PluginHost::scanPlugins(const QString& path) {
    emit scanStarted(path);

    QDir dir(path);
    if (!dir.exists()) {
        emit error("Plugin path does not exist: " + path);
        return;
    }

    // Scan VST3
    scanVST3(path + "/VST3");

    // Scan LV2
    scanLV2(path + "/LV2");

    // Scan CLAP
    scanCLAP(path + "/CLAP");

#ifdef __APPLE__
    scanAU("/Library/Audio/Plug-Ins/Components");
#endif

    emit scanFinished(m_availablePlugins.size());
}

void PluginHost::scanVST3(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) return;

    QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        if (entry.endsWith(".vst3")) {
            QString fullPath = dir.absoluteFilePath(entry);
            PluginInfo info;
            info.id = entry;
            info.name = entry;
            info.vendor = "Unknown";
            info.path = fullPath;
            info.format = "VST3";
            info.isEffect = true;
            info.numInputs = 2;
            info.numOutputs = 2;
            addPlugin(info);
            emit pluginFound(info);
        }
    }
}

void PluginHost::scanLV2(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) return;

    QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        if (entry.endsWith(".lv2")) {
            QString fullPath = dir.absoluteFilePath(entry);
            PluginInfo info;
            info.id = entry;
            info.name = entry;
            info.vendor = "Unknown";
            info.path = fullPath;
            info.format = "LV2";
            info.isEffect = true;
            info.numInputs = 2;
            info.numOutputs = 2;
            addPlugin(info);
            emit pluginFound(info);
        }
    }
}

void PluginHost::scanCLAP(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) return;

    QStringList entries = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        if (entry.endsWith(".clap")) {
            QString fullPath = dir.absoluteFilePath(entry);
            PluginInfo info;
            info.id = entry;
            info.name = entry;
            info.vendor = "Unknown";
            info.path = fullPath;
            info.format = "CLAP";
            info.isEffect = true;
            info.numInputs = 2;
            info.numOutputs = 2;
            addPlugin(info);
            emit pluginFound(info);
        }
    }
}

void PluginHost::scanAU(const QString& path) {
#ifdef __APPLE__
    QDir dir(path);
    if (!dir.exists()) return;

    QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        if (entry.endsWith(".component")) {
            QString fullPath = dir.absoluteFilePath(entry);
            PluginInfo info;
            info.id = entry;
            info.name = entry;
            info.vendor = "Unknown";
            info.path = fullPath;
            info.format = "AU";
            info.isEffect = true;
            info.numInputs = 2;
            info.numOutputs = 2;
            addPlugin(info);
            emit pluginFound(info);
        }
    }
#endif
}

PluginInfo PluginHost::getPluginInfo(const QString& id) const {
    for (const PluginInfo& info : m_availablePlugins) {
        if (info.id == id) return info;
    }
    return PluginInfo();
}

PluginInstance* PluginHost::createInstance(const QString& id) {
    PluginInfo info = getPluginInfo(id);
    if (info.id.isEmpty()) {
        emit error("Plugin not found: " + id);
        return nullptr;
    }

    PluginInstance* instance = nullptr;

    if (info.format == "VST3") {
        instance = new VST3Host(info);
    } else if (info.format == "LV2") {
        instance = new LV2Host(info);
    } else if (info.format == "CLAP") {
        instance = new CLAPHost(info);
    } else if (info.format == "AU") {
        // AU not implemented yet
        emit error("AU plugins not supported yet");
        return nullptr;
    }

    if (instance) {
        if (instance->initialize(m_sampleRate, m_blockSize)) {
            m_activeInstances.append(instance);
            emit instanceCreated(instance);
            qDebug() << "Plugin instance created:" << info.name;
            return instance;
        } else {
            delete instance;
            emit error("Failed to initialize plugin: " + info.name);
            return nullptr;
        }
    }

    return nullptr;
}

void PluginHost::destroyInstance(PluginInstance* instance) {
    if (!instance) return;
    if (m_activeInstances.removeAll(instance) > 0) {
        instance->shutdown();
        emit instanceDestroyed(instance);
        delete instance;
        qDebug() << "Plugin instance destroyed";
    }
}

QVector<PluginInfo> PluginHost::searchPlugins(const QString& query) const {
    QVector<PluginInfo> results;
    if (query.trimmed().isEmpty()) return m_availablePlugins;

    for (const PluginInfo& info : m_availablePlugins) {
        if (info.name.contains(query, Qt::CaseInsensitive) ||
            info.vendor.contains(query, Qt::CaseInsensitive)) {
            results.append(info);
        }
    }
    return results;
}

QVector<PluginInfo> PluginHost::getPluginsByCategory(const QString& category) const {
    QVector<PluginInfo> results;
    for (const PluginInfo& info : m_availablePlugins) {
        if (info.category == category) results.append(info);
    }
    return results;
}

QVector<PluginInfo> PluginHost::getSynths() const {
    QVector<PluginInfo> results;
    for (const PluginInfo& info : m_availablePlugins) {
        if (info.isSynth) results.append(info);
    }
    return results;
}

QVector<PluginInfo> PluginHost::getEffects() const {
    QVector<PluginInfo> results;
    for (const PluginInfo& info : m_availablePlugins) {
        if (info.isEffect && !info.isSynth) results.append(info);
    }
    return results;
}

void PluginHost::addPlugin(const PluginInfo& info) {
    for (const PluginInfo& existing : m_availablePlugins) {
        if (existing.id == info.id && existing.format == info.format) return;
    }
    m_availablePlugins.append(info);
}

void PluginHost::setSampleRate(double sampleRate) {
    m_sampleRate = sampleRate;
    for (auto* instance : m_activeInstances) {
        instance->setSampleRate(sampleRate);
    }
}

void PluginHost::setBlockSize(int blockSize) {
    m_blockSize = blockSize;
    for (auto* instance : m_activeInstances) {
        instance->setBlockSize(blockSize);
    }
}
