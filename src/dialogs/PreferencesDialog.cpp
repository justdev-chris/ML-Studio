#include "PreferencesDialog.h"
#include <QSettings>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDebug>

PreferencesDialog::PreferencesDialog(QWidget* parent)
    : QDialog(parent) {
    setupUI();
    loadSettings();
    setWindowTitle("Preferences");
    resize(600, 500);
}

PreferencesDialog::~PreferencesDialog() {}

void PreferencesDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);

    // Tab widget
    m_tabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_tabWidget);

    // Create tabs
    createAudioTab();
    createMIDITab();
    createPluginTab();
    createAppearanceTab();
    createPathsTab();

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_resetButton = new QPushButton("Reset Defaults", this);
    connect(m_resetButton, &QPushButton::clicked, this, &PreferencesDialog::onResetDefaults);
    buttonLayout->addWidget(m_resetButton);

    buttonLayout->addSpacing(10);

    m_okButton = new QPushButton("OK", this);
    connect(m_okButton, &QPushButton::clicked, this, &PreferencesDialog::accept);
    buttonLayout->addWidget(m_okButton);

    m_applyButton = new QPushButton("Apply", this);
    connect(m_applyButton, &QPushButton::clicked, this, &PreferencesDialog::saveSettings);
    buttonLayout->addWidget(m_applyButton);

    m_cancelButton = new QPushButton("Cancel", this);
    connect(m_cancelButton, &QPushButton::clicked, this, &PreferencesDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);
}

void PreferencesDialog::createAudioTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);

    // Audio Device
    QGroupBox* deviceGroup = new QGroupBox("Audio Device", tab);
    QGridLayout* deviceLayout = new QGridLayout(deviceGroup);

    QLabel* deviceLabel = new QLabel("Output Device:", deviceGroup);
    m_audioDeviceCombo = new QComboBox(deviceGroup);
    m_audioDeviceCombo->addItems({"Default", "ASIO", "WASAPI", "DirectSound"});
    connect(m_audioDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreferencesDialog::onAudioDeviceChanged);

    deviceLayout->addWidget(deviceLabel, 0, 0);
    deviceLayout->addWidget(m_audioDeviceCombo, 0, 1);

    // Sample Rate
    QLabel* sampleRateLabel = new QLabel("Sample Rate:", deviceGroup);
    m_sampleRateCombo = new QComboBox(deviceGroup);
    m_sampleRateCombo->addItems({"44100", "48000", "88200", "96000", "192000"});
    connect(m_sampleRateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreferencesDialog::onSampleRateChanged);

    deviceLayout->addWidget(sampleRateLabel, 1, 0);
    deviceLayout->addWidget(m_sampleRateCombo, 1, 1);

    // Buffer Size
    QLabel* bufferLabel = new QLabel("Buffer Size:", deviceGroup);
    m_bufferSizeCombo = new QComboBox(deviceGroup);
    m_bufferSizeCombo->addItems({"64", "128", "256", "512", "1024", "2048"});
    connect(m_bufferSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreferencesDialog::onBufferSizeChanged);

    deviceLayout->addWidget(bufferLabel, 2, 0);
    deviceLayout->addWidget(m_bufferSizeCombo, 2, 1);

    // Latency
    QLabel* latencyLabel = new QLabel("Latency:", deviceGroup);
    m_latencySlider = new QSlider(Qt::Horizontal, deviceGroup);
    m_latencySlider->setRange(0, 100);
    m_latencySlider->setValue(50);
    m_latencyLabel = new QLabel("50 ms", deviceGroup);
    connect(m_latencySlider, &QSlider::valueChanged, this, [this](int value) {
        m_latencyLabel->setText(QString("%1 ms").arg(value));
    });

    deviceLayout->addWidget(latencyLabel, 3, 0);
    deviceLayout->addWidget(m_latencySlider, 3, 1);
    deviceLayout->addWidget(m_latencyLabel, 3, 2);

    layout->addWidget(deviceGroup);
    layout->addStretch();

    m_tabWidget->addTab(tab, "Audio");
}

void PreferencesDialog::createMIDITab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QGroupBox* midiGroup = new QGroupBox("MIDI Settings", tab);
    QGridLayout* midiLayout = new QGridLayout(midiGroup);

    QLabel* inputLabel = new QLabel("MIDI Input:", midiGroup);
    m_midiInputCombo = new QComboBox(midiGroup);
    m_midiInputCombo->addItem("None");

    QLabel* outputLabel = new QLabel("MIDI Output:", midiGroup);
    m_midiOutputCombo = new QComboBox(midiGroup);
    m_midiOutputCombo->addItem("None");

    midiLayout->addWidget(inputLabel, 0, 0);
    midiLayout->addWidget(m_midiInputCombo, 0, 1);
    midiLayout->addWidget(outputLabel, 1, 0);
    midiLayout->addWidget(m_midiOutputCombo, 1, 1);

    layout->addWidget(midiGroup);
    layout->addStretch();

    m_tabWidget->addTab(tab, "MIDI");
}

void PreferencesDialog::createPluginTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QGroupBox* pluginGroup = new QGroupBox("Plugin Paths", tab);
    QVBoxLayout* pluginLayout = new QVBoxLayout(pluginGroup);

    m_pluginPathList = new QListWidget(pluginGroup);
    m_pluginPathList->addItem("C:/Program Files/Common Files/VST3");
    m_pluginPathList->addItem("/Library/Audio/Plug-Ins/VST3");
    m_pluginPathList->addItem("/usr/lib/vst3");

    pluginLayout->addWidget(m_pluginPathList);

    QHBoxLayout* pathButtonLayout = new QHBoxLayout();
    m_addPathButton = new QPushButton("Add Path", pluginGroup);
    connect(m_addPathButton, &QPushButton::clicked, this, &PreferencesDialog::onPluginPathAdded);
    pathButtonLayout->addWidget(m_addPathButton);

    m_removePathButton = new QPushButton("Remove Path", pluginGroup);
    connect(m_removePathButton, &QPushButton::clicked, this, &PreferencesDialog::onPluginPathRemoved);
    pathButtonLayout->addWidget(m_removePathButton);

    pathButtonLayout->addStretch();
    pluginLayout->addLayout(pathButtonLayout);

    layout->addWidget(pluginGroup);
    layout->addStretch();

    m_tabWidget->addTab(tab, "Plugins");
}

void PreferencesDialog::createAppearanceTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QGroupBox* themeGroup = new QGroupBox("Theme", tab);
    QGridLayout* themeLayout = new QGridLayout(themeGroup);

    QLabel* themeLabel = new QLabel("Theme:", themeGroup);
    m_themeCombo = new QComboBox(themeGroup);
    m_themeCombo->addItems({"Dark", "Light", "Cat"});
    themeLayout->addWidget(themeLabel, 0, 0);
    themeLayout->addWidget(m_themeCombo, 0, 1);

    m_darkModeCheck = new QCheckBox("Use Dark Mode", themeGroup);
    m_darkModeCheck->setChecked(true);
    themeLayout->addWidget(m_darkModeCheck, 1, 0, 1, 2);

    layout->addWidget(themeGroup);

    QGroupBox* editorGroup = new QGroupBox("Editor", tab);
    QVBoxLayout* editorLayout = new QVBoxLayout(editorGroup);

    m_showGridCheck = new QCheckBox("Show Grid", editorGroup);
    m_showGridCheck->setChecked(true);
    editorLayout->addWidget(m_showGridCheck);

    m_snapToGridCheck = new QCheckBox("Snap to Grid", editorGroup);
    m_snapToGridCheck->setChecked(true);
    editorLayout->addWidget(m_snapToGridCheck);

    layout->addWidget(editorGroup);
    layout->addStretch();

    m_tabWidget->addTab(tab, "Appearance");
}

void PreferencesDialog::createPathsTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QGroupBox* pathsGroup = new QGroupBox("File Paths", tab);
    QGridLayout* pathsLayout = new QGridLayout(pathsGroup);

    QLabel* projectLabel = new QLabel("Projects:", pathsGroup);
    m_projectPathEdit = new QLineEdit(pathsGroup);
    m_projectPathEdit->setReadOnly(true);
    m_projectPathEdit->setText(QDir::homePath() + "/Documents/MeowyLoops/Projects");
    m_browseProjectButton = new QPushButton("Browse", pathsGroup);
    connect(m_browseProjectButton, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getExistingDirectory(this, "Select Project Folder");
        if (!path.isEmpty()) m_projectPathEdit->setText(path);
    });

    pathsLayout->addWidget(projectLabel, 0, 0);
    pathsLayout->addWidget(m_projectPathEdit, 0, 1);
    pathsLayout->addWidget(m_browseProjectButton, 0, 2);

    QLabel* exportLabel = new QLabel("Exports:", pathsGroup);
    m_exportPathEdit = new QLineEdit(pathsGroup);
    m_exportPathEdit->setReadOnly(true);
    m_exportPathEdit->setText(QDir::homePath() + "/Documents/MeowyLoops/Exports");
    m_browseExportButton = new QPushButton("Browse", pathsGroup);
    connect(m_browseExportButton, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getExistingDirectory(this, "Select Export Folder");
        if (!path.isEmpty()) m_exportPathEdit->setText(path);
    });

    pathsLayout->addWidget(exportLabel, 1, 0);
    pathsLayout->addWidget(m_exportPathEdit, 1, 1);
    pathsLayout->addWidget(m_browseExportButton, 1, 2);

    QLabel* tempLabel = new QLabel("Temp:", pathsGroup);
    m_tempPathEdit = new QLineEdit(pathsGroup);
    m_tempPathEdit->setReadOnly(true);
    m_tempPathEdit->setText(QDir::tempPath() + "/MeowyLoops");
    m_browseTempButton = new QPushButton("Browse", pathsGroup);
    connect(m_browseTempButton, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getExistingDirectory(this, "Select Temp Folder");
        if (!path.isEmpty()) m_tempPathEdit->setText(path);
    });

    pathsLayout->addWidget(tempLabel, 2, 0);
    pathsLayout->addWidget(m_tempPathEdit, 2, 1);
    pathsLayout->addWidget(m_browseTempButton, 2, 2);

    layout->addWidget(pathsGroup);
    layout->addStretch();

    m_tabWidget->addTab(tab, "Paths");
}

void PreferencesDialog::loadSettings() {
    QSettings settings("MeowyLoops", "Studio");

    // Audio
    m_audioDeviceCombo->setCurrentText(settings.value("audio/device", "Default").toString());
    m_sampleRateCombo->setCurrentText(settings.value("audio/sampleRate", "44100").toString());
    m_bufferSizeCombo->setCurrentText(settings.value("audio/bufferSize", "256").toString());
    m_latencySlider->setValue(settings.value("audio/latency", 50).toInt());
    m_latencyLabel->setText(QString("%1 ms").arg(m_latencySlider->value()));

    // MIDI
    // (populate from actual MIDI devices)

    // Plugin paths
    // (populate from settings)

    // Appearance
    m_themeCombo->setCurrentText(settings.value("appearance/theme", "Dark").toString());
    m_darkModeCheck->setChecked(settings.value("appearance/darkMode", true).toBool());
    m_showGridCheck->setChecked(settings.value("appearance/showGrid", true).toBool());
    m_snapToGridCheck->setChecked(settings.value("appearance/snapToGrid", true).toBool());

    // Paths
    m_projectPathEdit->setText(settings.value("paths/projects", QDir::homePath() + "/Documents/MeowyLoops/Projects").toString());
    m_exportPathEdit->setText(settings.value("paths/exports", QDir::homePath() + "/Documents/MeowyLoops/Exports").toString());
    m_tempPathEdit->setText(settings.value("paths/temp", QDir::tempPath() + "/MeowyLoops").toString());
}

void PreferencesDialog::saveSettings() {
    QSettings settings("MeowyLoops", "Studio");

    // Audio
    settings.setValue("audio/device", m_audioDeviceCombo->currentText());
    settings.setValue("audio/sampleRate", m_sampleRateCombo->currentText());
    settings.setValue("audio/bufferSize", m_bufferSizeCombo->currentText());
    settings.setValue("audio/latency", m_latencySlider->value());

    // Appearance
    settings.setValue("appearance/theme", m_themeCombo->currentText());
    settings.setValue("appearance/darkMode", m_darkModeCheck->isChecked());
    settings.setValue("appearance/showGrid", m_showGridCheck->isChecked());
    settings.setValue("appearance/snapToGrid", m_snapToGridCheck->isChecked());

    // Paths
    settings.setValue("paths/projects", m_projectPathEdit->text());
    settings.setValue("paths/exports", m_exportPathEdit->text());
    settings.setValue("paths/temp", m_tempPathEdit->text());

    emit settingsChanged();
}

void PreferencesDialog::onAudioDeviceChanged(int index) {
    Q_UNUSED(index);
}

void PreferencesDialog::onSampleRateChanged(int index) {
    Q_UNUSED(index);
}

void PreferencesDialog::onBufferSizeChanged(int index) {
    Q_UNUSED(index);
}

void PreferencesDialog::onPluginPathAdded() {
    QString path = QFileDialog::getExistingDirectory(this, "Select Plugin Folder");
    if (!path.isEmpty()) {
        m_pluginPathList->addItem(path);
    }
}

void PreferencesDialog::onPluginPathRemoved() {
    int row = m_pluginPathList->currentRow();
    if (row >= 0) {
        delete m_pluginPathList->takeItem(row);
    }
}

void PreferencesDialog::onThemeChanged(int index) {
    Q_UNUSED(index);
}

void PreferencesDialog::onResetDefaults() {
    // Reset all settings to defaults
    m_audioDeviceCombo->setCurrentIndex(0);
    m_sampleRateCombo->setCurrentText("44100");
    m_bufferSizeCombo->setCurrentText("256");
    m_latencySlider->setValue(50);
    m_latencyLabel->setText("50 ms");

    m_themeCombo->setCurrentText("Dark");
    m_darkModeCheck->setChecked(true);
    m_showGridCheck->setChecked(true);
    m_snapToGridCheck->setChecked(true);

    m_projectPathEdit->setText(QDir::homePath() + "/Documents/MeowyLoops/Projects");
    m_exportPathEdit->setText(QDir::homePath() + "/Documents/MeowyLoops/Exports");
    m_tempPathEdit->setText(QDir::tempPath() + "/MeowyLoops");

    QMessageBox::information(this, "Reset", "Settings reset to defaults.");
}