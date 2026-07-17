#include "TransportWidget.h"

#include <QBoxLayout>
#include <QStyle>
#include <QIcon>
#include <QFont>
#include <QString>
#include <QSizePolicy>

TransportWidget::TransportWidget(QWidget* parent)
    : QWidget(parent)
    , isPlaying(false)
    , isRecording(false)
    , isLooping(false)
    , currentBPM(120)
    , currentBeats(4)
    , currentNoteValue(4)
    , currentBar(0)
    , currentBeat(0)
    , currentTick(0)
    , currentProgress(0.0f) {
    setupUI();
    setMinimumHeight(70);
    setMaximumHeight(80);
}

TransportWidget::~TransportWidget() {}

void TransportWidget::setupUI() {
    // Main layout
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 5, 10, 5);
    mainLayout->setSpacing(8);

    // --- Transport buttons ---
    // Rewind
    rewindButton = new QPushButton(this);
    rewindButton->setText("⏪");
    rewindButton->setFixedSize(40, 40);
    rewindButton->setToolTip("Rewind");
    mainLayout->addWidget(rewindButton);

    // Play
    playButton = new QPushButton(this);
    playButton->setText("▶");
    playButton->setFixedSize(50, 45);
    playButton->setToolTip("Play / Pause");
    connect(playButton, &QPushButton::clicked, this, &TransportWidget::onPlayClicked);
    mainLayout->addWidget(playButton);

    // Stop
    stopButton = new QPushButton(this);
    stopButton->setText("■");
    stopButton->setFixedSize(40, 40);
    stopButton->setToolTip("Stop");
    connect(stopButton, &QPushButton::clicked, this, &TransportWidget::onStopClicked);
    mainLayout->addWidget(stopButton);

    // Record
    recordButton = new QPushButton(this);
    recordButton->setText("●");
    recordButton->setFixedSize(40, 40);
    recordButton->setToolTip("Record");
    connect(recordButton, &QPushButton::clicked, this, &TransportWidget::onRecordClicked);
    mainLayout->addWidget(recordButton);

    // Forward
    forwardButton = new QPushButton(this);
    forwardButton->setText("⏩");
    forwardButton->setFixedSize(40, 40);
    forwardButton->setToolTip("Fast Forward");
    mainLayout->addWidget(forwardButton);

    // Loop
    loopButton = new QPushButton(this);
    loopButton->setText("🔁");
    loopButton->setFixedSize(40, 40);
    loopButton->setToolTip("Loop");
    loopButton->setCheckable(true);
    connect(loopButton, &QPushButton::clicked, this, &TransportWidget::onLoopClicked);
    mainLayout->addWidget(loopButton);

    // --- Spacer ---
    mainLayout->addSpacing(15);

    // --- Position display ---
    positionLabel = new QLabel("001.01.000", this);
    positionLabel->setFont(QFont("Segoe UI", 14, QFont::Bold));
    positionLabel->setFixedWidth(130);
    positionLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(positionLabel);

    mainLayout->addSpacing(10);

    // --- Progress slider ---
    progressSlider = new QSlider(Qt::Horizontal, this);
    progressSlider->setRange(0, 1000);
    progressSlider->setValue(0);
    progressSlider->setFixedWidth(180);
    progressSlider->setToolTip("Playhead position");
    mainLayout->addWidget(progressSlider);

    // --- Progress label ---
    progressLabel = new QLabel("0%", this);
    progressLabel->setFont(QFont("Segoe UI", 9));
    progressLabel->setFixedWidth(40);
    progressLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(progressLabel);

    mainLayout->addSpacing(15);

    // --- Tempo ---
    QVBoxLayout* tempoLayout = new QVBoxLayout();
    tempoLayout->setSpacing(1);

    tempoLabel = new QLabel("BPM", this);
    tempoLabel->setFont(QFont("Segoe UI", 8));
    tempoLabel->setAlignment(Qt::AlignCenter);
    tempoLayout->addWidget(tempoLabel);

    tempoSpinBox = new QSpinBox(this);
    tempoSpinBox->setRange(20, 300);
    tempoSpinBox->setValue(currentBPM);
    tempoSpinBox->setFixedWidth(60);
    tempoSpinBox->setFont(QFont("Segoe UI", 10));
    connect(tempoSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &TransportWidget::onTempoChanged);
    tempoLayout->addWidget(tempoSpinBox);

    mainLayout->addLayout(tempoLayout);

    mainLayout->addSpacing(10);

    // --- Time signature ---
    QVBoxLayout* tsLayout = new QVBoxLayout();
    tsLayout->setSpacing(1);

    QLabel* tsLabel = new QLabel("Time Sig", this);
    tsLabel->setFont(QFont("Segoe UI", 8));
    tsLabel->setAlignment(Qt::AlignCenter);
    tsLayout->addWidget(tsLabel);

    timeSignatureCombo = new QComboBox(this);
    timeSignatureCombo->addItems({"4/4", "3/4", "6/8", "5/4", "7/8", "2/4"});
    timeSignatureCombo->setFixedWidth(60);
    timeSignatureCombo->setCurrentIndex(0);
    connect(timeSignatureCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TransportWidget::onTimeSignatureChanged);
    tsLayout->addWidget(timeSignatureCombo);

    mainLayout->addLayout(tsLayout);

    // Update button states
    updateButtonStates();

    // Set overall style
    setStyleSheet(R"(
        QPushButton {
            background-color: #2a2a30;
            border: 1px solid #3a3a40;
            border-radius: 6px;
            color: #ddd;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #3a3a45;
        }
        QPushButton:pressed {
            background-color: #4a4a55;
        }
        QPushButton:checked {
            background-color: #4a6a8a;
            border-color: #6a8aaa;
        }
        QLabel {
            color: #ccc;
        }
        QSpinBox {
            background-color: #1a1a20;
            color: #ddd;
            border: 1px solid #3a3a40;
            border-radius: 4px;
            padding: 2px;
        }
        QSlider::groove:horizontal {
            height: 6px;
            background: #2a2a30;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #8b6eff;
            width: 14px;
            height: 14px;
            margin: -4px 0;
            border-radius: 7px;
        }
        QSlider::sub-page:horizontal {
            background: #6a4aaa;
            border-radius: 3px;
        }
        QComboBox {
            background-color: #1a1a20;
            color: #ddd;
            border: 1px solid #3a3a40;
            border-radius: 4px;
            padding: 2px;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox QAbstractItemView {
            background-color: #1a1a20;
            color: #ddd;
            selection-background-color: #4a4a55;
        }
    )");
}

void TransportWidget::onPlayClicked() {
    isPlaying = !isPlaying;
    updateButtonStates();
    emit playPressed();
}

void TransportWidget::onStopClicked() {
    isPlaying = false;
    isRecording = false;
    updateButtonStates();
    emit stopPressed();
}

void TransportWidget::onRecordClicked() {
    isRecording = !isRecording;
    if (isRecording) {
        isPlaying = true;
    }
    updateButtonStates();
    emit recordPressed();
}

void TransportWidget::onLoopClicked() {
    isLooping = loopButton->isChecked();
    emit loopToggled(isLooping);
    updateButtonStates();
}

void TransportWidget::onTempoChanged(int value) {
    currentBPM = value;
    emit tempoChanged(value);
}

void TransportWidget::onTimeSignatureChanged(int index) {
    QString text = timeSignatureCombo->currentText();
    QStringList parts = text.split('/');
    if (parts.size() == 2) {
        currentBeats = parts[0].toInt();
        currentNoteValue = parts[1].toInt();
        emit timeSignatureChanged(currentBeats, currentNoteValue);
    }
}

void TransportWidget::setPlaying(bool playing) {
    isPlaying = playing;
    updateButtonStates();
}

void TransportWidget::setRecording(bool recording) {
    isRecording = recording;
    updateButtonStates();
}

void TransportWidget::setLooping(bool looping) {
    isLooping = looping;
    loopButton->setChecked(looping);
    updateButtonStates();
}

void TransportWidget::setTempo(int bpm) {
    currentBPM = bpm;
    tempoSpinBox->setValue(bpm);
}

void TransportWidget::setTimeSignature(int beats, int noteValue) {
    currentBeats = beats;
    currentNoteValue = noteValue;
    QString text = QString("%1/%2").arg(beats).arg(noteValue);
    int index = timeSignatureCombo->findText(text);
    if (index >= 0) {
        timeSignatureCombo->setCurrentIndex(index);
    }
}

void TransportWidget::updateTimeDisplay(int bars, int beats, int ticks) {
    currentBar = bars;
    currentBeat = beats;
    currentTick = ticks;
    QString text = QString("%1.%2.%3")
                       .arg(bars, 3, 10, QChar('0'))
                       .arg(beats, 2, 10, QChar('0'))
                       .arg(ticks, 3, 10, QChar('0'));
    positionLabel->setText(text);
}

void TransportWidget::updateProgress(float percentage) {
    currentProgress = percentage;
    int value = static_cast<int>(percentage * 1000);
    progressSlider->setValue(value);
    progressLabel->setText(QString("%1%").arg(static_cast<int>(percentage * 100)));
}

void TransportWidget::updateButtonStates() {
    if (isPlaying) {
        playButton->setText("⏸");
        playButton->setStyleSheet("QPushButton { background-color: #4a6a4a; border-color: #6a8a6a; }");
    } else {
        playButton->setText("▶");
        playButton->setStyleSheet("");
    }

    if (isRecording) {
        recordButton->setStyleSheet("QPushButton { background-color: #8a2a2a; border-color: #aa4a4a; }");
    } else {
        recordButton->setStyleSheet("");
    }

    if (isLooping) {
        loopButton->setStyleSheet("QPushButton:checked { background-color: #4a6a8a; border-color: #6a8aaa; }");
    }
}