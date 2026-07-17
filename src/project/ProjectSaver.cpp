#include "ProjectSaver.h"
#include "Project.h"
#include "core/track/Track.h"
#include "core/track/Clip.h"
#include "utils/FileUtils.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QJsonDocument>

ProjectSaver::ProjectSaver(QObject* parent)
    : QObject(parent) {}

ProjectSaver::~ProjectSaver() {}

bool ProjectSaver::save(const QString& filePath, Project* project) {
    if (!project) {
        m_lastError = "Project pointer is null";
        return false;
    }

    emit statusMessage("Saving project...");
    emit progressUpdated(10);

    // Create project directory if needed
    QDir projectDir = QFileInfo(filePath).absoluteDir();
    if (!projectDir.exists()) {
        if (!projectDir.mkpath(".")) {
            m_lastError = "Could not create project directory: " + projectDir.path();
            return false;
        }
    }

    // Create audio directory
    QString audioDir = projectDir.path() + "/audio";
    if (!QDir(audioDir).exists()) {
        if (!QDir().mkdir(audioDir)) {
            qWarning() << "Could not create audio directory:" << audioDir;
        }
    }

    emit progressUpdated(30);

    // Serialize project to JSON
    QJsonObject root = serializeProject(project);
    emit progressUpdated(50);

    // Write JSON to file
    QJsonDocument doc(root);
    QByteArray data = doc.toJson(QJsonDocument::Indented);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = "Could not open file for writing: " + filePath;
        return false;
    }

    file.write(data);
    file.close();

    // Copy audio files
    emit statusMessage("Copying audio files...");
    emit progressUpdated(70);

    for (Track* track : project->getTracks()) {
        for (int i = 0; i < track->getClipCount(); i++) {
            Clip* clip = track->getClip(i);
            if (clip->getType() == ClipType::Audio) {
                AudioClip* audioClip = dynamic_cast<AudioClip*>(clip);
                if (audioClip) {
                    QString sourcePath = audioClip->getFilePath();
                    if (!sourcePath.isEmpty() && QFile::exists(sourcePath)) {
                        QString destPath = audioDir + "/" + QFileInfo(sourcePath).fileName();
                        if (!QFile::exists(destPath)) {
                            QFile::copy(sourcePath, destPath);
                            qDebug() << "Copied audio file:" << sourcePath << "->" << destPath;
                        }
                    }
                }
            }
        }
    }

    project->setFilePath(filePath);
    project->markClean();
    project->updateModified();

    emit statusMessage("Project saved successfully");
    emit progressUpdated(100);

    return true;
}

QJsonObject ProjectSaver::serializeProject(Project* project) {
    QJsonObject root;

    root["version"] = "1.0.0";

    // Project metadata
    QJsonObject proj;
    proj["name"] = project->getName();
    proj["author"] = project->getAuthor();
    proj["created"] = project->getCreated().toString(Qt::ISODate);
    proj["modified"] = project->getModified().toString(Qt::ISODate);
    root["project"] = proj;

    // Settings
    root["settings"] = serializeSettings(project);

    // Tracks
    root["tracks"] = serializeTracks(project);

    // Automation
    root["automation"] = serializeAutomation(project);

    // Markers
    root["markers"] = serializeMarkers(project);

    // Regions
    root["regions"] = serializeRegions(project);

    return root;
}

QJsonObject ProjectSaver::serializeSettings(Project* project) {
    QJsonObject settings;
    settings["sampleRate"] = project->getSampleRate();
    settings["bitDepth"] = project->getBitDepth();
    settings["bpm"] = project->getTransport()->getTempo();
    settings["timeSignature"] = QJsonArray({project->getTransport()->getBeats(), project->getTransport()->getNoteValue()});
    return settings;
}

QJsonArray ProjectSaver::serializeTracks(Project* project) {
    QJsonArray tracksArray;
    const auto& tracks = project->getTracks();

    for (Track* track : tracks) {
        tracksArray.append(serializeTrack(track));
    }

    return tracksArray;
}

QJsonObject ProjectSaver::serializeTrack(Track* track) {
    QJsonObject trackObj;

    trackObj["name"] = track->getName();
    trackObj["color"] = track->getColor().name();
    trackObj["volume"] = track->getVolume();
    trackObj["pan"] = track->getPan();
    trackObj["mute"] = track->isMuted();
    trackObj["solo"] = track->isSoloed();
    trackObj["recordArm"] = track->isRecordArmed();

    switch (track->getType()) {
        case TrackType::Audio: trackObj["type"] = "audio"; break;
        case TrackType::MIDI: trackObj["type"] = "midi"; break;
        case TrackType::Aux: trackObj["type"] = "aux"; break;
    }

    // Clips
    QJsonArray clipsArray;
    const auto& clips = track->getClips();
    for (Clip* clip : clips) {
        if (clip->getType() == ClipType::Audio) {
            clipsArray.append(serializeAudioClip(clip));
        } else if (clip->getType() == ClipType::MIDI) {
            clipsArray.append(serializeMIDIClip(clip));
        }
    }
    trackObj["clips"] = clipsArray;

    // Inserts (placeholder)
    trackObj["inserts"] = QJsonArray();

    // Sends (placeholder)
    trackObj["sends"] = QJsonArray();

    return trackObj;
}

QJsonObject ProjectSaver::serializeAudioClip(Clip* clip) {
    QJsonObject clipObj;
    clipObj["type"] = "audio";
    clipObj["name"] = clip->getName();
    clipObj["start"] = clip->getStart();
    clipObj["length"] = clip->getLength();

    auto* audioClip = dynamic_cast<AudioClip*>(clip);
    if (audioClip) {
        clipObj["gain"] = audioClip->getGain();
        clipObj["pitch"] = audioClip->getPitch();
        // Store relative path
        QString filePath = audioClip->getFilePath();
        if (!filePath.isEmpty()) {
            QString relativePath = QFileInfo(filePath).fileName();
            clipObj["file"] = "audio/" + relativePath;
        }
    }

    return clipObj;
}

QJsonObject ProjectSaver::serializeMIDIClip(Clip* clip) {
    QJsonObject clipObj;
    clipObj["type"] = "midi";
    clipObj["name"] = clip->getName();
    clipObj["start"] = clip->getStart();
    clipObj["length"] = clip->getLength();

    auto* midiClip = dynamic_cast<MIDIClip*>(clip);
    if (midiClip) {
        QJsonArray eventsArray;
        const auto& events = midiClip->getEvents();
        for (const MIDIEvent& event : events) {
            QJsonObject evt;
            evt["note"] = event.note;
            evt["velocity"] = event.velocity;
            evt["start"] = event.start;
            evt["length"] = event.length;
            evt["channel"] = event.channel;
            eventsArray.append(evt);
        }
        clipObj["events"] = eventsArray;
    }

    return clipObj;
}

QJsonObject ProjectSaver::serializeAutomation(Project* project) {
    QJsonObject automation;
    // TODO: Implement automation serialization
    Q_UNUSED(project);
    return automation;
}

QJsonArray ProjectSaver::serializeMarkers(Project* project) {
    QJsonArray markersArray;
    const auto& markers = project->getMarkers();

    for (const auto& marker : markers) {
        QJsonObject markerObj;
        if (marker.contains("time")) markerObj["time"] = marker["time"].toDouble();
        if (marker.contains("name")) markerObj["name"] = marker["name"].toString();
        if (marker.contains("color")) markerObj["color"] = marker["color"].toString();
        markersArray.append(markerObj);
    }

    return markersArray;
}

QJsonArray ProjectSaver::serializeRegions(Project* project) {
    QJsonArray regionsArray;
    const auto& regions = project->getRegions();

    for (const auto& region : regions) {
        QJsonObject regionObj;
        if (region.contains("start")) regionObj["start"] = region["start"].toDouble();
        if (region.contains("end")) regionObj["end"] = region["end"].toDouble();
        if (region.contains("name")) regionObj["name"] = region["name"].toString();
        if (region.contains("color")) regionObj["color"] = region["color"].toString();
        regionsArray.append(regionObj);
    }

    return regionsArray;
}
