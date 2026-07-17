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

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    void setupUI();
    void createMenuBar();
    void createToolBar();
    void createStatusBar();
    void createCentralWidget();

    // Central widgets
    TimelineWidget* timeline;
    MixerWidget* mixer;
    TransportWidget* transport;
    PluginBrowserWidget* pluginBrowser;
    PianoRollWidget* pianoRoll;

    QSplitter* mainSplitter;
    QSplitter* rightSplitter;
};

#endif