#include "MixerWidget.h"
#include <QScrollBar>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QStyle>

MixerWidget::MixerWidget(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    setMinimumHeight(200);
}

MixerWidget::~MixerWidget() {}

void MixerWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QLabel* title = new QLabel("Mixer", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 14px; font-weight: bold; padding: 8px; background-color: #1a1a20;");
    mainLayout->addWidget(title);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet("background-color: #1a1a20; border: none;");

    m_trackContainer = new QWidget();
    m_trackLayout = new QVBoxLayout(m_trackContainer);
    m_trackLayout->setContentsMargins(4, 4, 4, 4);
    m_trackLayout->setSpacing(2);
    m_trackLayout->addStretch();

    m_scrollArea->setWidget(m_trackContainer);
    mainLayout->addWidget(m_scrollArea);

    // Master section
    QWidget* masterWidget = new QWidget(this);
    masterWidget->setStyleSheet("background-color: #1a1a20; border-top: 1px solid #2a2a30;");
    QHBoxLayout* masterLayout = new QHBoxLayout(masterWidget);
    masterLayout->setContentsMargins(8, 4, 8, 4);
    masterLayout->setSpacing(8);

    QLabel* masterLabel = new QLabel("Master", masterWidget);
    masterLabel->setStyleSheet("font-weight: bold; color: #ddd;");
    masterLayout->addWidget(masterLabel);

    m_masterVolumeSlider = new QSlider(Qt::Horizontal, masterWidget);
    m_masterVolumeSlider->setRange(0, 100);
    m_masterVolumeSlider->setValue(100);
    m_masterVolumeSlider->setFixedWidth(100);
    connect(m_masterVolumeSlider, &QSlider::valueChanged, this, &MixerWidget::onMasterVolumeChanged);
    masterLayout->addWidget(m_masterVolumeSlider);

    m_masterVolumeLabel = new QLabel("100%", masterWidget);
    m_masterVolumeLabel->setFixedWidth(40);
    m_masterVolumeLabel->setAlignment(Qt::AlignCenter);
    m_masterVolumeLabel->setStyleSheet("color: #ddd;");
    masterLayout->addWidget(m_masterVolumeLabel);

    m_masterMuteButton = new QPushButton("M", masterWidget);
    m_masterMuteButton->setFixedSize(24, 24);
    m_masterMuteButton->setCheckable(true);
    m_masterMuteButton->setStyleSheet("QPushButton { background-color: #2a2a30; color: #ddd; border: 1px solid #3a3a40; border-radius: 4px; } QPushButton:checked { background-color: #6a2a2a; border-color: #8a3a3a; }");
    masterLayout->addWidget(m_masterMuteButton);

    masterLayout->addStretch();

    mainLayout->addWidget(masterWidget);

    setStyleSheet(R"(
        QScrollArea { background-color: #1a1a20; border: none; }
        QSlider::groove:horizontal { height: 6px; background: #2a2a30; border-radius: 3px; }
        QSlider::handle:horizontal { background: #8b6eff; width: 14px; height: 14px; margin: -4px 0; border-radius: 7px; }
        QSlider::sub-page:horizontal { background: #6a4aaa; border-radius: 3px; }
        QSlider::groove:vertical { width: 6px; background: #2a2a30; border-radius: 3px; }
        QSlider::handle:vertical { background: #8b6eff; height: 14px; width: 14px; margin: 0 -4px; border-radius: 7px; }
        QSlider::sub-page:vertical { background: #6a4aaa; border-radius: 3px; }
        QCheckBox { color: #ddd; }
        QCheckBox::indicator { width: 16px; height: 16px; }
    )");
}

void MixerWidget::setTrackCount(int count) {
    m_trackCount = count;

    for (auto& channel : m_trackChannels) {
        delete channel.container;
    }
    m_trackChannels.clear();

    while (m_trackLayout->count() > 1) {
        QLayoutItem* item = m_trackLayout->takeAt(0);
        if (item->widget()) delete item->widget();
        delete item;
    }

    if (count == 0) {
        QLabel* emptyLabel = new QLabel("No tracks", m_trackContainer);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #666; font-size: 16px; padding: 40px;");
        m_trackLayout->insertWidget(0, emptyLabel);
        return;
    }

    for (int i = 0; i < count; i++) {
        TrackChannel channel;
        channel.index = i;
        channel.container = new QWidget();
        channel.container->setStyleSheet("background-color: #22222a; border-radius: 4px;");

        QVBoxLayout* layout = new QVBoxLayout(channel.container);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(2);

        channel.nameLabel = new QLabel(QString("Track %1").arg(i + 1));
        channel.nameLabel->setAlignment(Qt::AlignCenter);
        channel.nameLabel->setStyleSheet("color: #ddd; font-weight: bold; font-size: 10px;");
        layout->addWidget(channel.nameLabel);

        channel.volumeSlider = new QSlider(Qt::Vertical);
        channel.volumeSlider->setRange(0, 100);
        channel.volumeSlider->setValue(80);
        channel.volumeSlider->setFixedHeight(80);
        connect(channel.volumeSlider, &QSlider::valueChanged, this, [this, i](int value) {
            emit trackVolumeChanged(i, value / 100.0f);
        });
        layout->addWidget(channel.volumeSlider);

        channel.volumeLabel = new QLabel("80%");
        channel.volumeLabel->setAlignment(Qt::AlignCenter);
        channel.volumeLabel->setStyleSheet("color: #aaa; font-size: 8px;");
        layout->addWidget(channel.volumeLabel);

        channel.panSlider = new QSlider(Qt::Horizontal);
        channel.panSlider->setRange(-100, 100);
        channel.panSlider->setValue(0);
        channel.panSlider->setFixedWidth(60);
        connect(channel.panSlider, &QSlider::valueChanged, this, [this, i](int value) {
            emit trackPanChanged(i, value / 100.0f);
        });
        layout->addWidget(channel.panSlider);

        channel.panLabel = new QLabel("0");
        channel.panLabel->setAlignment(Qt::AlignCenter);
        channel.panLabel->setStyleSheet("color: #aaa; font-size: 8px;");
        layout->addWidget(channel.panLabel);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(2);

        channel.muteButton = new QPushButton("M");
        channel.muteButton->setFixedSize(24, 24);
        channel.muteButton->setCheckable(true);
        channel.muteButton->setStyleSheet("QPushButton { background-color: #2a2a30; color: #ddd; border: 1px solid #3a3a40; border-radius: 4px; font-size: 9px; } QPushButton:checked { background-color: #6a2a2a; border-color: #8a3a3a; }");
        connect(channel.muteButton, &QPushButton::toggled, this, [this, i](bool checked) {
            emit trackMuteToggled(i, checked);
        });
        buttonLayout->addWidget(channel.muteButton);

        channel.soloButton = new QPushButton("S");
        channel.soloButton->setFixedSize(24, 24);
        channel.soloButton->setCheckable(true);
        channel.soloButton->setStyleSheet("QPushButton { background-color: #2a2a30; color: #ddd; border: 1px solid #3a3a40; border-radius: 4px; font-size: 9px; } QPushButton:checked { background-color: #2a6a2a; border-color: #3a8a3a; }");
        connect(channel.soloButton, &QPushButton::toggled, this, [this, i](bool checked) {
            emit trackSoloToggled(i, checked);
        });
        buttonLayout->addWidget(channel.soloButton);

        layout->addLayout(buttonLayout);

        m_trackChannels.append(channel);
        m_trackLayout->insertWidget(m_trackLayout->count() - 1, channel.container);
    }
}

void MixerWidget::setTrackName(int index, const QString& name) {
    if (index < 0 || index >= m_trackChannels.size()) return;
    m_trackChannels[index].nameLabel->setText(name);
}

void MixerWidget::setTrackVolume(int index, float volume) {
    if (index < 0 || index >= m_trackChannels.size()) return;
    int value = static_cast<int>(volume * 100);
    m_trackChannels[index].volumeSlider->setValue(value);
    m_trackChannels[index].volumeLabel->setText(QString("%1%").arg(value));
}

void MixerWidget::setTrackPan(int index, float pan) {
    if (index < 0 || index >= m_trackChannels.size()) return;
    int value = static_cast<int>(pan * 100);
    m_trackChannels[index].panSlider->setValue(value);
    m_trackChannels[index].panLabel->setText(QString::number(value));
}

void MixerWidget::setTrackMute(int index, bool muted) {
    if (index < 0 || index >= m_trackChannels.size()) return;
    m_trackChannels[index].muteButton->setChecked(muted);
}

void MixerWidget::setTrackSolo(int index, bool soloed) {
    if (index < 0 || index >= m_trackChannels.size()) return;
    m_trackChannels[index].soloButton->setChecked(soloed);
}

void MixerWidget::setMasterVolume(float volume) {
    m_masterVolume = volume;
    int value = static_cast<int>(volume * 100);
    m_masterVolumeSlider->setValue(value);
    m_masterVolumeLabel->setText(QString("%1%").arg(value));
}

void MixerWidget::onMasterVolumeChanged(int value) {
    float volume = value / 100.0f;
    m_masterVolume = volume;
    m_masterVolumeLabel->setText(QString("%1%").arg(value));
    emit masterVolumeChanged(volume);
}
