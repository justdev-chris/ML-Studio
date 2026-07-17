#include "PluginHost.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QCoreApplication>

// VST3 includes (placeholder - you'd include the actual VST3 SDK)
// #include <pluginterfaces/vst2.x/aeffectx.h>
// #include <pluginterfaces/vst3/ivstcomponent.h>

PluginHost::PluginHost(QObject* parent)
    : QObject(parent) {
    // Initialize plugin paths
    // Windows: C:/Program Files/Common Files/VST3
    // macOS: /Library/Audio/Plug-Ins/VST3
    // Linux: /usr/lib/vst3
}

PluginHost::~PluginHost() {
    // Clean up all active instances
    for (auto* instance : m_activeInstances) {
        instance->shutdown();
        delete instance;
    }
    m_activeInstances.clear();
}

void PluginHost::scanPlugins(const QString& path) {
    emit scanStarted(path);

    // Scan VST3
    scanVST3(path + "/VST3");

    // Scan LV2
    scanLV2(path + "/LV2");

    // Scan CLAP
    scanCLAP(path + "/CLAP");

    // Scan AU (macOS only)
#ifdef __APPLE__
    scanAU("/Library/Audio/Plug-Ins/Components");
#endif

    emit scanFinished(m_availablePlugins.size());
}

void PluginHost::scanVST3(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) return;

    QStringList filters = {"*.vst3"};
    QStringList directories = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    QStringList files = dir.entryList(filters);

    // Handle both .vst3 files and folders
    QStringList entries = directories + files;
    for (const QString& entry : entries) {
        QString fullPath = dir.absoluteFilePath(entry);

        // Check if it's a valid plugin
        // TODO: Actually load and verify plugin

        PluginInfo info;
        info.id = entry;
        info.name = entry;
        info.vendor = "Unknown";
        info.path = fullPath;
        info.format = "VST3";
        info.isSynth = false;
        info.isEffect = true;
        info.numInputs = 2;
        info.numOutputs = 2;

        addPlugin(info);
        emit pluginFound(info);
    }
}

void PluginHost::scanLV2(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) return;

    // LV2 plugins are directories ending with .lv2
    QStringList directories = dir.entryList({"*.lv2"}, QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString& entry : directories) {
        QString fullPath = dir.absoluteFilePath(entry);

        // TODO: Parse manifest.ttl for plugin info

        PluginInfo info;
        info.id = entry;
        info.name = entry;
        info.vendor = "Unknown";
        info.path = fullPath;
        info.format = "LV2";
        info.isSynth = false;
        info.isEffect = true;
        info.numInputs = 2;
        info.numOutputs = 2;

        addPlugin(info);
        emit pluginFound(info);
    }
}

void PluginHost::scanCLAP(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) return;

    QStringList filters = {"*.clap"};
    QStringList files = dir.entryList(filters);

    for (const QString& file : files) {
        QString fullPath = dir.absoluteFilePath(file);

        // TODO: Load CLAP plugin and query info

        PluginInfo info;
        info.id = file;
        info.name = file;
        info.vendor = "Unknown";
        info.path = fullPath;
        info.format = "CLAP";
        info.isSynth = false;
        info.isEffect = true;
        info.numInputs = 2;
        info.numOutputs = 2;

        addPlugin(info);
        emit pluginFound(info);
    }
}

void PluginHost::scanAU(const QString& path) {
#ifdef __APPLE__
    // AudioUnit components are .component bundles
    QDir dir(path);
    if (!dir.exists()) return;

    QStringList components = dir.entryList({"*.component"}, QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString& component : components) {
        QString fullPath = dir.absoluteFilePath(component);

        // TODO: Load AU and query info

        PluginInfo info;
        info.id = component;
        info.name = component;
        info.vendor = "Unknown";
        info.path = fullPath;
        info.format = "AU";
        info.isSynth = false;
        info.isEffect = true;
        info.numInputs = 2;
        info.numOutputs = 2;

        addPlugin(info);
        emit pluginFound(info);
    }
#endif
}

PluginInfo PluginHost::getPluginInfo(const QString& id) const {
    for (const PluginInfo& info : m_availablePlugins) {
        if (info.id == id) {
            return info;
        }
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
        instance = createVST3(info);
    } else if (info.format == "LV2") {
        instance = createLV2(info);
    } else if (info.format == "CLAP") {
        instance = createCLAP(info);
    } else if (info.format == "AU") {
        instance = createAU(info);
    }

    if (instance) {
        m_activeInstances.append(instance);
        emit instanceCreated(instance);
    }

    return instance;
}

void PluginHost::destroyInstance(PluginInstance* instance) {
    if (!instance) return;
    if (m_activeInstances.removeAll(instance) > 0) {
        instance->shutdown();
        emit instanceDestroyed(instance);
        delete instance;
    }
}

QVector<PluginInfo> PluginHost::searchPlugins(const QString& query) const {
    QVector<PluginInfo> results;
    if (query.trimmed().isEmpty()) return m_availablePlugins;

    for (const PluginInfo& info : m_availablePlugins) {
        if (info.name.contains(query, Qt::CaseInsensitive) ||
            info.vendor.contains(query, Qt::CaseInsensitive) ||
            info.id.contains(query, Qt::CaseInsensitive)) {
            results.append(info);
        }
    }
    return results;
}

QVector<PluginInfo> PluginHost::getPluginsByCategory(const QString& category) const {
    QVector<PluginInfo> results;
    for (const PluginInfo& info : m_availablePlugins) {
        if (info.category == category) {
            results.append(info);
        }
    }
    return results;
}

QVector<PluginInfo> PluginHost::getSynths() const {
    QVector<PluginInfo> results;
    for (const PluginInfo& info : m_availablePlugins) {
        if (info.isSynth) {
            results.append(info);
        }
    }
    return results;
}

QVector<PluginInfo> PluginHost::getEffects() const {
    QVector<PluginInfo> results;
    for (const PluginInfo& info : m_availablePlugins) {
        if (info.isEffect && !info.isSynth) {
            results.append(info);
        }
    }
    return results;
}

void PluginHost::addPlugin(const PluginInfo& info) {
    // Check if already exists
    for (const PluginInfo& existing : m_availablePlugins) {
        if (existing.id == info.id && existing.format == info.format) {
            return;
        }
    }
    m_availablePlugins.append(info);
}

void PluginHost::removePlugin(const QString& id) {
    for (int i = 0; i < m_availablePlugins.size(); i++) {
        if (m_availablePlugins[i].id == id) {
            m_availablePlugins.removeAt(i);
            break;
        }
    }
}

void PluginHost::clearPlugins() {
    m_availablePlugins.clear();
}

PluginInstance* PluginHost::createVST3(const PluginInfo& info) {
    // TODO: Implement VST3 instance creation
    // This would load the .vst3, create the component, and return a wrapper
    return nullptr;
}

PluginInstance* PluginHost::createLV2(const PluginInfo& info) {
    // TODO: Implement LV2 instance creation
    return nullptr;
}

PluginInstance* PluginHost::createCLAP(const PluginInfo& info) {
    // TODO: Implement CLAP instance creation
    return nullptr;
}

PluginInstance* PluginHost::createAU(const PluginInfo& info) {
    // TODO: Implement AudioUnit instance creation
    return nullptr;
}