#include "PianoRollWidget.h"

#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QFontMetrics>
#include <QScrollBar>
#include <QLayout>
#include <QDebug>
#include <cmath>

PianoRollWidget::PianoRollWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumHeight(200);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    // Default notes for demo
    QVector<MIDINote> demoNotes;
    for (int i = 0; i < 8; i++) {
        MIDINote note;
        note.note = 60 + i * 2;
        note.velocity = 80 + (i % 3) * 20;
        note.start = i * 480;
        note.length = 480;
        demoNotes.append(note);
    }
    setNotes(demoNotes);

    m_totalHeight = m_visibleNotes * m_keyHeight;
}

PianoRollWidget::~PianoRollWidget() {}

void PianoRollWidget::setNotes(const QVector<MIDINote>& notes) {
    m_notes = notes;
    m_selectedIndices.clear();
    update();
    emit notesChanged(m_notes);
}

void PianoRollWidget::addNote(const MIDINote& note) {
    MIDINote newNote = note;
    if (m_quantize) {
        quantizePosition(newNote.start);
        newNote.length = (newNote.length / m_gridSnap) * m_gridSnap;
        if (newNote.length < m_gridSnap / 2) newNote.length = m_gridSnap;
    }
    m_notes.append(newNote);
    update();
    emit noteAdded(newNote);
    emit notesChanged(m_notes);
}

void PianoRollWidget::removeNote(int index) {
    if (index < 0 || index >= m_notes.size()) return;
    m_notes.removeAt(index);
    update();
    emit noteRemoved(index);
    emit notesChanged(m_notes);
}

void PianoRollWidget::clearNotes() {
    m_notes.clear();
    m_selectedIndices.clear();
    update();
    emit notesChanged(m_notes);
}

QVector<MIDINote> PianoRollWidget::getSelectedNotes() const {
    QVector<MIDINote> selected;
    for (int idx : m_selectedIndices) {
        if (idx >= 0 && idx < m_notes.size()) {
            selected.append(m_notes[idx]);
        }
    }
    return selected;
}

void PianoRollWidget::setZoom(float zoom) {
    if (zoom < 0.1f) zoom = 0.1f;
    if (zoom > 5.0f) zoom = 5.0f;
    m_zoom = zoom;
    update();
}

void PianoRollWidget::setGridSnap(int ticks) {
    if (ticks < 1) ticks = 1;
    m_gridSnap = ticks;
}

void PianoRollWidget::setKey(int rootNote, const QString& scale) {
    m_rootNote = rootNote;
    m_scale = scale;
    update();
}

void PianoRollWidget::setShowVelocity(bool show) {
    m_showVelocity = show;
    update();
}

void PianoRollWidget::setQuantize(bool quantize) {
    m_quantize = quantize;
}

void PianoRollWidget::setCurrentTrack(int trackIndex, int clipIndex) {
    m_currentTrack = trackIndex;
    m_currentClip = clipIndex;
    emit contextChanged(trackIndex, clipIndex);
}

void PianoRollWidget::setPlayhead(int position) {
    m_playhead = position;
    update();
}

void PianoRollWidget::midiNoteOn(int note, int velocity) {
    // Add a note from MIDI input
    MIDINote newNote;
    newNote.note = note;
    newNote.velocity = velocity;
    newNote.start = m_playhead;
    newNote.length = m_gridSnap * 2;
    addNote(newNote);
}

void PianoRollWidget::midiNoteOff(int note) {
    // Find the note and adjust length
    for (int i = m_notes.size() - 1; i >= 0; i--) {
        if (m_notes[i].note == note && m_notes[i].start <= m_playhead) {
            int length = m_playhead - m_notes[i].start;
            if (length > 0) {
                m_notes[i].length = length;
                update();
                emit notesChanged(m_notes);
            }
            break;
        }
    }
}

void PianoRollWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // Background
    painter.fillRect(rect(), QColor(20, 20, 25));

    // Draw keyboard
    drawKeyboard(painter);

    // Draw grid
    drawGrid(painter);

    // Draw notes
    drawNotes(painter);

    // Draw velocity
    if (m_showVelocity) {
        drawVelocity(painter);
    }

    // Draw selection
    drawSelection(painter);

    // Draw playhead
    drawPlayhead(painter);

    // Border
    painter.setPen(QPen(QColor(40, 40, 45), 1));
    painter.drawRect(rect());
}

void PianoRollWidget::drawKeyboard(QPainter& painter) {
    int startX = 0;
    int startY = 0;

    painter.fillRect(0, 0, m_keyWidth, height(), QColor(30, 30, 35));

    for (int note = m_highestNote; note >= m_lowestNote; note--) {
        int y = (m_highestNote - note) * m_keyHeight + m_offsetY;
        if (y < -m_keyHeight || y > height()) continue;

        bool isBlack = (note % 12 == 1 || note % 12 == 3 || note % 12 == 6 || note % 12 == 8 || note % 12 == 10);
        QColor color = isBlack ? QColor(40, 40, 45) : QColor(55, 55, 60);
        if (note == m_hoverNote) {
            color = color.lighter(130);
        }

        painter.fillRect(startX, y, m_keyWidth, m_keyHeight, color);
        painter.setPen(QPen(QColor(60, 60, 65), 1));
        painter.drawRect(startX, y, m_keyWidth, m_keyHeight);

        // Note name
        if (!isBlack) {
            QString noteName = QString("%1%2")
                                   .arg(QChar('C' + (note % 12)))
                                   .arg((note / 12) - 1);
            painter.setPen(QPen(QColor(100, 100, 110)));
            painter.setFont(QFont("Segoe UI", 7));
            painter.drawText(startX + 2, y + m_keyHeight - 4, noteName);
        }
    }
}

void PianoRollWidget::drawGrid(QPainter& painter) {
    int startX = m_keyWidth;
    int width = this->width() - m_keyWidth;
    int startY = 0;

    // Background
    painter.fillRect(startX, 0, width, height(), QColor(25, 25, 30));

    // Vertical grid (beats)
    int beatWidth = static_cast<int>(m_gridSnap * m_zoom);
    int totalBeats = (width / beatWidth) + 4;

    painter.setPen(QPen(QColor(45, 45, 50), 1));
    for (int i = 0; i < totalBeats; i++) {
        int x = startX + i * beatWidth - m_offsetX;
        if (x > startX && x < startX + width) {
            painter.drawLine(x, 0, x, height());
        }
    }

    // Strong beat lines (every 4 beats)
    painter.setPen(QPen(QColor(55, 55, 60), 1));
    for (int i = 0; i < totalBeats; i += 4) {
        int x = startX + i * beatWidth - m_offsetX;
        if (x > startX && x < startX + width) {
            painter.drawLine(x, 0, x, height());
        }
    }

    // Horizontal grid (note lines)
    painter.setPen(QPen(QColor(40, 40, 45), 1));
    for (int note = m_highestNote; note >= m_lowestNote; note--) {
        int y = (m_highestNote - note) * m_keyHeight + m_offsetY;
        if (y < 0 || y > height()) continue;
        painter.drawLine(startX, y, startX + width, y);
    }

    // Beat numbers
    painter.setPen(QPen(QColor(80, 80, 90)));
    painter.setFont(QFont("Segoe UI", 7));
    for (int i = 0; i < totalBeats; i += 4) {
        int x = startX + i * beatWidth - m_offsetX + 2;
        if (x > startX && x < startX + width) {
            painter.drawText(x, 12, QString::number(i / 4 + 1));
        }
    }
}

void PianoRollWidget::drawNotes(QPainter& painter) {
    int startX = m_keyWidth;

    for (int i = 0; i < m_notes.size(); i++) {
        const MIDINote& note = m_notes[i];
        int y = (m_highestNote - note.note) * m_keyHeight + m_offsetY;
        int x = startX + note.start * m_zoom - m_offsetX;
        int width = note.length * m_zoom;

        // Skip if outside visible area
        if (x + width < startX || x > this->width()) continue;
        if (y < -m_keyHeight || y > height()) continue;

        // Note color based on velocity
        int velocity = note.velocity;
        QColor color;
        if (velocity > 100) color = QColor(100, 200, 255);
        else if (velocity > 70) color = QColor(70, 150, 220);
        else if (velocity > 40) color = QColor(50, 100, 180);
        else color = QColor(30, 60, 120);

        // Selected highlight
        if (note.selected) {
            color = color.lighter(150);
            painter.setPen(QPen(QColor(255, 255, 255, 200), 2));
        } else {
            painter.setPen(QPen(QColor(255, 255, 255, 50), 1));
        }

        painter.fillRect(x, y, width, m_keyHeight - 1, color);
        painter.drawRect(x, y, width, m_keyHeight - 1);

        // Note number
        painter.setPen(QPen(QColor(255, 255, 255, 150)));
        painter.setFont(QFont("Segoe UI", 6));
        painter.drawText(x + 2, y + m_keyHeight - 4, QString::number(note.note));
    }
}

void PianoRollWidget::drawVelocity(QPainter& painter) {
    // Draw velocity bars below notes
    int startX = m_keyWidth;

    for (int i = 0; i < m_notes.size(); i++) {
        const MIDINote& note = m_notes[i];
        int y = (m_highestNote - note.note) * m_keyHeight + m_offsetY + m_keyHeight;
        int x = startX + note.start * m_zoom - m_offsetX;
        int width = note.length * m_zoom;

        if (x + width < startX || x > this->width()) continue;

        // Velocity bar (below note)
        int barHeight = static_cast<int>(note.velocity / 127.0f * 6);
        painter.fillRect(x, y, width, barHeight, QColor(100, 200, 255, 100));
    }
}

void PianoRollWidget::drawSelection(QPainter& painter) {
    if (m_mode == ModeSelect && !m_selectionRect.isNull()) {
        painter.setPen(QPen(QColor(100, 150, 255, 150), 1));
        painter.fillRect(m_selectionRect, QColor(100, 150, 255, 30));
    }
}

void PianoRollWidget::drawPlayhead(QPainter& painter) {
    int x = m_keyWidth + m_playhead * m_zoom - m_offsetX;

    painter.setPen(QPen(QColor(255, 50, 50, 200), 2));
    painter.drawLine(x, 0, x, height());

    // Playhead triangle
    QPolygon triangle;
    triangle << QPoint(x, 0) << QPoint(x - 6, 10) << QPoint(x + 6, 10);
    painter.setBrush(QColor(255, 50, 50, 200));
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(triangle);
}

void PianoRollWidget::mousePressEvent(QMouseEvent* event) {
    int startX = m_keyWidth;
    int x = event->pos().x();
    int y = event->pos().y();
    int note = getNoteAtY(y);

    // Check if clicking in note area
    if (x < startX) {
        // Keyboard click - preview note
        if (note >= 0 && note < 128) {
            midiNoteOn(note, 100);
        }
        return;
    }

    // Check if clicking on a note
    int pos = xToPosition(x);
    for (int i = m_notes.size() - 1; i >= 0; i--) {
        const MIDINote& noteData = m_notes[i];
        int noteY = (m_highestNote - noteData.note) * m_keyHeight + m_offsetY;
        int noteX = startX + noteData.start * m_zoom - m_offsetX;
        int noteWidth = noteData.length * m_zoom;

        if (x >= noteX && x <= noteX + noteWidth &&
            y >= noteY && y <= noteY + m_keyHeight) {
            // Select note
            m_editingIndex = i;
            m_editStartPos = noteData.start;
            m_editStartNote = noteData.note;

            if (event->modifiers() & Qt::ControlModifier) {
                // Toggle selection
                m_notes[i].selected = !m_notes[i].selected;
            } else if (!m_notes[i].selected) {
                // Clear selection and select this note
                for (auto& n : m_notes) n.selected = false;
                m_notes[i].selected = true;
            }

            // Check if resizing from edge
            if (x >= noteX + noteWidth - 8) {
                m_mode = ModeResize;
            } else {
                m_mode = ModeMove;
            }

            m_dragStart = event->pos();
            m_dragStartPosition = noteData.start;
            m_dragStartNote = noteData.note;
            update();
            return;
        }
    }

    // Click on empty space - start selection or add note
    if (event->modifiers() & Qt::ShiftModifier) {
        // Select range
        m_mode = ModeSelect;
        m_selectionRect = QRect(event->pos(), event->pos());
    } else if (note >= 0 && note < 128) {
        // Add note at click position
        MIDINote newNote;
        newNote.note = note;
        newNote.velocity = 100;
        newNote.start = pos;
        newNote.length = m_gridSnap;
        if (m_quantize) {
            quantizePosition(newNote.start);
            newNote.length = m_gridSnap;
        }
        addNote(newNote);
        m_mode = ModeDraw;
    }

    update();
}

void PianoRollWidget::mouseMoveEvent(QMouseEvent* event) {
    int x = event->pos().x();
    int y = event->pos().y();
    int note = getNoteAtY(y);

    // Update hover
    m_hoverNote = note;
    setCursor(Qt::ArrowCursor);

    if (m_mode == ModeMove && m_editingIndex >= 0 && m_editingIndex < m_notes.size()) {
        int newPos = xToPosition(x);
        int delta = newPos - m_dragStartPosition;
        int newStart = m_editStartPos + delta;
        if (m_quantize) quantizePosition(newStart);
        if (newStart < 0) newStart = 0;

        int newNote = m_dragStartNote - (y - m_dragStart.y()) / m_keyHeight;
        if (newNote < 0) newNote = 0;
        if (newNote > 127) newNote = 127;

        m_notes[m_editingIndex].start = newStart;
        m_notes[m_editingIndex].note = newNote;
        update();
        emit notesChanged(m_notes);
    }

    if (m_mode == ModeResize && m_editingIndex >= 0 && m_editingIndex < m_notes.size()) {
        int newPos = xToPosition(x);
        int newLength = newPos - m_notes[m_editingIndex].start;
        if (newLength < m_gridSnap / 2) newLength = m_gridSnap / 2;
        if (m_quantize) newLength = (newLength / m_gridSnap) * m_gridSnap;
        if (newLength < 1) newLength = m_gridSnap;
        m_notes[m_editingIndex].length = newLength;
        update();
        emit notesChanged(m_notes);
    }

    if (m_mode == ModeSelect) {
        m_selectionRect.setBottomRight(event->pos());
        update();
    }

    // Update cursor for resize
    if (m_editingIndex >= 0 && m_editingIndex < m_notes.size()) {
        const MIDINote& noteData = m_notes[m_editingIndex];
        int startX = m_keyWidth;
        int noteX = startX + noteData.start * m_zoom - m_offsetX;
        int noteWidth = noteData.length * m_zoom;
        if (x >= noteX + noteWidth - 8 && x <= noteX + noteWidth + 4) {
            setCursor(Qt::SizeHorCursor);
        }
    }
}

void PianoRollWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (m_mode == ModeSelect && !m_selectionRect.isNull()) {
        // Select notes in rectangle
        int startX = m_keyWidth;
        QRect rect = m_selectionRect.normalized();
        for (int i = 0; i < m_notes.size(); i++) {
            const MIDINote& noteData = m_notes[i];
            int noteX = startX + noteData.start * m_zoom - m_offsetX;
            int noteY = (m_highestNote - noteData.note) * m_keyHeight + m_offsetY;
            int noteWidth = noteData.length * m_zoom;
            QRect noteRect(noteX, noteY, noteWidth, m_keyHeight);
            if (rect.intersects(noteRect)) {
                m_notes[i].selected = true;
            }
        }
        emit notesChanged(m_notes);
        m_selectionRect = QRect();
    }

    m_mode = ModeNone;
    m_editingIndex = -1;
    setCursor(Qt::ArrowCursor);
    update();
}

void PianoRollWidget::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom
        float delta = event->angleDelta().y() > 0 ? 1.2f : 0.8f;
        setZoom(m_zoom * delta);
    } else {
        // Scroll
        m_offsetX += event->angleDelta().x() / 4;
        m_offsetY += event->angleDelta().y() / 4;
        if (m_offsetX < 0) m_offsetX = 0;
        if (m_offsetY < 0) m_offsetY = 0;
        update();
    }
}

void PianoRollWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        // Delete selected notes
        for (int i = m_notes.size() - 1; i >= 0; i--) {
            if (m_notes[i].selected) {
                removeNote(i);
            }
        }
    }
    if (event->key() == Qt::Key_A && event->modifiers() & Qt::ControlModifier) {
        // Select all notes
        for (auto& note : m_notes) {
            note.selected = true;
        }
        update();
    }
    QWidget::keyPressEvent(event);
}

int PianoRollWidget::noteToY(int note) const {
    return (m_highestNote - note) * m_keyHeight + m_offsetY;
}

int PianoRollWidget::getNoteAtY(int y) const {
    int note = m_highestNote - (y - m_offsetY) / m_keyHeight;
    if (note < 0) note = 0;
    if (note > 127) note = 127;
    return note;
}

int PianoRollWidget::positionToX(int position) const {
    return m_keyWidth + position * m_zoom - m_offsetX;
}

int PianoRollWidget::xToPosition(int x) const {
    return static_cast<int>((x - m_keyWidth + m_offsetX) / m_zoom);
}

void PianoRollWidget::quantizePosition(int& position) {
    int snap = m_gridSnap;
    position = ((position + snap / 2) / snap) * snap;
    if (position < 0) position = 0;
}