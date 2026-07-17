#include "MainWindow.h"

#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setupUI();
    setWindowTitle("MeowyLoops Studio");
    resize(1400, 900);
}

MainWindow::~MainWindow() {}

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

    QAction* newAction = new QAction("&New", this);
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, []() {
        // TODO: New project
    });
    fileMenu->addAction(newAction);

    QAction* openAction = new QAction("&Open...", this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, []() {
        // TODO: Open project
    });
    fileMenu->addAction(openAction);

    QAction* saveAction = new QAction("&Save", this);
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, []() {
        // TODO: Save project
    });
    fileMenu->addAction(saveAction);

    fileMenu->addSeparator();

    QAction* exportAction = new QAction("&Export...", this);
    exportAction->setShortcut(QKeySequence("Ctrl+E"));
    connect(exportAction, &QAction::triggered, this, []() {
        // TODO: Export audio
    });
    fileMenu->addAction(exportAction);

    fileMenu->addSeparator();

    QAction* exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);

    // Edit menu
    QMenu* editMenu = menuBar->addMenu("&Edit");

    QAction* undoAction = new QAction("&Undo", this);
    undoAction->setShortcut(QKeySequence::Undo);
    editMenu->addAction(undoAction);

    QAction* redoAction = new QAction("&Redo", this);
    redoAction->setShortcut(QKeySequence::Redo);
    editMenu->addAction(redoAction);

    editMenu->addSeparator();

    QAction* prefsAction = new QAction("&Preferences", this);
    prefsAction->setShortcut(QKeySequence("Ctrl+,"));
    connect(prefsAction, &QAction::triggered, this, []() {
        // TODO: Preferences dialog
    });
    editMenu->addAction(prefsAction);

    // View menu
    QMenu* viewMenu = menuBar->addMenu("&View");
    // TODO: Toggle views

    // Track menu
    QMenu* trackMenu = menuBar->addMenu("&Track");
    // TODO: Add track, delete track, etc.

    // Help menu
    QMenu* helpMenu = menuBar->addMenu("&Help");

    QAction* aboutAction = new QAction("&About", this);
    connect(aboutAction, &QAction::triggered, this, []() {
        QMessageBox::about(nullptr, "About MeowyLoops Studio",
            "<h2>MeowyLoops Studio</h2>"
            "<p>Version 1.0.0</p>"
            "<p>A cat-powered DAW.</p>");
    });
    helpMenu->addAction(aboutAction);
}

void MainWindow::createToolBar() {
    QToolBar* toolbar = addToolBar("Main");
    toolbar->setMovable(false);

    // TODO: Add transport buttons (play, stop, record, loop)
    // Will be filled when TransportWidget is ready
}

void MainWindow::createStatusBar() {
    QStatusBar* statusBar = this->statusBar();
    statusBar->showMessage("Ready");
}

void MainWindow::createCentralWidget() {
    // Create widgets
    timeline = new TimelineWidget(this);
    mixer = new MixerWidget(this);
    transport = new TransportWidget(this);
    pluginBrowser = new PluginBrowserWidget(this);
    pianoRoll = new PianoRollWidget(this);

    // Main splitter (left: timeline + piano roll, right: mixer + plugin browser)
    mainSplitter = new QSplitter(Qt::Horizontal, this);

    // Left side: timeline on top, piano roll on bottom
    QWidget* leftPanel = new QWidget(this);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    leftLayout->addWidget(timeline, 3);
    leftLayout->addWidget(pianoRoll, 1);

    // Right side: mixer on top, plugin browser on bottom
    QWidget* rightPanel = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    rightLayout->addWidget(mixer, 2);
    rightLayout->addWidget(pluginBrowser, 1);

    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setSizes({1000, 400});

    // Transport at the bottom
    QWidget* central = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(mainSplitter, 1);
    layout->addWidget(transport, 0);

    setCentralWidget(central);
}