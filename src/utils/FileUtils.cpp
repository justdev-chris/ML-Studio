#include "FileUtils.h"
#include <QFileInfo>
#include <QDir>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QStandardPaths>

namespace FileUtils {

    // --- Basic file operations ---

    bool exists(const QString& path) {
        return QFileInfo::exists(path);
    }

    bool isDirectory(const QString& path) {
        return QFileInfo(path).isDir();
    }

    bool isFile(const QString& path) {
        return QFileInfo(path).isFile();
    }

    bool createDirectory(const QString& path) {
        if (exists(path)) return isDirectory(path);
        return QDir().mkpath(path);
    }

    bool removeFile(const QString& path) {
        if (!exists(path) || isDirectory(path)) return false;
        return QFile::remove(path);
    }

    bool removeDirectory(const QString& path) {
        if (!exists(path) || !isDirectory(path)) return false;
        QDir dir(path);
        return dir.removeRecursively();
    }

    bool copyFile(const QString& source, const QString& destination) {
        if (!exists(source) || isDirectory(source)) return false;
        if (exists(destination)) {
            if (isDirectory(destination)) {
                // Copy into directory
                QString destPath = joinPath(destination, getFileName(source));
                return QFile::copy(source, destPath);
            }
            return false;
        }
        return QFile::copy(source, destination);
    }

    bool moveFile(const QString& source, const QString& destination) {
        if (!exists(source) || isDirectory(source)) return false;
        if (exists(destination)) return false;
        return QFile::rename(source, destination);
    }

    // --- Reading ---

    QString readTextFile(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Failed to open file:" << path;
            return QString();
        }
        return QString::fromUtf8(file.readAll());
    }

    QByteArray readBinaryFile(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to open file:" << path;
            return QByteArray();
        }
        return file.readAll();
    }

    QStringList readLines(const QString& path) {
        QString content = readTextFile(path);
        if (content.isEmpty()) return QStringList();
        return content.split('\n', Qt::SkipEmptyParts);
    }

    // --- Writing ---

    bool writeTextFile(const QString& path, const QString& content) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "Failed to write file:" << path;
            return false;
        }
        file.write(content.toUtf8());
        file.close();
        return true;
    }

    bool writeBinaryFile(const QString& path, const QByteArray& data) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << "Failed to write file:" << path;
            return false;
        }
        file.write(data);
        file.close();
        return true;
    }

    bool appendTextFile(const QString& path, const QString& content) {
        QFile file(path);
        if (!file.open(QIODevice::Append | QIODevice::Text)) {
            qWarning() << "Failed to append file:" << path;
            return false;
        }
        file.write(content.toUtf8());
        file.close();
        return true;
    }

    // --- Path operations ---

    QString getFileName(const QString& path) {
        return QFileInfo(path).fileName();
    }

    QString getBaseName(const QString& path) {
        return QFileInfo(path).baseName();
    }

    QString getExtension(const QString& path) {
        return QFileInfo(path).suffix();
    }

    QString getDirectory(const QString& path) {
        return QFileInfo(path).absolutePath();
    }

    QString joinPath(const QString& base, const QString& path) {
        return QDir(base).filePath(path);
    }

    QString normalizePath(const QString& path) {
        return QDir::cleanPath(path);
    }

    QString getAbsolutePath(const QString& path) {
        return QFileInfo(path).absoluteFilePath();
    }

    QString getRelativePath(const QString& path, const QString& base) {
        return QDir(base).relativeFilePath(path);
    }

    // --- Directory operations ---

    QStringList listFiles(const QString& path, const QStringList& filters) {
        QDir dir(path);
        if (!dir.exists()) return QStringList();
        QDir::Filters filterFlags = QDir::Files | QDir::NoDotAndDotDot;
        if (!filters.isEmpty()) {
            return dir.entryList(filters, filterFlags);
        }
        return dir.entryList(filterFlags);
    }

    QStringList listDirectories(const QString& path) {
        QDir dir(path);
        if (!dir.exists()) return QStringList();
        return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    }

    QStringList listAllFilesRecursive(const QString& path) {
        QStringList files;
        QDir dir(path);
        if (!dir.exists()) return files;

        QDirIterator it(path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            files.append(it.next());
        }
        return files;
    }

    qint64 getFileSize(const QString& path) {
        return QFileInfo(path).size();
    }

    qint64 getDirectorySize(const QString& path) {
        qint64 size = 0;
        QDir dir(path);
        if (!dir.exists()) return size;

        QDirIterator it(path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            size += it.fileInfo().size();
        }
        return size;
    }

    QDateTime getLastModified(const QString& path) {
        return QFileInfo(path).lastModified();
    }

    // --- Temporary files ---

    QString createTempFile(const QString& prefix, const QString& suffix) {
        QTemporaryFile tempFile(getTempPath() + "/" + prefix + "XXXXXX" + suffix);
        if (!tempFile.open()) return QString();
        return tempFile.fileName();
    }

    QString createTempDirectory(const QString& prefix) {
        QTemporaryDir tempDir(getTempPath() + "/" + prefix + "XXXXXX");
        if (!tempDir.isValid()) return QString();
        return tempDir.path();
    }

    QString getTempPath() {
        return QDir::tempPath();
    }

    // --- Path validation ---

    bool isValidPath(const QString& path) {
        QFileInfo info(path);
        return info.exists();
    }

    bool isValidFilename(const QString& filename) {
        return !filename.isEmpty() && !filename.contains(QRegularExpression(R"([<>:"/\\|?*])"));
    }

    QString sanitizeFilename(const QString& filename) {
        static QRegularExpression invalidChars(R"([<>:"/\\|?*])");
        QString clean = filename;
        clean.replace(invalidChars, "_");
        return clean;
    }

    // --- Audio specific ---

    QStringList getSupportedAudioExtensions() {
        return {
            "wav", "aiff", "flac", "mp3", "ogg", "m4a", "aac",
            "wma", "opus", "alac", "pcm", "aif", "snd"
        };
    }

    bool isAudioFile(const QString& path) {
        QString ext = getExtension(path).toLower();
        return getSupportedAudioExtensions().contains(ext);
    }

    // --- Project specific ---

    QString getProjectFileExtension() {
        return "mlproj";
    }

    bool isProjectFile(const QString& path) {
        return getExtension(path) == getProjectFileExtension();
    }

    QString getProjectDirectory(const QString& projectPath) {
        return getDirectory(projectPath);
    }

} // namespace FileUtils