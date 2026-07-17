#ifndef TRANSPORTWIDGET_H
#define TRANSPORTWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

class TransportWidget : public QWidget {
    Q_OBJECT

public:
    explicit TransportWidget(QWidget* parent = nullptr);
    ~TransportWidget();

    void setPlaying(bool playing);
    void setRecording(bool recording);
    void setLooping(bool looping);
    void setTempo(int bpm);
    void setTimeSignature(int beats, int noteValue);
    void updateTimeDisplay(int bars, int beats, int ticks);
    void updateProgress(float percentage);

signals:
    void playPressed();
    void stopPressed();
    void recordPressed();
    void loopToggled(bool enabled);
    void tempoChanged(int bpm);
    void timeSignatureChanged(int beats, int noteValue);
    void positionChanged(int frames);

private slots:
    void onPlayClicked();
    void onStopClicked();
    void onRecordClicked();
    void onLoopClicked();
    void onTempoChanged(int value);
    void onTimeSignatureChanged(int index);

private:
    void setupUI();
    void updateButtonStates();

    // Transport buttons
    QPushButton* playButton;
    QPushButton* stopButton;
    QPushButton* recordButton;
    QPushButton* loopButton;
    QPushButton* rewindButton;
    QPushButton* forwardButton;

    // Tempo
    QSpinBox* tempoSpinBox;
    QLabel* tempoLabel;

    // Time signature
    QComboBox* timeSignatureCombo;

    // Position display
    QLabel* positionLabel;
    QLabel* progressLabel;

    // Progress bar (custom or QSlider)
    QSlider* progressSlider;

    // State
    bool isPlaying;
    bool isRecording;
    bool isLooping;
    int currentBPM;
    int currentBeats;
    int currentNoteValue;
    int currentBar;
    int currentBeat;
    int currentTick;
    float currentProgress;
};

#endif