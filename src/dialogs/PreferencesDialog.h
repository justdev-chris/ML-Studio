#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QSlider>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget* parent = nullptr);
    ~PreferencesDialog();

    void loadSettings();
    void saveSettings();

signals:
    void settingsChanged();

private slots:
    void onAudioDeviceChanged(int index);
    void onSampleRateChanged(int value);
    void onBufferSizeChanged(int value);
    void onPluginPathAdded();
    void onPluginPathRemoved();
    void onThemeChanged(int index);
    void onResetDefaults();

private:
    void setupUI();
    void createAudioTab();
    void createPluginTab();
    void createMIDITab();
    void createAppearanceTab();
    void createPathsTab();

    QTabWidget* m_tabWidget;

    // Audio
    QComboBox* m_audioDeviceCombo;
    QComboBox* m_sampleRateCombo;
    QComboBox* m_bufferSizeCombo;
    QSlider* m_latencySlider;
    QLabel* m_latencyLabel;

    // MIDI
    QComboBox* m_midiInputCombo;
    QComboBox* m_midiOutputCombo;

    // Plugin
    QListWidget* m_pluginPathList;
    QPushButton* m_addPathButton;
    QPushButton* m_removePathButton;

    // Appearance
    QComboBox* m_themeCombo;
    QCheckBox* m_darkModeCheck;
    QCheckBox* m_showGridCheck;
    QCheckBox* m_snapToGridCheck;

    // Paths
    QLineEdit* m_projectPathEdit;
    QLineEdit* m_exportPathEdit;
    QLineEdit* m_tempPathEdit;
    QPushButton* m_browseProjectButton;
    QPushButton* m_browseExportButton;
    QPushButton* m_browseTempButton;

    // Buttons
    QPushButton* m_resetButton;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    QPushButton* m_applyButton;
};

#endif // PREFERENCESDIALOG_H