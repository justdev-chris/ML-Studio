#ifndef PIANOROLLWIDGET_H
#define PIANOROLLWIDGET_H

#include <QWidget>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QVector>
#include <QRect>
#include <QPoint>
#include <QColor>
#include <QMap>

struct MIDINote {
    int note = 60;      // MIDI note number (0-127)
    int velocity = 100; // 0-127
    int start = 0;      // in ticks
    int length = 480;   // in ticks
    int trackIndex = -1;
    int clipIndex = -1;
    bool selected = false;
};

class PianoRollWidget : public QWidget {
    Q_OBJECT

public:
    explicit PianoRollWidget(QWidget* parent = nullptr);
    ~PianoRollWidget();

    // Note management
    void setNotes(const QVector<MIDINote>& notes);
    void addNote(const MIDINote& note);
    void removeNote(int index);
    void clearNotes();
    QVector<MIDINote> getSelectedNotes() const;

    // Display settings
    void setZoom(float zoom);
    void setGridSnap(int ticks);
    void setKey(int rootNote, const QString& scale = "major");
    void setShowVelocity(bool show);
    void setQuantize(bool quantize);

    // Track context
    void setCurrentTrack(int trackIndex, int clipIndex);
    int getCurrentTrack() const { return m_currentTrack; }
    int getCurrentClip() const { return m_currentClip; }

    // Playhead
    void setPlayhead(int position);
    int getPlayhead() const { return m_playhead; }

    // MIDI input
    void midiNoteOn(int note, int velocity);
    void midiNoteOff(int note);

signals:
    void noteAdded(const MIDINote& note);
    void noteRemoved(int index);
    void noteMoved(int index, int newStart);
    void noteResized(int index, int newLength);
    void noteSelected(const QVector<int>& indices);
    void notesChanged(const QVector<MIDINote>& notes);
    void playheadMoved(int position);
    void contextChanged(int trackIndex, int clipIndex);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void drawGrid(QPainter& painter);
    void drawNotes(QPainter& painter);
    void drawPlayhead(QPainter& painter);
    void drawKeyboard(QPainter& painter);
    void drawVelocity(QPainter& painter);
    void drawSelection(QPainter& painter);

    int noteToY(int note) const;
    int getNoteAtY(int y) const;
    int positionToX(int position) const;
    int xToPosition(int x) const;
    void updateScrollBars();
    void quantizePosition(int& position);

    QVector<MIDINote> m_notes;
    QVector<int> m_selectedIndices;

    // View settings
    float m_zoom = 1.0f;
    int m_gridSnap = 480; // ticks
    int m_rootNote = 60;  // C4
    QString m_scale = "major";
    bool m_showVelocity = true;
    bool m_quantize = true;

    // Note range displayed
    int m_lowestNote = 36;  // C2
    int m_highestNote = 96; // C7
    int m_visibleNotes = 48;

    // Keyboard
    int m_keyWidth = 30;
    int m_keyHeight = 12;

    // Scroll
    int m_offsetX = 0;
    int m_offsetY = 0;
    int m_totalWidth = 10000;
    int m_totalHeight = 0;

    // Playhead
    int m_playhead = 0;

    // Context
    int m_currentTrack = -1;
    int m_currentClip = -1;

    // Interaction state
    enum InteractionMode {
        ModeNone,
        ModeSelect,
        ModeMove,
        ModeResize,
        ModeDraw,
        ModeErase
    };

    InteractionMode m_mode = ModeNone;
    QPoint m_dragStart;
    int m_dragStartPosition = 0;
    int m_dragStartNote = 0;
    QRect m_selectionRect;
    int m_hoverNote = -1;

    // Note being edited
    int m_editingIndex = -1;
    int m_editStartPos = 0;
    int m_editStartNote = 0;
};

#endif // PIANOROLLWIDGET_H