#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVector>
#include <QMap>

#include "core/track/Track.h"
#include "core/transport/Transport.h"

class Project : public QObject {
    Q_OBJECT

public:
    explicit Project(QObject* parent = nullptr);
    ~Project();

    // Basic info
    QString getName() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    QString getAuthor() const { return m_author; }
    void setAuthor(const QString& author) { m_author = author; }

    QString getFilePath() const { return m_filePath; }
    void setFilePath(const QString& path) { m_filePath = path; }

    QDateTime getCreated() const { return m_created; }
    QDateTime getModified() const { return m_modified; }
    void updateModified() { m_modified = QDateTime::currentDateTime(); }

    // Settings
    int getSampleRate() const { return m_sampleRate; }
    void setSampleRate(int sampleRate) { m_sampleRate = sampleRate; }

    int getBitDepth() const { return m_bitDepth; }
    void setBitDepth(int bitDepth) { m_bitDepth = bitDepth; }

    // Transport
    Transport* getTransport() { return m_transport; }
    const Transport* getTransport() const { return m_transport; }

    // Tracks
    void addTrack(Track* track);
    void removeTrack(int index);
    void insertTrack(int index, Track* track);
    void moveTrack(int from, int to);
    Track* getTrack(int index) const;
    int getTrackCount() const { return m_tracks.size(); }
    const QVector<Track*>& getTracks() const { return m_tracks; }
    void clearTracks();

    // Master
    void setMasterVolume(float volume);
    float getMasterVolume() const { return m_masterVolume; }
    void setMasterInsert(int index, const QString& plugin, const QMap<QString, float>& params);
    void removeMasterInsert(int index);
    const QVector<QMap<QString, QVariant>>& getMasterInserts() const { return m_masterInserts; }

    // Automation
    void addAutomationPoint(int trackIndex, const QString& parameter, double time, float value);
    void removeAutomationPoints(int trackIndex, const QString& parameter);
    QMap<double, float> getAutomationPoints(int trackIndex, const QString& parameter) const;

    // Markers
    void addMarker(double time, const QString& name, const QColor& color);
    void removeMarker(int index);
    void clearMarkers();
    const QVector<QMap<QString, QVariant>>& getMarkers() const { return m_markers; }

    // Regions
    void addRegion(double start, double end, const QString& name, const QColor& color);
    void removeRegion(int index);
    void clearRegions();
    const QVector<QMap<QString, QVariant>>& getRegions() const { return m_regions; }

    // Dirty state
    bool isDirty() const { return m_dirty; }
    void setDirty(bool dirty) { m_dirty = dirty; }
    void markClean() { m_dirty = false; }

signals:
    void nameChanged(const QString& name);
    void authorChanged(const QString& author);
    void trackAdded(Track* track);
    void trackRemoved(int index);
    void trackMoved(int from, int to);
    void masterVolumeChanged(float volume);
    void markerAdded(const QMap<QString, QVariant>& marker);
    void markerRemoved(int index);
    void regionAdded(const QMap<QString, QVariant>& region);
    void regionRemoved(int index);
    void dirtyChanged(bool dirty);
    void projectSaved(const QString& path);
    void projectLoaded(const QString& path);

private:
    QString m_name;
    QString m_author;
    QString m_filePath;
    QDateTime m_created;
    QDateTime m_modified;

    int m_sampleRate = 44100;
    int m_bitDepth = 24;

    Transport* m_transport;
    QVector<Track*> m_tracks;

    float m_masterVolume = 1.0f;
    QVector<QMap<QString, QVariant>> m_masterInserts;

    QMap<int, QMap<QString, QMap<double, float>>> m_automation;

    QVector<QMap<QString, QVariant>> m_markers;
    QVector<QMap<QString, QVariant>> m_regions;

    bool m_dirty = false;
};

#endif // PROJECT_H