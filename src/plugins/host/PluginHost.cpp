#include "PluginHost.h"
#include "vst3/VST3Host.h"
#include "lv2/LV2Host.h"
#include "clap/CLAPHost.h"
#include <QDir>
#include <QSettings>

PluginHost::PluginHost(QObject* parent) : QObject(parent) {}
PluginHost::~PluginHost() {
    for (auto* instance : m_activeInstances) { instance->shutdown(); delete instance; }
    m_activeInstances.clear();
}

void PluginHost::scanPlugins(const QString& path) {
    emit scanStarted(path);
    scanVST3(path + "/VST3");
    scanLV2(path + "/LV2");
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
            PluginInfo info;
            info.id = entry; info.name = entry; info.vendor = "Unknown";
            info.path = dir.absoluteFilePath(entry); info.format = "VST3";
            info.isEffect = true; info.numInputs = 2; info.numOutputs = 2;
            addPlugin(info);
            emit pluginFound(info);
            emit scanProgress(0, "Found: " + entry);
        }
    }
}

void PluginHost::scanLV2(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) return;
    QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        if (entry.endsWith(".lv2")) {
            PluginInfo info;
            info.id = entry; info.name = entry; info.vendor = "Unknown";
            info.path = dir.absoluteFilePath(entry); info.format = "LV2";
            info.isEffect = true; info.numInputs = 2; info.numOutputs = 2;
            addPlugin(info);
            emit pluginFound(info);
            emit scanProgress(0, "Found: " + entry);
        }
    }
}

void PluginHost::scanCLAP(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) return;
    QStringList entries = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        if (entry.endsWith(".clap")) {
            PluginInfo info;
            info.id = entry; info.name = entry; info.vendor = "Unknown";
            info.path = dir.absoluteFilePath(entry); info.format = "CLAP";
            info.isEffect = true; info.numInputs = 2; info.numOutputs = 2;
            addPlugin(info);
            emit pluginFound(info);
            emit scanProgress(0, "Found: " + entry);
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
            PluginInfo info;
            info.id = entry; info.name = entry; info.vendor = "Unknown";
            info.path = dir.absoluteFilePath(entry); info.format = "AU";
            info.isEffect = true; info.numInputs = 2; info.numOutputs = 2;
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
    QMutexLocker locker(&m_mutex);
    PluginInfo info = getPluginInfo(id);
    if (info.id.isEmpty()) { emit error("Plugin not found: " + id); return nullptr; }

    PluginInstance* instance = nullptr;
    if (info.format == "VST3") instance = new VST3Host(info);
    else if (info.format == "LV2") instance = new LV2Host(info);
    else if (info.format == "CLAP") instance = new CLAPHost(info);

    if (instance) {
        if (instance->initialize(m_sampleRate, m_blockSize)) {
            m_activeInstances.append(instance);
            emit instanceCreated(instance);
            return instance;
        }
        delete instance;
        emit error("Failed to initialize: " + info.name);
    }
    return nullptr;
}

void PluginHost::destroyInstance(PluginInstance* instance) {
    if (!instance) return;
    QMutexLocker locker(&m_mutex);
    if (m_activeInstances.removeAll(instance) > 0) {
        instance->shutdown();
        emit instanceDestroyed(instance);
        delete instance;
    }
}

void PluginHost::setSampleRate(double sampleRate) {
    m_sampleRate = sampleRate;
    for (auto* instance : m_activeInstances) instance->setSampleRate(sampleRate);
}

void PluginHost::setBlockSize(int blockSize) {
    m_blockSize = blockSize;
    for (auto* instance : m_activeInstances) instance->setBlockSize(blockSize);
}

void PluginHost::addPlugin(const PluginInfo& info) {
    for (const PluginInfo& existing : m_availablePlugins) {
        if (existing.id == info.id && existing.format == info.format) return;
    }
    m_availablePlugins.append(info);
}
