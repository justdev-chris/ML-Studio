#include "MainWindow.h"
#include "dialogs/PreferencesDialog.h"
#include "dialogs/ExportDialog.h"
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_engine(nullptr)
    , m_project(nullptr)
    , m_audioIO(nullptr)
    , m_midi(nullptr)
    , m_pluginHost(nullptr) {
    setupUI();
    setupConnections();
    setWindowTitle("MeowyLoops Studio");
    resize(1400, 900);

    // Initialize engine
    m_engine = new AudioEngine(this);
    m_engine->initialize(44100, 256);

    // Initialize AudioIO
    m_audioIO = new AudioIO(this);
    m_engine->setAudioIO(m_audioIO);

    // Initialize MIDI
    m_midi = new MIDI(this);
    m_midi->initialize();

    // Initialize PluginHost
    m_pluginHost = new PluginHost(this);
    m_pluginHost->setSampleRate(44100);
    m_pluginHost->setBlockSize(256);

    // Load preferences
    loadPreferences();

    // Create new project
    newProject();

    // Scan plugins
    scanPlugins();
}

MainWindow::~MainWindow() {
    delete m_project;
}

void MainWindow::setupUI() {
    createMenuBar();
    createToolBar();
    createStatusBar();
    createCentralWidget();
}

void MainWindow::createMenuBar() {
    QMenuBar* menuBar = this->menuBar();

    // File menu
    QMenu* fileMenu = menuBar->addMenu("&File");

    QAction* newAction = new QAction("&New Project", this);
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::newProject);
    fileMenu->addAction(newAction);

    QAction* openAction = new QAction("&Open Project...", this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openProject);
    fileMenu->addAction(openAction);

    QAction* saveAction = new QAction("&Save Project", this);
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveProject);
    fileMenu->addAction(saveAction);

    QAction* saveAsAction = new QAction("Save Project &As...", this);
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveProjectAs);
    fileMenu->addAction(saveAsAction);

    fileMenu->addSeparator();

    QAction* exportAction = new QAction("&Export Audio...", this);
    exportAction->setShortcut(QKeySequence("Ctrl+E"));
    connect(exportAction, &QAction::triggered, this, &MainWindow::exportAudio);
    fileMenu->addAction(exportAction);

    fileMenu->addSeparator();

    QAction* exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);

    // Edit menu
    QMenu* editMenu = menuBar->addMenu("&Edit");

    QAction* prefsAction = new QAction("&Preferences", this);
    prefsAction->setShortcut(QKeySequence("Ctrl+,"));
    connect(prefsAction, &QAction::triggered, this, &MainWindow::showPreferences);
    editMenu->addAction(prefsAction);

    // View menu
    QMenu* viewMenu = menuBar->addMenu("&View");

    // Track menu
    QMenu* trackMenu = menuBar->addMenu("&Track");

    QAction* addTrackAction = new QAction("Add &Audio Track", this);
    connect(addTrackAction, &QAction::triggered, this, &MainWindow::addAudioTrack);
    trackMenu->addAction(addTrackAction);

    QAction* addMIDITrackAction = new QAction("Add &MIDI Track", this);
    connect(addMIDITrackAction, &QAction::triggered, this, &MainWindow::addMIDITrack);
    trackMenu->addAction(addMIDITrackAction);

    // Help menu
    QMenu* helpMenu = menuBar->addMenu("&Help");

    QAction* aboutAction = new QAction("&About", this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    helpMenu->addAction(aboutAction);
}

void MainWindow::createToolBar() {
    QToolBar* toolbar = addToolBar("Main");
    toolbar->setMovable(false);

    // Transport buttons
    QAction* playAction = new QAction("▶ Play", this);
    connect(playAction, &QAction::triggered, this, &MainWindow::play);
    toolbar->addAction(playAction);

    QAction* stopAction = new QAction("■ Stop", this);
    connect(stopAction, &QAction::triggered, this, &MainWindow::stop);
    toolbar->addAction(stopAction);

    QAction* recordAction = new QAction("● Record", this);
    connect(recordAction, &QAction::triggered, this, &MainWindow::record);
    toolbar->addAction(recordAction);

    toolbar->addSeparator();

    QAction* loopAction = new QAction("🔁 Loop", this);
    loopAction->setCheckable(true);
    connect(loopAction, &QAction::triggered, this, &MainWindow::toggleLoop);
    toolbar->addAction(loopAction);
}

void MainWindow::createStatusBar() {
    QStatusBar* statusBar = this->statusBar();
    statusBar->showMessage("Ready");
}

void MainWindow::createCentralWidget() {
    // Create widgets
    m_timeline = new TimelineWidget(this);
    m_mixerWidget = new MixerWidget(this);
    m_transport = new TransportWidget(this);
    m_pluginBrowser = new PluginBrowserWidget(this);
    m_pianoRoll = new PianoRollWidget(this);

    // Main splitter
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, this);

    // Left side: timeline + piano roll
    QWidget* leftPanel = new QWidget(this);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);
    leftLayout->addWidget(m_timeline, 3);
    leftLayout->addWidget(m_pianoRoll, 1);

    // Right side: mixer + plugin browser
    QWidget* rightPanel = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    rightLayout->addWidget(m_mixerWidget, 2);
    rightLayout->addWidget(m_pluginBrowser, 1);

    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setSizes({1000, 400});

    // Transport at bottom
    QWidget* central = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(mainSplitter, 1);
    layout->addWidget(m_transport, 0);

    setCentralWidget(central);
}

void MainWindow::setupConnections() {
    // Transport → Engine
    connect(m_transport, &TransportWidget::playPressed, this, &MainWindow::play);
    connect(m_transport, &TransportWidget::stopPressed, this, &MainWindow::stop);
    connect(m_transport, &TransportWidget::recordPressed, this, &MainWindow::record);
    connect(m_transport, &TransportWidget::loopToggled, this, &MainWindow::toggleLoop);
    connect(m_transport, &TransportWidget::tempoChanged, this, &MainWindow::setTempo);
    connect(m_transport, &TransportWidget::positionChanged, this, &MainWindow::setPosition);

    // Mixer → Engine
    connect(m_mixerWidget, &MixerWidget::trackVolumeChanged, this, &MainWindow::setTrackVolume);
    connect(m_mixerWidget, &MixerWidget::trackPanChanged, this, &MainWindow::setTrackPan);
    connect(m_mixerWidget, &MixerWidget::trackMuteToggled, this, &MainWindow::setTrackMute);
    connect(m_mixerWidget, &MixerWidget::trackSoloToggled, this, &MainWindow::setTrackSolo);
    connect(m_mixerWidget, &MixerWidget::masterVolumeChanged, this, &MainWindow::setMasterVolume);

    // Plugin browser → PluginHost
    connect(m_pluginBrowser, &PluginBrowserWidget::pluginDoubleClicked, this, &MainWindow::addPluginToTrack);

    // MIDI → PianoRoll
    connect(m_midi, &MIDI::noteOnReceived, m_pianoRoll, &PianoRollWidget::midiNoteOn);
    connect(m_midi, &MIDI::noteOffReceived, m_pianoRoll, &PianoRollWidget::midiNoteOff);

    // Engine → Transport
    connect(m_engine, &AudioEngine::positionChanged, m_transport, &TransportWidget::updateTimeDisplay);
    connect(m_engine, &AudioEngine::transportStateChanged, this, &MainWindow::updateTransportState);
}

void MainWindow::newProject() {
    if (m_project && m_project->isDirty()) {
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Save Project",
            "Do you want to save the current project?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (reply == QMessageBox::Yes) {
            saveProject();
        } else if (reply == QMessageBox::Cancel) {
            return;
        }
    }

    delete m_project;
    m_project = new Project(this);
    m_project->setName("Untitled");
    m_project->setAuthor("Unknown");

    m_engine->clearProject();
    m_engine->loadProject(m_project);

    updateUI();
    statusBar()->showMessage("New project created");
}

void MainWindow::openProject() {
    QString path = QFileDialog::getOpenFileName(this, "Open Project",
        QDir::homePath() + "/Documents/MeowyLoops/Projects",
        "MeowyLoops Projects (*.mlproj)");

    if (path.isEmpty()) return;

    ProjectLoader loader;
    Project* project = new Project(this);
    if (loader.load(path, project)) {
        delete m_project;
        m_project = project;
        m_engine->clearProject();
        m_engine->loadProject(m_project);
        updateUI();
        statusBar()->showMessage("Project loaded: " + path);
    } else {
        QMessageBox::warning(this, "Error", "Failed to load project: " + loader.getLastError());
        delete project;
    }
}

void MainWindow::saveProject() {
    if (!m_project) return;
    if (m_project->getFilePath().isEmpty()) {
        saveProjectAs();
        return;
    }

    ProjectSaver saver;
    if (saver.save(m_project->getFilePath(), m_project)) {
        statusBar()->showMessage("Project saved: " + m_project->getFilePath());
    } else {
        QMessageBox::warning(this, "Error", "Failed to save project: " + saver.getLastError());
    }
}

void MainWindow::saveProjectAs() {
    if (!m_project) return;

    QString path = QFileDialog::getSaveFileName(this, "Save Project As",
        QDir::homePath() + "/Documents/MeowyLoops/Projects/" + m_project->getName() + ".mlproj",
        "MeowyLoops Projects (*.mlproj)");

    if (path.isEmpty()) return;

    ProjectSaver saver;
    if (saver.save(path, m_project)) {
        statusBar()->showMessage("Project saved: " + path);
    } else {
        QMessageBox::warning(this, "Error", "Failed to save project: " + saver.getLastError());
    }
}

void MainWindow::exportAudio() {
    if (!m_project) return;

    ExportDialog dialog(this);
    dialog.setProjectName(m_project->getName());
    dialog.setExportPath(QDir::homePath() + "/Documents/MeowyLoops/Exports");
    connect(&dialog, &ExportDialog::exportRequested, this, &MainWindow::doExport);
    dialog.exec();
}

void MainWindow::doExport(const QString& path, const QString& format, int bitDepth, int sampleRate, bool normalize) {
    Q_UNUSED(format);
    Q_UNUSED(bitDepth);
    Q_UNUSED(sampleRate);
    Q_UNUSED(normalize);

    // TODO: Implement actual export
    statusBar()->showMessage("Exporting to: " + path);
    QMessageBox::information(this, "Export", "Export not yet implemented.");
}

void MainWindow::showPreferences() {
    PreferencesDialog dialog(this);
    connect(&dialog, &PreferencesDialog::settingsChanged, this, &MainWindow::applyPreferences);
    dialog.exec();
}

void MainWindow::applyPreferences() {
    QSettings settings("MeowyLoops", "Studio");

    // Apply audio settings
    int sampleRate = settings.value("audio/sampleRate", 44100).toInt();
    int bufferSize = settings.value("audio/bufferSize", 256).toInt();
    m_engine->initialize(sampleRate, bufferSize);

    // Apply theme
    QString theme = settings.value("appearance/theme", "Dark").toString();
    // TODO: Apply theme

    statusBar()->showMessage("Preferences applied");
}

void MainWindow::loadPreferences() {
    QSettings settings("MeowyLoops", "Studio");

    // Load audio settings
    int sampleRate = settings.value("audio/sampleRate", 44100).toInt();
    int bufferSize = settings.value("audio/bufferSize", 256).toInt();
    m_engine->initialize(sampleRate, bufferSize);

    // Load appearance settings
    QString theme = settings.value("appearance/theme", "Dark").toString();
    // TODO: Apply theme
}

void MainWindow::scanPlugins() {
    QSettings settings("MeowyLoops", "Studio");
    QStringList paths = settings.value("plugins/paths").toStringList();
    if (paths.isEmpty()) {
#ifdef _WIN32
        paths.append("C:/Program Files/Common Files/VST3");
#elif __APPLE__
        paths.append("/Library/Audio/Plug-Ins/VST3");
#else
        paths.append("/usr/lib/vst3");
#endif
    }

    for (const QString& path : paths) {
        m_pluginHost->scanPlugins(path);
    }
}

void MainWindow::addAudioTrack() {
    if (!m_engine) return;
    Track* track = m_engine->addTrack(TrackType::Audio);
    m_mixerWidget->setTrackCount(m_engine->getTrackCount());
    updateUI();
}

void MainWindow::addMIDITrack() {
    if (!m_engine) return;
    Track* track = m_engine->addTrack(TrackType::MIDI);
    m_mixerWidget->setTrackCount(m_engine->getTrackCount());
    updateUI();
}

void MainWindow::addPluginToTrack(const PluginInfo& info) {
    // Add plugin to selected track
    // TODO: Get selected track from UI
    statusBar()->showMessage("Added plugin: " + info.name);
}

void MainWindow::play() {
    if (m_engine) {
        m_engine->play();
    }
}

void MainWindow::stop() {
    if (m_engine) {
        m_engine->stop();
    }
}

void MainWindow::record() {
    if (m_engine) {
        m_engine->record();
    }
}

void MainWindow::toggleLoop(bool enabled) {
    if (m_engine && m_engine->getProject()) {
        m_engine->getProject()->getTransport()->setLooping(enabled);
    }
}

void MainWindow::setTempo(int bpm) {
    if (m_engine) {
        m_engine->setTempo(bpm);
    }
}

void MainWindow::setPosition(int frames) {
    // Convert frames to seconds
    double seconds = frames / 44100.0;
    if (m_engine) {
        m_engine->setPosition(seconds);
    }
}

void MainWindow::setTrackVolume(int index, float volume) {
    if (m_engine && m_engine->getTrack(index)) {
        m_engine->getTrack(index)->setVolume(volume);
    }
}

void MainWindow::setTrackPan(int index, float pan) {
    if (m_engine && m_engine->getTrack(index)) {
        m_engine->getTrack(index)->setPan(pan);
    }
}

void MainWindow::setTrackMute(int index, bool muted) {
    if (m_engine && m_engine->getTrack(index)) {
        m_engine->getTrack(index)->setMuted(muted);
    }
}

void MainWindow::setTrackSolo(int index, bool soloed) {
    if (m_engine && m_engine->getTrack(index)) {
        m_engine->getTrack(index)->setSoloed(soloed);
    }
}

void MainWindow::setMasterVolume(float volume) {
    if (m_engine) {
        m_engine->setMasterVolume(volume);
    }
}

void MainWindow::updateTransportState(bool playing) {
    m_transport->setPlaying(playing);
}

void MainWindow::updateUI() {
    if (!m_engine) return;

    // Update mixer
    m_mixerWidget->setTrackCount(m_engine->getTrackCount());
    for (int i = 0; i < m_engine->getTrackCount(); i++) {
        Track* track = m_engine->getTrack(i);
        if (track) {
            m_mixerWidget->setTrackName(i, track->getName());
            m_mixerWidget->setTrackVolume(i, track->getVolume());
            m_mixerWidget->setTrackPan(i, track->getPan());
            m_mixerWidget->setTrackMute(i, track->isMuted());
            m_mixerWidget->setTrackSolo(i, track->isSoloed());
        }
    }

    // Update timeline
    m_timeline->setTracks(m_engine->getTrackCount());
    // TODO: Add clips to timeline
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "About MeowyLoops Studio",
        "<h2>MeowyLoops Studio</h2>"
        "<p>Version 1.0.0</p>"
        "<p>A cat-powered DAW.</p>"
        "<p>Built by Chris</p>");
}
