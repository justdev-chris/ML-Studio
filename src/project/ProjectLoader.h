#ifndef PROJECTLOADER_H
#define PROJECTLOADER_H

#include <QObject>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QColor>

class Project;
class Track;
class Clip;

class ProjectLoader : public QObject {
    Q_OBJECT

public:
    explicit ProjectLoader(QObject* parent = nullptr);
    ~ProjectLoader();

    bool load(const QString& filePath, Project* project);
    QString getLastError() const { return m_lastError; }

signals:
    void progressUpdated(int percent);
    void statusMessage(const QString& message);

private:
    QString m_lastError;

    bool parseProject(const QJsonObject& json, Project* project);
    bool parseSettings(const QJsonObject& json, Project* project);
    bool parseTracks(const QJsonArray& tracks, Project* project);
    bool parseTrack(const QJsonObject& trackJson, Track* track);
    bool parseAudioClip(const QJsonObject& clipJson, Clip* clip);
    bool parseMIDIClip(const QJsonObject& clipJson, Clip* clip);
    bool parseAutomation(const QJsonObject& json, Project* project);
    bool parseMarkers(const QJsonArray& markers, Project* project);
    bool parseRegions(const QJsonArray& regions, Project* project);

    QString resolvePath(const QString& relativePath);
};

#endif // PROJECTLOADER_H