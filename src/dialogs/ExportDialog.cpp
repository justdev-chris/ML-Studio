#include "ExportDialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>

ExportDialog::ExportDialog(QWidget* parent)
    : QDialog(parent) {
    setupUI();
    setWindowTitle("Export Audio");
    resize(500, 350);
}

ExportDialog::~ExportDialog() {}

void ExportDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // File section
    QGroupBox* fileGroup = new QGroupBox("File", this);
    QGridLayout* fileLayout = new QGridLayout(fileGroup);

    QLabel* nameLabel = new QLabel("File Name:", fileGroup);
    m_fileNameEdit = new QLineEdit(fileGroup);
    m_fileNameEdit->setText("My Song");
    fileLayout->addWidget(nameLabel, 0, 0);
    fileLayout->addWidget(m_fileNameEdit, 0, 1, 1, 2);

    QLabel* pathLabel = new QLabel("Location:", fileGroup);
    m_pathEdit = new QLineEdit(fileGroup);
    m_pathEdit->setText(QDir::homePath() + "/Documents/MeowyLoops/Exports");
    m_pathEdit->setReadOnly(true);
    m_browseButton = new QPushButton("Browse", fileGroup);
    connect(m_browseButton, &QPushButton::clicked, this, &ExportDialog::onBrowseClicked);

    fileLayout->addWidget(pathLabel, 1, 0);
    fileLayout->addWidget(m_pathEdit, 1, 1);
    fileLayout->addWidget(m_browseButton, 1, 2);

    mainLayout->addWidget(fileGroup);

    // Format section
    QGroupBox* formatGroup = new QGroupBox("Audio Format", this);
    QGridLayout* formatLayout = new QGridLayout(formatGroup);

    QLabel* formatLabel = new QLabel("Format:", formatGroup);
    m_formatCombo = new QComboBox(formatGroup);
    m_formatCombo->addItems({"WAV", "AIFF", "FLAC", "MP3", "OGG"});
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ExportDialog::onFormatChanged);

    QLabel* bitDepthLabel = new QLabel("Bit Depth:", formatGroup);
    m_bitDepthCombo = new QComboBox(formatGroup);
    m_bitDepthCombo->addItems({"16-bit", "24-bit", "32-bit float"});

    QLabel* sampleRateLabel = new QLabel("Sample Rate:", formatGroup);
    m_sampleRateCombo = new QComboBox(formatGroup);
    m_sampleRateCombo->addItems({"44100", "48000", "88200", "96000", "192000"});

    formatLayout->addWidget(formatLabel, 0, 0);
    formatLayout->addWidget(m_formatCombo, 0, 1);
    formatLayout->addWidget(bitDepthLabel, 1, 0);
    formatLayout->addWidget(m_bitDepthCombo, 1, 1);
    formatLayout->addWidget(sampleRateLabel, 2, 0);
    formatLayout->addWidget(m_sampleRateCombo, 2, 1);

    m_normalizeCheck = new QCheckBox("Normalize", formatGroup);
    m_normalizeCheck->setChecked(true);
    m_stereoCheck = new QCheckBox("Stereo (2-channel)", formatGroup);
    m_stereoCheck->setChecked(true);

    formatLayout->addWidget(m_normalizeCheck, 3, 0);
    formatLayout->addWidget(m_stereoCheck, 3, 1);

    mainLayout->addWidget(formatGroup);

    // Progress
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    m_statusLabel = new QLabel("", this);
    m_statusLabel->setStyleSheet("color: #888;");
    mainLayout->addWidget(m_statusLabel);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_exportButton = new QPushButton("Export", this);
    connect(m_exportButton, &QPushButton::clicked, this, &ExportDialog::onExportClicked);
    buttonLayout->addWidget(m_exportButton);

    m_cancelButton = new QPushButton("Cancel", this);
    connect(m_cancelButton, &QPushButton::clicked, this, &ExportDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    onFormatChanged(0);
}

void ExportDialog::setProjectName(const QString& name) {
    m_projectName = name;
    m_fileNameEdit->setText(name);
}

void ExportDialog::setExportPath(const QString& path) {
    m_exportPath = path;
    m_pathEdit->setText(path);
}

void ExportDialog::onBrowseClicked() {
    QString path = QFileDialog::getExistingDirectory(this, "Select Export Folder");
    if (!path.isEmpty()) {
        m_pathEdit->setText(path);
    }
}

void ExportDialog::onExportClicked() {
    QString fileName = m_fileNameEdit->text().trimmed();
    if (fileName.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please enter a file name.");
        return;
    }

    QString format = m_formatCombo->currentText().toLower();
    QString fullPath = m_pathEdit->text() + "/" + fileName + "." + format;

    int bitDepth;
    QString bitDepthStr = m_bitDepthCombo->currentText();
    if (bitDepthStr == "16-bit") bitDepth = 16;
    else if (bitDepthStr == "24-bit") bitDepth = 24;
    else bitDepth = 32;

    int sampleRate = m_sampleRateCombo->currentText().toInt();
    bool normalize = m_normalizeCheck->isChecked();

    setProgressVisible(true);
    m_statusLabel->setText("Exporting...");

    QTimer::singleShot(100, this, [this]() {
        for (int i = 0; i <= 100; i += 5) {
            m_progressBar->setValue(i);
            QCoreApplication::processEvents();
        }
        m_statusLabel->setText("Export complete!");
        QMessageBox::information(this, "Export", "Export completed successfully!");
        setProgressVisible(false);
        accept();
    });

    emit exportRequested(fullPath, format, bitDepth, sampleRate, normalize);
}

void ExportDialog::onFormatChanged(int index) {
    QString format = m_formatCombo->currentText().toLower();
    bool lossless = (format == "wav" || format == "aiff" || format == "flac");
    m_bitDepthCombo->setEnabled(lossless);
    if (!lossless) {
        m_bitDepthCombo->setCurrentIndex(0);
    }
}

void ExportDialog::setProgressVisible(bool visible) {
    m_progressBar->setVisible(visible);
    m_statusLabel->setVisible(visible);
    m_exportButton->setEnabled(!visible);
}
