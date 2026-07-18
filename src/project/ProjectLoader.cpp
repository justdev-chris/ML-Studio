#include "ProjectLoader.h"
#include "Project.h"
#include "core/track/Track.h"
#include "core/track/Clip.h"
#include "utils/FileUtils.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonParseError>

ProjectLoader::ProjectLoader(QObject* parent) : QObject(parent) {}
ProjectLoader::~ProjectLoader() {}

bool ProjectLoader::load(const QString& filePath, Project* project) {
    if (!project) { m_lastError = "Project pointer is null"; return false; }
    if (!QFile::exists(filePath)) { m_lastError = "File does not exist: " + filePath; return false; }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) { m_lastError = "Could not open file: " + filePath; return false; }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) { m_lastError = "JSON parse error: " + parseError.errorString(); return false; }
    if (!doc.isObject()) { m_lastError = "Root is not a JSON object"; return false; }

    QJsonObject root = doc.object();
    if (!root.contains("version")) { m_lastError = "Missing version field"; return false; }
    if (root["version"].toString() != "1.0.0") { m_lastError = "Unsupported project version: " + root["version"].toString(); return false; }

    emit statusMessage("Loading project...");
    emit progressUpdated(10);

    if (!parseProject(root, project)) return false;
    emit progressUpdated(20);

    if (!parseSettings(root, project)) return false;
    emit progressUpdated(30);

    if (root.contains("tracks") && root["tracks"].isArray()) {
        if (!parseTracks(root["tracks"].toArray(), project, filePath)) return false;
    }
    emit progressUpdated(60);

    if (root.contains("automation")) {
        if (!parseAutomation(root["automation"].toObject(), project)) return false;
    }
    emit progressUpdated(80);

    if (root.contains("markers") && root["markers"].isArray()) {
        if (!parseMarkers(root["markers"].toArray(), project)) return false;
    }
    emit progressUpdated(90);

    if (root.contains("regions") && root["regions"].isArray()) {
        if (!parseRegions(root["regions"].toArray(), project)) return false;
    }

    project->setFilePath(filePath);
    project->markClean();
    project->updateModified();
    emit statusMessage("Project loaded successfully");
    emit progressUpdated(100);
    return true;
}

bool ProjectLoader::parseProject(const QJsonObject& json, Project* project) {
    if (json.contains("project") && json["project"].isObject()) {
        QJsonObject proj = json["project"].toObject();
        if (proj.contains("name")) project->setName(proj["name"].toString("Untitled"));
        if (proj.contains("author")) project->setAuthor(proj["author"].toString("Unknown"));
        if (proj.contains("created")) project->setCreated(QDateTime::fromString(proj["created"].toString(), Qt::ISODate));
        if (proj.contains("modified")) project->setModified(QDateTime::fromString(proj["modified"].toString(), Qt::ISODate));
    }
    return true;
}

bool ProjectLoader::parseSettings(const QJsonObject& json, Project* project) {
    if (json.contains("settings") && json["settings"].isObject()) {
        QJsonObject settings = json["settings"].toObject();
        if (settings.contains("sampleRate")) project->setSampleRate(settings["sampleRate"].toInt(44100));
        if (settings.contains("bitDepth")) project->setBitDepth(settings["bitDepth"].toInt(24));
        if (settings.contains("bpm")) project->getTransport()->setTempo(settings["bpm"].toDouble(120.0));
        if (settings.contains("timeSignature")) {
            QJsonArray ts = settings["timeSignature"].toArray();
            if (ts.size() >= 2) {
                project->getTransport()->setTimeSignature(ts[0].toInt(4), ts[1].toInt(4));
            }
        }
    }
    return true;
}

bool ProjectLoader::parseTracks(const QJsonArray& tracks, Project* project, const QString& projectPath) {
    QString projectDir = QFileInfo(projectPath).absolutePath();
    for (int i = 0; i < tracks.size(); i++) {
        if (!tracks[i].isObject()) { m_lastError = "Track " + QString::number(i) + " is not an object"; return false; }
        QJsonObject trackJson = tracks[i].toObject();

        TrackType type = TrackType::Audio;
        if (trackJson.contains("type")) {
            QString typeStr = trackJson["type"].toString("audio");
            if (typeStr == "audio") type = TrackType::Audio;
            else if (typeStr == "midi") type = TrackType::MIDI;
            else if (typeStr == "aux") type = TrackType::Aux;
        }

        Track* track = new Track(type, project->getSampleRate(), project);
        if (!parseTrack(trackJson, track, projectDir)) { delete track; return false; }
        project->addTrack(track);
    }
    return true;
}

bool ProjectLoader::parseTrack(const QJsonObject& trackJson, Track* track, const QString& projectDir) {
    if (trackJson.contains("name")) track->setName(trackJson["name"].toString("Track"));
    if (trackJson.contains("color")) track->setColor(QColor(trackJson["color"].toString("#8080A0")));
    if (trackJson.contains("volume")) track->setVolume(trackJson["volume"].toDouble(0.8));
    if (trackJson.contains("pan")) track->setPan(trackJson["pan"].toDouble(0.0));
    if (trackJson.contains("mute")) track->setMuted(trackJson["mute"].toBool(false));
    if (trackJson.contains("solo")) track->setSoloed(trackJson["solo"].toBool(false));
    if (trackJson.contains("recordArm")) track->setRecordArmed(trackJson["recordArm"].toBool(false));

    // Parse clips
    if (trackJson.contains("clips") && trackJson["clips"].isArray()) {
        QJsonArray clips = trackJson["clips"].toArray();
        for (int i = 0; i < clips.size(); i++) {
            if (!clips[i].isObject()) continue;
            QJsonObject clipJson = clips[i].toObject();
            QString type = clipJson["type"].toString("audio");
            if (type == "audio") {
                auto* clip = new AudioClip(track);
                if (!parseAudioClip(clipJson, clip, projectDir)) { delete clip; return false; }
                track->addClip(clip);
            } else if (type == "midi") {
                auto* clip = new MIDIClip(track);
                if (!parseMIDIClip(clipJson, clip)) { delete clip; return false; }
                track->addClip(clip);
            }
        }
    }

    // Parse inserts
    if (trackJson.contains("inserts") && trackJson["inserts"].isArray()) {
        QJsonArray inserts = trackJson["inserts"].toArray();
        for (const QJsonValue& insertVal : inserts) {
            if (!insertVal.isObject()) continue;
            QJsonObject insertObj = insertVal.toObject();
            // We would create the plugin instance here and add it to the track
            // For now, we'll skip since plugin creation is complex
            // In a full implementation, you'd use PluginHost to create the instance
        }
    }

    return true;
}

bool ProjectLoader::parseAudioClip(const QJsonObject& clipJson, Clip* clip, const QString& projectDir) {
    auto* audioClip = dynamic_cast<AudioClip*>(clip);
    if (!audioClip) return false;

    if (clipJson.contains("name")) audioClip->setName(clipJson["name"].toString("Audio Clip"));
    if (clipJson.contains("start")) audioClip->setStart(clipJson["start"].toInt(0));
    if (clipJson.contains("length")) audioClip->setLength(clipJson["length"].toInt(44100));
    if (clipJson.contains("gain")) audioClip->setGain(clipJson["gain"].toDouble(1.0));
    if (clipJson.contains("pitch")) audioClip->setPitch(clipJson["pitch"].toDouble(1.0));
    if (clipJson.contains("file")) {
        QString filePath = clipJson["file"].toString();
        QString fullPath = QDir(projectDir).absoluteFilePath(filePath);
        if (QFile::exists(fullPath)) {
            audioClip->loadFromFile(fullPath);
        } else {
            qWarning() << "Audio file not found:" << fullPath;
        }
    }
    return true;
}

bool ProjectLoader::parseMIDIClip(const QJsonObject& clipJson, Clip* clip) {
    auto* midiClip = dynamic_cast<MIDIClip*>(clip);
    if (!midiClip) return false;

    if (clipJson.contains("name")) midiClip->setName(clipJson["name"].toString("MIDI Clip"));
    if (clipJson.contains("start")) midiClip->setStart(clipJson["start"].toInt(0));
    if (clipJson.contains("length")) midiClip->setLength(clipJson["length"].toInt(44100));

    if (clipJson.contains("events") && clipJson["events"].isArray()) {
        QJsonArray events = clipJson["events"].toArray();
        for (int i = 0; i < events.size(); i++) {
            if (!events[i].isObject()) continue;
            QJsonObject evt = events[i].toObject();
            MIDIEvent event;
            event.note = evt["note"].toInt(60);
            event.velocity = evt["velocity"].toInt(100);
            event.start = evt["start"].toInt(0);
            event.length = evt["length"].toInt(480);
            event.channel = evt["channel"].toInt(0);
            midiClip->addEvent(event);
        }
    }
    return true;
}

bool ProjectLoader::parseAutomation(const QJsonObject& json, Project* project) {
    for (auto it = json.begin(); it != json.end(); ++it) {
        int trackIndex = it.key().toInt();
        QJsonObject trackAuto = it.value().toObject();
        for (auto paramIt = trackAuto.begin(); paramIt != trackAuto.end(); ++paramIt) {
            QString paramName = paramIt.key();
            QJsonArray points = paramIt.value().toArray();
            for (const QJsonValue& pointVal : points) {
                if (!pointVal.isObject()) continue;
                QJsonObject pointObj = pointVal.toObject();
                double time = pointObj["time"].toDouble(0.0);
                float value = pointObj["value"].toDouble(0.0f);
                project->addAutomationPoint(trackIndex, paramName, time, value);
            }
        }
    }
    return true;
}

bool ProjectLoader::parseMarkers(const QJsonArray& markers, Project* project) {
    for (int i = 0; i < markers.size(); i++) {
        if (!markers[i].isObject()) continue;
        QJsonObject marker = markers[i].toObject();
        double time = marker["time"].toDouble(0);
        QString name = marker["name"].toString("Marker");
        QColor color = QColor(marker["color"].toString("#FFFF00"));
        project->addMarker(time, name, color);
    }
    return true;
}

bool ProjectLoader::parseRegions(const QJsonArray& regions, Project* project) {
    for (int i = 0; i < regions.size(); i++) {
        if (!regions[i].isObject()) continue;
        QJsonObject region = regions[i].toObject();
        double start = region["start"].toDouble(0);
        double end = region["end"].toDouble(4);
        QString name = region["name"].toString("Region");
        QColor color = QColor(region["color"].toString("#00FF00"));
        project->addRegion(start, end, name, color);
    }
    return true;
}
