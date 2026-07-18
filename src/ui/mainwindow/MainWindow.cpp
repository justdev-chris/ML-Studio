#include "MainWindow.h"
#include "dialogs/PreferencesDialog.h"
#include "dialogs/ExportDialog.h"
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QSettings>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_engine(new AudioEngine(this))
    , m_project(nullptr)
    , m_audioIO(new AudioIO(this))
    , m_midi(new MIDI(this))
    , m_pluginHost(new PluginHost(this)) {

    setupUI();
    setupConnections();
    setWindowTitle("MeowyLoops Studio");
    resize(1400, 900);

    m_engine->setAudioIO(m_audioIO);
    if (!m_engine->initialize(44100, 256)) {
        qWarning() << "Failed to initialize audio engine";
    }

    m_midi->initialize();
    m_pluginHost->setSampleRate(44100);
    m_pluginHost->setBlockSize(256);

    loadPreferences();
    newProject();
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

    // Edit, Track, Help menus similarly...
}

void MainWindow::createToolBar() {
    QToolBar* toolbar = addToolBar("Main");
    toolbar->setMovable(false);

    QAction* playAction = new QAction("▶ Play", this);
    connect(playAction, &QAction::triggered, this, &MainWindow::play);
    toolbar->addAction(playAction);

    QAction* stopAction = new QAction("■ Stop", this);
    connect(stopAction, &QAction::triggered, this, &MainWindow::stop);
    toolbar->addAction(stopAction);

    QAction* recordAction = new QAction("● Record", this);
    recordAction->setCheckable(true);
    connect(recordAction, &QAction::toggled, this, &MainWindow::record);
    toolbar->addAction(recordAction);

    toolbar->addSeparator();

    QAction* loopAction = new QAction("🔁 Loop", this);
    loopAction->setCheckable(true);
    connect(loopAction, &QAction::triggered, this, &MainWindow::toggleLoop);
    toolbar->addAction(loopAction);
}

void MainWindow::createStatusBar() {
    statusBar()->showMessage("Ready");
}

void MainWindow::createCentralWidget() {
    m_timeline = new TimelineWidget(this);
    m_mixerWidget = new MixerWidget(this);
    m_transport = new TransportWidget(this);
    m_pluginBrowser = new PluginBrowserWidget(this);
    m_pianoRoll = new PianoRollWidget(this);

    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, this);

    QWidget* leftPanel = new QWidget(this);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->addWidget(m_timeline, 3);
    leftLayout->addWidget(m_pianoRoll, 1);

    QWidget* rightPanel = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->addWidget(m_mixerWidget, 2);
    rightLayout->addWidget(m_pluginBrowser, 1);

    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setSizes({1000, 400});

    QWidget* central = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(central);
    layout->addWidget(mainSplitter, 1);
    layout->addWidget(m_transport, 0);

    setCentralWidget(central);
}

void MainWindow::setupConnections() {
    connect(m_transport, &TransportWidget::playPressed, this, &MainWindow::play);
    connect(m_transport, &TransportWidget::stopPressed, this, &MainWindow::stop);
    connect(m_transport, &TransportWidget::recordPressed, this, &MainWindow::record);
    connect(m_transport, &TransportWidget::loopToggled, this, &MainWindow::toggleLoop);
    connect(m_transport, &TransportWidget::tempoChanged, this, &MainWindow::setTempo);
    connect(m_transport, &TransportWidget::positionChanged, this, &MainWindow::setPosition);

    connect(m_engine, &AudioEngine::positionChanged, this, &MainWindow::updatePlayhead);
    connect(m_engine, &AudioEngine::transportStateChanged, this, &MainWindow::updateTransportState);

    connect(m_mixerWidget, &MixerWidget::trackVolumeChanged, this, &MainWindow::setTrackVolume);
    connect(m_mixerWidget, &MixerWidget::masterVolumeChanged, this, &MainWindow::setMasterVolume);

    connect(m_pluginBrowser, &PluginBrowserWidget::pluginDoubleClicked, this, &MainWindow::addPluginToTrack);
}

void MainWindow::newProject() {
    if (m_project && m_project->isDirty()) {
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Save Project", "Do you want to save the current project?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (reply == QMessageBox::Yes) saveProject();
        else if (reply == QMessageBox::Cancel) return;
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
    QString path = QFileDialog::getOpenFileName(this, "Open Project", QDir::homePath() + "/Documents/MeowyLoops/Projects", "MeowyLoops Projects (*.mlproj)");
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
        QMessageBox::warning(this, "Error", "Failed to load project");
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
        statusBar()->showMessage("Project saved");
    } else {
        QMessageBox::warning(this, "Error", "Failed to save project");
    }
}

void MainWindow::saveProjectAs() {
    if (!m_project) return;
    QString path = QFileDialog::getSaveFileName(this, "Save Project As", QDir::homePath() + "/Documents/MeowyLoops/Projects/" + m_project->getName() + ".mlproj", "MeowyLoops Projects (*.mlproj)");
    if (path.isEmpty()) return;

    ProjectSaver saver;
    if (saver.save(path, m_project)) {
        statusBar()->showMessage("Project saved: " + path);
    } else {
        QMessageBox::warning(this, "Error", "Failed to save project");
    }
}

void MainWindow::exportAudio() {
    if (!m_project) return;
    ExportDialog dialog(this);
    dialog.setProjectName(m_project->getName());
    connect(&dialog, &ExportDialog::exportRequested, this, &MainWindow::doExport);
    dialog.exec();
}

void MainWindow::doExport(const QString& path, const QString& format, int bitDepth, int sampleRate, bool normalize) {
    statusBar()->showMessage("Exporting to " + path);
    // Real export logic would go here calling engine
    QMessageBox::information(this, "Export", "Export finished (placeholder for now)");
}

void MainWindow::showPreferences() {
    PreferencesDialog dialog(this);
    connect(&dialog, &PreferencesDialog::settingsChanged, this, &MainWindow::applyPreferences);
    dialog.exec();
}

void MainWindow::applyPreferences() {
    QSettings settings("MeowyLoops", "Studio");
    int sampleRate = settings.value("audio/sampleRate", 44100).toInt();
    int bufferSize = settings.value("audio/bufferSize", 256).toInt();
    m_engine->initialize(sampleRate, bufferSize);
    m_pluginHost->setSampleRate(sampleRate);
    m_pluginHost->setBlockSize(bufferSize);
    statusBar()->showMessage("Preferences applied");
}

void MainWindow::loadPreferences() {
    QSettings settings("MeowyLoops", "Studio");
    int sampleRate = settings.value("audio/sampleRate", 44100).toInt();
    int bufferSize = settings.value("audio/bufferSize", 256).toInt();
    m_engine->initialize(sampleRate, bufferSize);
    m_pluginHost->setSampleRate(sampleRate);
    m_pluginHost->setBlockSize(bufferSize);
}

void MainWindow::play() { if (m_engine) m_engine->play(); }
void MainWindow::stop() { if (m_engine) m_engine->stop(); }
void MainWindow::record() { if (m_engine) m_engine->record(true); }
void MainWindow::toggleLoop(bool enabled) { /* implement */ }
void MainWindow::setTempo(int bpm) { if (m_engine) m_engine->setTempo(bpm); }
void MainWindow::setPosition(int frames) { if (m_engine) m_engine->setPosition(frames / 44100.0); }
void MainWindow::updateTransportState(bool playing) { m_isPlaying = playing; }
void MainWindow::updatePlayhead(double seconds) { m_currentPosition = seconds; updateTimeline(); }
void MainWindow::addAudioTrack() { if (m_engine) m_engine->addTrack(TrackType::Audio); }
void MainWindow::addMIDITrack() { if (m_engine) m_engine->addTrack(TrackType::MIDI); }
void MainWindow::addPluginToTrack(const PluginInfo& info) { /* implement */ }
void MainWindow::setTrackVolume(int index, float volume) { /* implement via engine */ }
void MainWindow::setMasterVolume(float volume) { if (m_engine) m_engine->setMasterVolume(volume); }
void MainWindow::scanPlugins() { if (m_pluginHost) m_pluginHost->scanPlugins(); }
void MainWindow::updateUI() { /* refresh widgets */ }
void MainWindow::updateTimeline() { /* update */ }
void MainWindow::showAbout() {
    QMessageBox::about(this, "About MeowyLoops Studio", "FL Studio inspired DAW");
}
