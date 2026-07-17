#ifndef MIXERWIDGET_H
#define MIXERWIDGET_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QVector>

class MixerWidget : public QWidget {
    Q_OBJECT

public:
    explicit MixerWidget(QWidget* parent = nullptr);
    ~MixerWidget();

    void setTrackCount(int count);
    void setTrackName(int index, const QString& name);
    void setTrackVolume(int index, float volume);
    void setTrackPan(int index, float pan);
    void setTrackMute(int index, bool muted);
    void setTrackSolo(int index, bool soloed);
    void setMasterVolume(float volume);

signals:
    void trackVolumeChanged(int index, float volume);
    void trackPanChanged(int index, float pan);
    void trackMuteToggled(int index, bool muted);
    void trackSoloToggled(int index, bool soloed);
    void masterVolumeChanged(float volume);
    void trackSelected(int index);

private slots:
    void onVolumeSliderMoved(int value);
    void onPanSliderMoved(int value);
    void onMuteClicked(bool checked);
    void onSoloClicked(bool checked);
    void onMasterVolumeChanged(int value);

private:
    void setupUI();
    void updateTrackLayout();

    struct TrackChannel {
        QLabel* nameLabel;
        QSlider* volumeSlider;
        QLabel* volumeLabel;
        QSlider* panSlider;
        QLabel* panLabel;
        QPushButton* muteButton;
        QPushButton* soloButton;
        QWidget* container;
        int index;
    };

    QVector<TrackChannel> m_trackChannels;
    QWidget* m_trackContainer;
    QVBoxLayout* m_trackLayout;
    QScrollArea* m_scrollArea;

    QSlider* m_masterVolumeSlider;
    QLabel* m_masterVolumeLabel;
    QPushButton* m_masterMuteButton;

    int m_trackCount = 0;
    float m_masterVolume = 1.0f;
};

#endif // MIXERWIDGET_H