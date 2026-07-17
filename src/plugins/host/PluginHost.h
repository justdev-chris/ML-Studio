#ifndef PLUGINHOST_H
#define PLUGINHOST_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QList>
#include <QSharedPointer>
#include <functional>

struct PluginParameter {
    QString id;
    QString name;
    float value = 0.0f;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.0f;
    bool isAutomated = false;
    int index = -1;
};

struct PluginInfo {
    QString id;
    QString name;
    QString vendor;
    QString path;
    QString format;      // "VST3", "LV2", "CLAP", "AU"
    QString category;
    bool isSynth = false;
    bool isEffect = true;
    int numInputs = 2;
    int numOutputs = 2;
    QVector<PluginParameter> parameters;
    QMap<QString, QString> metadata;
};

class PluginInstance {
public:
    virtual ~PluginInstance() = default;

    virtual bool initialize(double sampleRate, int blockSize) = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;

    virtual void process(float** inputs, float** outputs, int numChannels, int numFrames) = 0;
    virtual void reset() = 0;

    virtual void setParameter(int index, float value) = 0;
    virtual float getParameter(int index) const = 0;
    virtual int getParameterCount() const = 0;

    virtual void setSampleRate(double sampleRate) = 0;
    virtual void setBlockSize(int blockSize) = 0;

    virtual PluginInfo getInfo() const = 0;
    virtual void* getNativeHandle() const = 0; // For editor UI

    // Optional: Editor UI
    virtual bool hasEditor() const { return false; }
    virtual void* createEditor(void* parent) { return nullptr; }
    virtual void destroyEditor(void* editor) {}
    virtual void editorResized(int width, int height) {}

signals:
    void parameterChanged(int index, float value);
    void processingError(const QString& error);
};

class PluginHost : public QObject {
    Q_OBJECT

public:
    explicit PluginHost(QObject* parent = nullptr);
    ~PluginHost();

    // Scanning
    void scanPlugins(const QString& path);
    void scanVST3(const QString& path);
    void scanLV2(const QString& path);
    void scanCLAP(const QString& path);
    void scanAU(const QString& path);

    // Plugin management
    QVector<PluginInfo> getAvailablePlugins() const { return m_availablePlugins; }
    PluginInfo getPluginInfo(const QString& id) const;
    PluginInstance* createInstance(const QString& id);
    void destroyInstance(PluginInstance* instance);

    // Search/filter
    QVector<PluginInfo> searchPlugins(const QString& query) const;
    QVector<PluginInfo> getPluginsByCategory(const QString& category) const;
    QVector<PluginInfo> getSynths() const;
    QVector<PluginInfo> getEffects() const;

    // Active instances
    QList<PluginInstance*> getActiveInstances() const { return m_activeInstances; }
    int getActiveInstanceCount() const { return m_activeInstances.size(); }

signals:
    void scanStarted(const QString& path);
    void scanProgress(int percent, const QString& message);
    void scanFinished(int pluginCount);
    void pluginFound(const PluginInfo& info);
    void instanceCreated(PluginInstance* instance);
    void instanceDestroyed(PluginInstance* instance);
    void error(const QString& message);

private:
    QVector<PluginInfo> m_availablePlugins;
    QList<PluginInstance*> m_activeInstances;

    void addPlugin(const PluginInfo& info);
    void removePlugin(const QString& id);
    void clearPlugins();

    // Plugin format handlers
    PluginInstance* createVST3(const PluginInfo& info);
    PluginInstance* createLV2(const PluginInfo& info);
    PluginInstance* createCLAP(const PluginInfo& info);
    PluginInstance* createAU(const PluginInfo& info);

    // Path scanning utilities
    void scanDirectory(const QString& path, const QStringList& extensions, const QString& format);
};

#endif // PLUGINHOST_H