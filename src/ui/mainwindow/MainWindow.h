#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "ui/timeline/TimelineWidget.h"
#include "ui/mixer/MixerWidget.h"
#include "ui/transport/TransportWidget.h"
#include "ui/pluginbrowser/PluginBrowserWidget.h"
#include "ui/pianoroll/PianoRollWidget.h"
#include "core/engine/AudioEngine.h"
#include "audio/io/AudioIO.h"
#include "audio/midi/MIDI.h"
#include "plugins/host/PluginHost.h"
#include "project/Project.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    // Project
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void exportAudio();
    void doExport(const QString& path, const QString& format, int bitDepth, int sampleRate, bool normalize);

    // Preferences
    void showPreferences();
    void applyPreferences();
    void loadPreferences();

    // Transport
    void play();
    void stop();
    void record();
    void toggleLoop(bool enabled);
    void setTempo(int bpm);
    void setPosition(int frames);
    void updateTransportState(bool playing);
    void updatePlayhead(double seconds);

    // Tracks
    void addAudioTrack();
    void addMIDITrack();
    void addPluginToTrack(const PluginInfo& info);

    // Mixer
    void setTrackVolume(int index, float volume);
    void setTrackPan(int index, float pan);
    void setTrackMute(int index, bool muted);
    void setTrackSolo(int index, bool soloed);
    void setMasterVolume(float volume);

    // Plugin scanning
    void scanPlugins();

    // UI updates
    void updateUI();
    void updateTimeline();

    // About
    void showAbout();

private:
    void setupUI();
    void createMenuBar();
    void createToolBar();
    void createStatusBar();
    void createCentralWidget();
    void setupConnections();

    // Core components
    AudioEngine* m_engine;
    Project* m_project;
    AudioIO* m_audioIO;
    MIDI* m_midi;
    PluginHost* m_pluginHost;

    // UI components
    TimelineWidget* m_timeline;
    MixerWidget* m_mixerWidget;
    TransportWidget* m_transport;
    PluginBrowserWidget* m_pluginBrowser;
    PianoRollWidget* m_pianoRoll;

    // State
    bool m_isPlaying = false;
    bool m_isRecording = false;
    double m_currentPosition = 0.0;
};

#endif // MAINWINDOW_H
