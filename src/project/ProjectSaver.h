#ifndef PROJECTSAVER_H
#define PROJECTSAVER_H

#include <QObject>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>

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

    QString getRelativePath(const QString& absolutePath);
};

#endif // PROJECTSAVER_H