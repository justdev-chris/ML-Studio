#include "PianoRollWidget.h"
#include <QPainter>
#include <QKeyEvent>
#include <QDebug>

PianoRollWidget::PianoRollWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumHeight(200);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

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
    m_playhead = 0;
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
        newNote.start = ((newNote.start + m_gridSnap / 2) / m_gridSnap) * m_gridSnap;
        newNote.length = ((newNote.length + m_gridSnap / 2) / m_gridSnap) * m_gridSnap;
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
        if (idx >= 0 && idx < m_notes.size()) selected.append(m_notes[idx]);
    }
    return selected;
}

void PianoRollWidget::setZoom(float zoom) {
    m_zoom = qBound(0.1f, zoom, 5.0f);
    update();
}

void PianoRollWidget::setGridSnap(int ticks) {
    m_gridSnap = qMax(1, ticks);
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
    MIDINote newNote;
    newNote.note = note;
    newNote.velocity = velocity;
    newNote.start = m_playhead;
    newNote.length = m_gridSnap * 2;
    addNote(newNote);
}

void PianoRollWidget::midiNoteOff(int note) {
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
    painter.fillRect(rect(), QColor(20, 20, 25));
    drawKeyboard(painter);
    drawGrid(painter);
    drawNotes(painter);
    drawPlayhead(painter);
    drawSelection(painter);
    painter.setPen(QPen(QColor(40, 40, 45), 1));
    painter.drawRect(rect());
}

void PianoRollWidget::drawKeyboard(QPainter& painter) {
    painter.fillRect(0, 0, m_keyWidth, height(), QColor(30, 30, 35));
    for (int note = m_highestNote; note >= m_lowestNote; note--) {
        int y = (m_highestNote - note) * m_keyHeight + m_offsetY;
        if (y < -m_keyHeight || y > height()) continue;
        bool isBlack = (note % 12 == 1 || note % 12 == 3 || note % 12 == 6 || note % 12 == 8 || note % 12 == 10);
        QColor color = isBlack ? QColor(40, 40, 45) : QColor(55, 55, 60);
        if (note == m_hoverNote) color = color.lighter(130);
        painter.fillRect(0, y, m_keyWidth, m_keyHeight, color);
        painter.setPen(QPen(QColor(60, 60, 65), 1));
        painter.drawRect(0, y, m_keyWidth, m_keyHeight);
        if (!isBlack) {
            QString noteName = QString("%1%2").arg(QChar('C' + (note % 12))).arg((note / 12) - 1);
            painter.setPen(QPen(QColor(100, 100, 110)));
            painter.setFont(QFont("Segoe UI", 7));
            painter.drawText(2, y + m_keyHeight - 4, noteName);
        }
    }
}

void PianoRollWidget::drawGrid(QPainter& painter) {
    int startX = m_keyWidth;
    int width = this->width() - m_keyWidth;
    painter.fillRect(startX, 0, width, height(), QColor(25, 25, 30));

    int beatWidth = (int)(m_gridSnap * m_zoom);
    int totalBeats = (width / beatWidth) + 4;

    painter.setPen(QPen(QColor(45, 45, 50), 1));
    for (int i = 0; i < totalBeats; i++) {
        int x = startX + i * beatWidth - m_offsetX;
        if (x > startX && x < startX + width) painter.drawLine(x, 0, x, height());
    }
    painter.setPen(QPen(QColor(55, 55, 60), 1));
    for (int i = 0; i < totalBeats; i += 4) {
        int x = startX + i * beatWidth - m_offsetX;
        if (x > startX && x < startX + width) painter.drawLine(x, 0, x, height());
    }
    painter.setPen(QPen(QColor(40, 40, 45), 1));
    for (int note = m_highestNote; note >= m_lowestNote; note--) {
        int y = (m_highestNote - note) * m_keyHeight + m_offsetY;
        if (y < 0 || y > height()) continue;
        painter.drawLine(startX, y, startX + width, y);
    }
    painter.setPen(QPen(QColor(80, 80, 90)));
    painter.setFont(QFont("Segoe UI", 7));
    for (int i = 0; i < totalBeats; i += 4) {
        int x = startX + i * beatWidth - m_offsetX + 2;
        if (x > startX && x < startX + width)
            painter.drawText(x, 12, QString::number(i / 4 + 1));
    }
}

void PianoRollWidget::drawNotes(QPainter& painter) {
    int startX = m_keyWidth;
    for (int i = 0; i < m_notes.size(); i++) {
        const MIDINote& note = m_notes[i];
        int y = (m_highestNote - note.note) * m_keyHeight + m_offsetY;
        int x = startX + note.start * m_zoom - m_offsetX;
        int w = note.length * m_zoom;
        if (x + w < startX || x > this->width()) continue;
        if (y < -m_keyHeight || y > height()) continue;

        int vel = note.velocity;
        QColor color;
        if (vel > 100) color = QColor(100, 200, 255);
        else if (vel > 70) color = QColor(70, 150, 220);
        else if (vel > 40) color = QColor(50, 100, 180);
        else color = QColor(30, 60, 120);

        if (note.selected) {
            color = color.lighter(150);
            painter.setPen(QPen(QColor(255, 255, 255, 200), 2));
        } else {
            painter.setPen(QPen(QColor(255, 255, 255, 50), 1));
        }
        painter.fillRect(x, y, w, m_keyHeight - 1, color);
        painter.drawRect(x, y, w, m_keyHeight - 1);
        painter.setPen(QPen(QColor(255, 255, 255, 150)));
        painter.setFont(QFont("Segoe UI", 6));
        painter.drawText(x + 2, y + m_keyHeight - 4, QString::number(note.note));
    }
}

void PianoRollWidget::drawPlayhead(QPainter& painter) {
    int x = m_keyWidth + m_playhead * m_zoom - m_offsetX;
    painter.setPen(QPen(QColor(255, 50, 50, 200), 2));
    painter.drawLine(x, 0, x, height());

    QPolygon triangle;
    triangle << QPoint(x, 0) << QPoint(x - 6, 10) << QPoint(x + 6, 10);
    painter.setBrush(QColor(255, 50, 50, 200));
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(triangle);
}

void PianoRollWidget::drawSelection(QPainter& painter) {
    if (m_mode == ModeSelect && !m_selectionRect.isNull()) {
        painter.setPen(QPen(QColor(100, 150, 255, 150), 1));
        painter.fillRect(m_selectionRect, QColor(100, 150, 255, 30));
    }
}

void PianoRollWidget::mousePressEvent(QMouseEvent* event) {
    int startX = m_keyWidth;
    int x = event->pos().x();
    int y = event->pos().y();
    int note = getNoteAtY(y);

    if (x < startX) {
        if (note >= 0 && note < 128) midiNoteOn(note, 100);
        return;
    }

    int pos = xToPosition(x);
    for (int i = m_notes.size() - 1; i >= 0; i--) {
        const MIDINote& noteData = m_notes[i];
        int noteY = (m_highestNote - noteData.note) * m_keyHeight + m_offsetY;
        int noteX = startX + noteData.start * m_zoom - m_offsetX;
        int noteW = noteData.length * m_zoom;

        if (x >= noteX && x <= noteX + noteW &&
            y >= noteY && y <= noteY + m_keyHeight) {
            m_editingIndex = i;
            m_editStartPos = noteData.start;
            m_editStartNote = noteData.note;

            if (event->modifiers() & Qt::ControlModifier) {
                m_notes[i].selected = !m_notes[i].selected;
            } else if (!m_notes[i].selected) {
                for (auto& n : m_notes) n.selected = false;
                m_notes[i].selected = true;
            }

            if (x >= noteX + noteW - 8) m_mode = ModeResize;
            else m_mode = ModeMove;

            m_dragStart = event->pos();
            m_dragStartPosition = noteData.start;
            m_dragStartNote = noteData.note;
            update();
            return;
        }
    }

    if (event->modifiers() & Qt::ShiftModifier) {
        m_mode = ModeSelect;
        m_selectionRect = QRect(event->pos(), event->pos());
    } else if (note >= 0 && note < 128) {
        MIDINote newNote;
        newNote.note = note;
        newNote.velocity = 100;
        newNote.start = pos;
        newNote.length = m_gridSnap;
        if (m_quantize) {
            newNote.start = ((newNote.start + m_gridSnap / 2) / m_gridSnap) * m_gridSnap;
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
    m_hoverNote = getNoteAtY(y);
    setCursor(Qt::ArrowCursor);

    if (m_mode == ModeMove && m_editingIndex >= 0 && m_editingIndex < m_notes.size()) {
        int newPos = xToPosition(x);
        int delta = newPos - m_dragStartPosition;
        int newStart = m_editStartPos + delta;
        if (m_quantize) newStart = ((newStart + m_gridSnap / 2) / m_gridSnap) * m_gridSnap;
        if (newStart < 0) newStart = 0;

        int newNote = m_editStartNote - (y - m_dragStart.y()) / m_keyHeight;
        if (newNote < 0) newNote = 0;
        if (newNote > 127) newNote = 127;

        m_notes[m_editingIndex].start = newStart;
        m_notes[m_editingIndex].note = newNote;
        update();
        emit notesChanged(m_notes);
    }

    if (m_mode == ModeResize && m_editingIndex >= 0 && m_editingIndex < m_notes.size()) {
        int newPos = xToPosition(x);
        int newLen = newPos - m_notes[m_editingIndex].start;
        if (newLen < m_gridSnap / 2) newLen = m_gridSnap / 2;
        if (m_quantize) newLen = ((newLen + m_gridSnap / 2) / m_gridSnap) * m_gridSnap;
        if (newLen < 1) newLen = m_gridSnap;
        m_notes[m_editingIndex].length = newLen;
        update();
        emit notesChanged(m_notes);
    }

    if (m_mode == ModeSelect) {
        m_selectionRect.setBottomRight(event->pos());
        update();
    }

    if (m_editingIndex >= 0 && m_editingIndex < m_notes.size()) {
        const MIDINote& noteData = m_notes[m_editingIndex];
        int startX = m_keyWidth;
        int noteX = startX + noteData.start * m_zoom - m_offsetX;
        int noteW = noteData.length * m_zoom;
        if (x >= noteX + noteW - 8 && x <= noteX + noteW + 4)
            setCursor(Qt::SizeHorCursor);
    }
}

void PianoRollWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (m_mode == ModeSelect && !m_selectionRect.isNull()) {
        QRect rect = m_selectionRect.normalized();
        int startX = m_keyWidth;
        for (int i = 0; i < m_notes.size(); i++) {
            const MIDINote& noteData = m_notes[i];
            int noteX = startX + noteData.start * m_zoom - m_offsetX;
            int noteY = (m_highestNote - noteData.note) * m_keyHeight + m_offsetY;
            int noteW = noteData.length * m_zoom;
            QRect noteRect(noteX, noteY, noteW, m_keyHeight);
            if (rect.intersects(noteRect)) m_notes[i].selected = true;
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
        float delta = event->angleDelta().y() > 0 ? 1.2f : 0.8f;
        setZoom(m_zoom * delta);
    } else {
        m_offsetX += event->angleDelta().x() / 4;
        m_offsetY += event->angleDelta().y() / 4;
        if (m_offsetX < 0) m_offsetX = 0;
        if (m_offsetY < 0) m_offsetY = 0;
        update();
    }
}

void PianoRollWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        for (int i = m_notes.size() - 1; i >= 0; i--) {
            if (m_notes[i].selected) removeNote(i);
        }
    }
    if (event->key() == Qt::Key_A && event->modifiers() & Qt::ControlModifier) {
        for (auto& note : m_notes) note.selected = true;
        update();
    }
    QWidget::keyPressEvent(event);
}

int PianoRollWidget::getNoteAtY(int y) const {
    int note = m_highestNote - (y - m_offsetY) / m_keyHeight;
    return qBound(0, note, 127);
}

int PianoRollWidget::xToPosition(int x) const {
    return (int)((x - m_keyWidth + m_offsetX) / m_zoom);
}
