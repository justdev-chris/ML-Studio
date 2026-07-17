#ifndef TIMELINEWIDGET_H
#define TIMELINEWIDGET_H

#include <QWidget>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QVector>
#include <QPoint>
#include <QRect>

struct ClipDisplay {
    int trackIndex;
    int startX;
    int width;
    QString name;
    QColor color;
};

class TimelineWidget : public QWidget {
    Q_OBJECT

public:
    explicit TimelineWidget(QWidget* parent = nullptr);
    ~TimelineWidget();

    void setPlayheadPosition(int x);
    void setZoom(float factor);
    void setTracks(int count);
    void addClip(int trackIndex, int startX, int width, const QString& name, const QColor& color);
    void clearClips();

    int getPlayheadX() const { return playheadX; }
    float getZoom() const { return zoom; }
    int getTrackCount() const { return trackCount; }

signals:
    void playheadMoved(int x);
    void clipClicked(int trackIndex, int clipIndex);
    void clipDragged(int trackIndex, int clipIndex, int deltaX);
    void zoomChanged(float factor);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void drawGrid(QPainter& painter);
    void drawTracks(QPainter& painter);
    void drawClips(QPainter& painter);
    void drawPlayhead(QPainter& painter);
    void drawTrackLabels(QPainter& painter);
    void drawTimeRuler(QPainter& painter);
    void updateScrollArea();

    int trackHeight = 60;
    int trackLabelWidth = 80;
    int rulerHeight = 30;
    int playheadX = 0;
    float zoom = 1.0f;
    int trackCount = 8;
    int viewOffsetX = 0;

    QVector<ClipDisplay> clips;
    QVector<QString> trackNames;

    bool isDragging = false;
    bool isDraggingPlayhead = false;
    int dragStartX = 0;
    int dragStartPlayheadX = 0;
    int selectedClipIndex = -1;
    int selectedTrackIndex = -1;
    int dragClipIndex = -1;
    int dragTrackIndex = -1;

    QScrollBar* hScrollBar;
    QScrollBar* vScrollBar;
};

#endif