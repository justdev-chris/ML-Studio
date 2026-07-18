#include "VST3Host.h"
#include <QDebug>
#include <QDir>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

#define DECLARE_CLASS_IID(className, iid) static const char* const IID_##className = iid;
DECLARE_CLASS_IID(IComponent, "00000000-0000-0000-0000-000000000001")
DECLARE_CLASS_IID(IAudioProcessor, "00000000-0000-0000-0000-000000000002")
DECLARE_CLASS_IID(IEditController, "00000000-0000-0000-0000-000000000003")

VST3Host::VST3Host(const PluginInfo& info) : m_info(info) {}
VST3Host::~VST3Host() { shutdown(); }

bool VST3Host::initialize(double sampleRate, int blockSize) {
    if (m_initialized) return true;
    m_sampleRate = sampleRate; m_blockSize = blockSize;
    if (!loadPlugin()) return false;
    m_initialized = true;
    return true;
}

void VST3Host::shutdown() {
    if (!m_initialized) return;
    unloadPlugin();
    m_initialized = false;
    m_parameterCache.clear();
}

void VST3Host::process(float** inputs, float** outputs, int numChannels, int numFrames) {
    if (!m_initialized || !m_processor) {
        for (int c = 0; c < numChannels && c < 2; c++) {
            if (inputs[c] && outputs[c] && inputs[c] != outputs[c])
                memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
        }
        return;
    }
    // Real VST3 processing would go here
    for (int c = 0; c < numChannels && c < 2; c++) {
        if (inputs[c] && outputs[c] && inputs[c] != outputs[c])
            memcpy(outputs[c], inputs[c], numFrames * sizeof(float));
    }
}

void VST3Host::reset() {}
void VST3Host::setParameter(int index, float value) {
    if (index < 0 || index >= getParameterCount()) return;
    m_parameterCache[index] = value;
}
float VST3Host::getParameter(int index) const {
    return m_parameterCache.value(index, 0.0f);
}
int VST3Host::getParameterCount() const { return m_info.parameters.size(); }

bool VST3Host::hasEditor() const { return false; }
void* VST3Host::createEditor(void* parent) { return nullptr; }
void VST3Host::destroyEditor(void* editor) {}
void VST3Host::editorResized(int width, int height) {}

bool VST3Host::loadPlugin() {
    QString path = m_info.path;
    QFileInfo info(path);
    if (info.isDir()) {
        QDir dir(path);
#ifdef _WIN32
        QStringList subDirs = {"x86_64-win", "win64", "x64"};
        for (const QString& sub : subDirs) {
            QDir subDir(dir.absoluteFilePath(sub));
            if (subDir.exists()) {
                QStringList files = subDir.entryList({"*.dll"}, QDir::Files);
                if (!files.isEmpty()) { path = subDir.absoluteFilePath(files.first()); break; }
            }
        }
#elif __APPLE__
        QDir contentsDir(dir.absoluteFilePath("Contents/MacOS"));
        if (contentsDir.exists()) {
            QStringList files = contentsDir.entryList({"*.dylib", "*.so"}, QDir::Files);
            if (!files.isEmpty()) path = contentsDir.absoluteFilePath(files.first());
        }
#else
        QDir linuxDir(dir.absoluteFilePath("x86_64-linux"));
        if (linuxDir.exists()) {
            QStringList files = linuxDir.entryList({"*.so"}, QDir::Files);
            if (!files.isEmpty()) path = linuxDir.absoluteFilePath(files.first());
        }
#endif
    }

#ifdef _WIN32
    m_handle = LoadLibraryW(path.toStdWString().c_str());
    if (!m_handle) return false;
    using CreateFunc = void* (*)(const char*, const char*);
    m_createInstance = (CreateFunc)GetProcAddress((HMODULE)m_handle, "createInstance");
    if (!m_createInstance) { unloadPlugin(); return false; }
    m_component = m_createInstance(IID_IComponent, IID_IComponent);
    m_processor = m_createInstance(IID_IAudioProcessor, IID_IAudioProcessor);
    m_controller = m_createInstance(IID_IEditController, IID_IEditController);
#else
    m_handle = dlopen(path.toUtf8().constData(), RTLD_LAZY);
    if (!m_handle) return false;
    using CreateFunc = void* (*)(const char*, const char*);
    m_createInstance = (CreateFunc)dlsym(m_handle, "createInstance");
    if (!m_createInstance) { unloadPlugin(); return false; }
    m_component = m_createInstance(IID_IComponent, IID_IComponent);
    m_processor = m_createInstance(IID_IAudioProcessor, IID_IAudioProcessor);
    m_controller = m_createInstance(IID_IEditController, IID_IEditController);
#endif
    return true;
}

void VST3Host::unloadPlugin() {
#ifdef _WIN32
    if (m_handle) FreeLibrary((HMODULE)m_handle);
#else
    if (m_handle) dlclose(m_handle);
#endif
    m_handle = nullptr;
    m_processor = m_controller = m_component = nullptr;
    m_createInstance = nullptr;
}
