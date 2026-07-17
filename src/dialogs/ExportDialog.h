#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

class ExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExportDialog(QWidget* parent = nullptr);
    ~ExportDialog();

    void setProjectName(const QString& name);
    void setExportPath(const QString& path);

signals:
    void exportRequested(const QString& path, const QString& format, int bitDepth, int sampleRate, bool normalize);

private slots:
    void onBrowseClicked();
    void onExportClicked();
    void onFormatChanged(int index);

private:
    void setupUI();
    void updateProgress(int percent);
    void setProgressVisible(bool visible);

    QLineEdit* m_fileNameEdit;
    QLineEdit* m_pathEdit;
    QPushButton* m_browseButton;

    QComboBox* m_formatCombo;
    QComboBox* m_bitDepthCombo;
    QComboBox* m_sampleRateCombo;

    QCheckBox* m_normalizeCheck;
    QCheckBox* m_stereoCheck;

    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;

    QPushButton* m_exportButton;
    QPushButton* m_cancelButton;

    QString m_projectName;
    QString m_exportPath;
};

#endif // EXPORTDIALOG_H