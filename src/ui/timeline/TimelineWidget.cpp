#include "TimelineWidget.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QDebug>

TimelineWidget::TimelineWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumHeight(200);
    setMouseTracking(true);

    hScrollBar = new QScrollBar(Qt::Horizontal, this);
    vScrollBar = new QScrollBar(Qt::Vertical, this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(hScrollBar);

    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(0);
    hLayout->addWidget(vScrollBar);
    layout->addLayout(hLayout);

    connect(hScrollBar, &QScrollBar::valueChanged, this, [this](int value) {
        viewOffsetX = value;
        update();
    });
    connect(vScrollBar, &QScrollBar::valueChanged, this, [this](int) {
        update();
    });

    trackNames = {"Track 1", "Track 2", "Track 3", "Track 4", "Track 5", "Track 6", "Track 7", "Track 8"};
}

TimelineWidget::~TimelineWidget() {}

void TimelineWidget::setPlayheadPosition(int x) {
    playheadX = x;
    emit playheadMoved(x);
    update();
}

void TimelineWidget::setZoom(float factor) {
    zoom = factor;
    updateScrollArea();
    emit zoomChanged(zoom);
    update();
}

void TimelineWidget::setTracks(int count) {
    trackCount = count;
    while (trackNames.size() < trackCount)
        trackNames.append(QString("Track %1").arg(trackNames.size() + 1));
    updateScrollArea();
    update();
}

void TimelineWidget::loadClipsFromProject(Project* project) {
    if (!project) return;
    clips.clear();
    for (int t = 0; t < project->getTrackCount(); t++) {
        Track* track = project->getTrack(t);
        if (!track) continue;
        for (int c = 0; c < track->getClipCount(); c++) {
            Clip* clip = track->getClip(c);
            if (!clip) continue;
            ClipDisplay display;
            display.trackIndex = t;
            display.startX = clip->getStart() / 100;
            display.width = clip->getLength() / 100;
            display.name = clip->getName();
            display.color = clip->getColor();
            clips.append(display);
        }
    }
    update();
}

void TimelineWidget::addClip(int trackIndex, int startX, int width, const QString& name, const QColor& color) {
    ClipDisplay clip;
    clip.trackIndex = trackIndex;
    clip.startX = startX;
    clip.width = width;
    clip.name = name;
    clip.color = color;
    clips.append(clip);
    update();
}

void TimelineWidget::clearClips() {
    clips.clear();
    update();
}

void TimelineWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    int width = this->width();
    int height = this->height();

    painter.fillRect(0, 0, width, height, QColor(30, 30, 35));
    drawTimeRuler(painter);
    drawTracks(painter);
    drawGrid(painter);
    drawClips(painter);
    drawTrackLabels(painter);
    drawPlayhead(painter);
    updateScrollArea();
}

void TimelineWidget::drawGrid(QPainter& painter) {
    int startX = trackLabelWidth;
    int width = this->width() - trackLabelWidth;
    int startY = rulerHeight;
    int height = trackCount * trackHeight;

    int beatWidth = (int)(100 * zoom);
    int totalBeats = (width / beatWidth) + 4;

    painter.setPen(QPen(QColor(50, 50, 55), 1));
    for (int i = 0; i < totalBeats; i++) {
        int x = startX + i * beatWidth - viewOffsetX;
        if (x > startX && x < startX + width)
            painter.drawLine(x, startY, x, startY + height);
    }
    painter.setPen(QPen(QColor(60, 60, 70), 2));
    for (int i = 0; i < totalBeats; i += 4) {
        int x = startX + i * beatWidth - viewOffsetX;
        if (x > startX && x < startX + width)
            painter.drawLine(x, startY, x, startY + height);
    }
    painter.setPen(QPen(QColor(40, 40, 45), 1));
    for (int i = 0; i <= trackCount; i++) {
        int y = startY + i * trackHeight;
        painter.drawLine(startX, y, startX + width, y);
    }
}

void TimelineWidget::drawTracks(QPainter& painter) {
    int startX = trackLabelWidth;
    int width = this->width() - trackLabelWidth;
    int startY = rulerHeight;
    for (int i = 0; i < trackCount; i++) {
        int y = startY + i * trackHeight;
        QColor trackColor = (i % 2 == 0) ? QColor(35, 35, 40) : QColor(40, 40, 45);
        painter.fillRect(startX, y, width, trackHeight, trackColor);
    }
}

void TimelineWidget::drawClips(QPainter& painter) {
    int startX = trackLabelWidth;
    int startY = rulerHeight;
    for (const ClipDisplay& clip : clips) {
        int x = startX + clip.startX - viewOffsetX;
        int y = startY + clip.trackIndex * trackHeight + 2;
        int w = clip.width;
        int h = trackHeight - 4;
        QRect rect(x, y, w, h);
        painter.fillRect(rect, clip.color);
        painter.setPen(QPen(QColor(255, 255, 255, 80), 1));
        painter.drawRect(rect);
        painter.setPen(QPen(QColor(255, 255, 255, 200)));
        painter.setFont(QFont("Segoe UI", 9));
        painter.drawText(rect.adjusted(4, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, clip.name);
    }
}

void TimelineWidget::drawPlayhead(QPainter& painter) {
    int x = trackLabelWidth + playheadX - viewOffsetX;
    int y = rulerHeight;
    int height = trackCount * trackHeight;
    painter.setPen(QPen(QColor(255, 50, 50, 200), 2));
    painter.drawLine(x, y, x, y + height);

    QPolygon triangle;
    triangle << QPoint(x, y) << QPoint(x - 6, y + 10) << QPoint(x + 6, y + 10);
    painter.setBrush(QColor(255, 50, 50, 200));
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(triangle);
}

void TimelineWidget::drawTrackLabels(QPainter& painter) {
    int startY = rulerHeight;
    painter.fillRect(0, startY, trackLabelWidth, trackCount * trackHeight, QColor(25, 25, 30));
    for (int i = 0; i < trackCount && i < trackNames.size(); i++) {
        int y = startY + i * trackHeight;
        QRect rect(4, y, trackLabelWidth - 8, trackHeight);
        painter.setPen(QPen(QColor(200, 200, 200)));
        painter.setFont(QFont("Segoe UI", 8));
        painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, trackNames[i]);
    }
}

void TimelineWidget::drawTimeRuler(QPainter& painter) {
    int startX = trackLabelWidth;
    int width = this->width() - trackLabelWidth;
    painter.fillRect(0, 0, this->width(), rulerHeight, QColor(20, 20, 25));
    painter.setPen(QPen(QColor(100, 100, 110)));
    painter.drawLine(0, rulerHeight, this->width(), rulerHeight);

    int beatWidth = (int)(100 * zoom);
    int totalBeats = (width / beatWidth) + 4;
    painter.setPen(QPen(QColor(150, 150, 160)));
    painter.setFont(QFont("Segoe UI", 8));
    for (int i = 0; i < totalBeats; i++) {
        int x = startX + i * beatWidth - viewOffsetX;
        if (x > startX && x < startX + width)
            painter.drawText(x + 2, 0, 30, rulerHeight, Qt::AlignLeft | Qt::AlignVCenter, QString::number(i + 1));
    }
}

void TimelineWidget::mousePressEvent(QMouseEvent* event) {
    int startX = trackLabelWidth;
    int x = event->pos().x();

    if (x >= startX + playheadX - viewOffsetX - 5 &&
        x <= startX + playheadX - viewOffsetX + 5) {
        isDraggingPlayhead = true;
        dragStartX = x;
        dragStartPlayheadX = playheadX;
        return;
    }

    for (int i = clips.size() - 1; i >= 0; i--) {
        ClipDisplay& clip = clips[i];
        int clipX = startX + clip.startX - viewOffsetX;
        int clipY = rulerHeight + clip.trackIndex * trackHeight + 2;
        int clipW = clip.width;
        int clipH = trackHeight - 4;
        if (x >= clipX && x <= clipX + clipW &&
            event->pos().y() >= clipY && event->pos().y() <= clipY + clipH) {
            selectedClipIndex = i;
            selectedTrackIndex = clip.trackIndex;
            dragClipIndex = i;
            dragTrackIndex = clip.trackIndex;
            dragStartX = x;
            isDragging = true;
            emit clipClicked(clip.trackIndex, i);
            return;
        }
    }

    if (x >= startX) {
        playheadX = x - startX + viewOffsetX;
        emit playheadMoved(playheadX);
        update();
    }
}

void TimelineWidget::mouseMoveEvent(QMouseEvent* event) {
    int x = event->pos().x();
    if (isDraggingPlayhead) {
        int delta = x - dragStartX;
        playheadX = dragStartPlayheadX + delta;
        if (playheadX < 0) playheadX = 0;
        emit playheadMoved(playheadX);
        update();
        return;
    }
    if (isDragging && dragClipIndex >= 0) {
        int delta = x - dragStartX;
        int snap = (int)(10 * zoom);
        int newStart = clips[dragClipIndex].startX + delta;
        newStart = (newStart / snap) * snap;
        clips[dragClipIndex].startX = newStart;
        dragStartX = x;
        emit clipDragged(dragTrackIndex, dragClipIndex, delta);
        update();
    }
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent* event) {
    isDragging = false;
    isDraggingPlayhead = false;
    dragClipIndex = -1;
    dragTrackIndex = -1;
}

void TimelineWidget::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        float delta = event->angleDelta().y() > 0 ? 1.1f : 0.9f;
        setZoom(zoom * delta);
    } else {
        hScrollBar->setValue(hScrollBar->value() - event->angleDelta().x());
    }
}

void TimelineWidget::updateScrollArea() {
    int totalWidth = trackLabelWidth + (int)(100 * zoom * 16);
    int totalHeight = rulerHeight + trackCount * trackHeight;
    int visibleWidth = this->width() - trackLabelWidth;
    int visibleHeight = this->height() - rulerHeight;

    hScrollBar->setRange(0, std::max(0, totalWidth - visibleWidth));
    hScrollBar->setPageStep(visibleWidth);
    hScrollBar->setVisible(totalWidth > visibleWidth);

    vScrollBar->setRange(0, std::max(0, totalHeight - visibleHeight));
    vScrollBar->setPageStep(visibleHeight);
    vScrollBar->setVisible(totalHeight > visibleHeight);
}
