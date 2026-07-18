#include "Project.h"
#include <QDebug>

Project::Project(QObject* parent)
    : QObject(parent)
    , m_name("Untitled")
    , m_author("Unknown")
    , m_created(QDateTime::currentDateTime())
    , m_modified(QDateTime::currentDateTime())
    , m_transport(new Transport(this)) {}

Project::~Project() { clearTracks(); }

void Project::addTrack(Track* track) {
    if (!track) return;
    m_tracks.append(track);
    emit trackAdded(track);
    setDirty(true);
}

void Project::removeTrack(int index) {
    if (index < 0 || index >= m_tracks.size()) return;
    Track* track = m_tracks.at(index);
    m_tracks.removeAt(index);
    delete track;
    emit trackRemoved(index);
    setDirty(true);
}

void Project::insertTrack(int index, Track* track) {
    if (!track) return;
    m_tracks.insert(index, track);
    emit trackAdded(track);
    setDirty(true);
}

void Project::moveTrack(int from, int to) {
    if (from == to) return;
    m_tracks.move(from, to);
    emit trackMoved(from, to);
    setDirty(true);
}

Track* Project::getTrack(int index) const {
    if (index < 0 || index >= m_tracks.size()) return nullptr;
    return m_tracks.at(index);
}

void Project::clearTracks() {
    for (Track* track : m_tracks) delete track;
    m_tracks.clear();
}

void Project::setMasterVolume(float volume) {
    m_masterVolume = qBound(0.0f, volume, 2.0f);
    emit masterVolumeChanged(m_masterVolume);
    setDirty(true);
}

void Project::addAutomationPoint(int trackIndex, const QString& parameter, double time, float value) {
    if (time < 0) time = 0;
    m_automation[trackIndex][parameter][time] = value;
    setDirty(true);
}

void Project::removeAutomationPoints(int trackIndex, const QString& parameter) {
    m_automation[trackIndex].remove(parameter);
    setDirty(true);
}

QMap<double, float> Project::getAutomationPoints(int trackIndex, const QString& parameter) const {
    if (!m_automation.contains(trackIndex)) return QMap<double, float>();
    if (!m_automation[trackIndex].contains(parameter)) return QMap<double, float>();
    return m_automation[trackIndex][parameter];
}

void Project::addMarker(double time, const QString& name, const QColor& color) {
    QMap<QString, QVariant> marker;
    marker["time"] = time;
    marker["name"] = name;
    marker["color"] = color.name();
    m_markers.append(marker);
    emit markerAdded(marker);
    setDirty(true);
}

void Project::removeMarker(int index) {
    if (index < 0 || index >= m_markers.size()) return;
    m_markers.removeAt(index);
    emit markerRemoved(index);
    setDirty(true);
}

void Project::clearMarkers() {
    m_markers.clear();
    setDirty(true);
}

void Project::addRegion(double start, double end, const QString& name, const QColor& color) {
    if (start < 0) start = 0;
    if (end <= start) end = start + 1.0;
    QMap<QString, QVariant> region;
    region["start"] = start;
    region["end"] = end;
    region["name"] = name;
    region["color"] = color.name();
    m_regions.append(region);
    emit regionAdded(region);
    setDirty(true);
}

void Project::removeRegion(int index) {
    if (index < 0 || index >= m_regions.size()) return;
    m_regions.removeAt(index);
    emit regionRemoved(index);
    setDirty(true);
}

void Project::clearRegions() {
    m_regions.clear();
    setDirty(true);
}
