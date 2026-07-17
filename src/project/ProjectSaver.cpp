#include "ProjectSaver.h"
#include "Project.h"
#include "core/track/Track.h"
#include "core/track/Clip.h"
#include "utils/FileUtils.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QJsonDocument>
#include <QCryptographicHash>

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

    // Copy audio files
    emit statusMessage("Copying audio files...");
    for (Track* track : project->getTracks()) {
        for (int i = 0; i < track->getClipCount(); i++) {
            Clip* clip = track->getClip(i);
            if (clip->getType() == ClipType::Audio) {
                AudioClip* audioClip = dynamic_cast<AudioClip*>(clip);
                if (audioClip) {
                    QString sourcePath = audioClip->getFilePath();
                    if (!sourcePath.isEmpty() && QFile::exists(sourcePath)) {
                        if (!copyAudioFile(sourcePath, audioDir)) {
                            qWarning() << "Failed to copy audio file:" << sourcePath;
                        }
                    }
                }
            }
        }
    }

    emit progressUpdated(50);

    // Serialize project to JSON
    QJsonObject root = serializeProject(project);
    emit progressUpdated(70);

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

    // Inserts
    trackObj["inserts"] = serializeInserts(track);

    // Sends
    QJsonArray sendsArray;
    // TODO: Save sends
    trackObj["sends"] = sendsArray;

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

        QString filePath = audioClip->getFilePath();
        if (!filePath.isEmpty()) {
            // Store relative path to audio file
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

    // For each track
    for (int trackIndex = 0; trackIndex < project->getTrackCount(); trackIndex++) {
        QString trackKey = QString::number(trackIndex);
        QJsonObject trackAuto;

        // For each parameter
        // (We need to get automation points from the project)
        // For now, this is a placeholder
        // In a real implementation, we'd iterate over m_automation in Project

        if (!trackAuto.isEmpty()) {
            automation[trackKey] = trackAuto;
        }
    }

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

QJsonArray ProjectSaver::serializeInserts(Track* track) {
    QJsonArray insertsArray;

    // TODO: Get inserts from track and serialize them
    // For now, return empty array

    return insertsArray;
}

QString ProjectSaver::getRelativePath(const QString& absolutePath, const QString& baseDir) {
    return QDir(baseDir).relativeFilePath(absolutePath);
}

bool ProjectSaver::copyAudioFile(const QString& sourcePath, const QString& destDir) {
    QString fileName = QFileInfo(sourcePath).fileName();
    QString destPath = destDir + "/" + fileName;

    // If file already exists, check if it's the same file
    if (QFile::exists(destPath)) {
        // Compare file contents using hash
        QFile src(sourcePath);
        QFile dst(destPath);
        if (src.open(QIODevice::ReadOnly) && dst.open(QIODevice::ReadOnly)) {
            QCryptographicHash srcHash(QCryptographicHash::Sha1);
            QCryptographicHash dstHash(QCryptographicHash::Sha1);
            srcHash.addData(&src);
            dstHash.addData(&dst);
            if (srcHash.result() == dstHash.result()) {
                return true; // Same file, no need to copy
            }
        }
        // Different file with same name — generate unique name
        destPath = generateUniqueFilename(fileName, destDir);
    }

    return QFile::copy(sourcePath, destPath);
}

QString ProjectSaver::generateUniqueFilename(const QString& baseName, const QString& destDir) {
    QString name = QFileInfo(baseName).baseName();
    QString ext = QFileInfo(baseName).suffix();
    QString destPath;

    int counter = 1;
    do {
        QString newName = name + "_" + QString::number(counter) + "." + ext;
        destPath = destDir + "/" + newName;
        counter++;
    } while (QFile::exists(destPath));

    return destPath;
}
