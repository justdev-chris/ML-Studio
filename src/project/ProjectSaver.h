#ifndef PROJECTSAVER_H
#define PROJECTSAVER_H

#include <QObject>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class Project;
class Track;
class Clip;

class ProjectSaver : public QObject {
    Q_OBJECT

public:
    explicit ProjectSaver(QObject* parent = nullptr);
    ~ProjectSaver();

    bool save(const QString& filePath, Project* project);
    QString getLastError() const { return m_lastError; }

signals:
    void progressUpdated(int percent);
    void statusMessage(const QString& message);

private:
    QString m_lastError;

    QJsonObject serializeProject(Project* project);
    QJsonObject serializeSettings(Project* project);
    QJsonArray serializeTracks(Project* project);
    QJsonObject serializeTrack(Track* track);
    QJsonObject serializeAudioClip(Clip* clip);
    QJsonObject serializeMIDIClip(Clip* clip);
    QJsonObject serializeAutomation(Project* project);
    QJsonArray serializeMarkers(Project* project);
    QJsonArray serializeRegions(Project* project);
    QJsonArray serializeInserts(Track* track);

    QString getRelativePath(const QString& absolutePath, const QString& baseDir);
    bool copyAudioFile(const QString& sourcePath, const QString& destDir);
    QString generateUniqueFilename(const QString& baseName, const QString& destDir);
};

#endif // PROJECTSAVER_H
