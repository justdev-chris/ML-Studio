#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>
#include <vector>

namespace FileUtils {

    // File operations
    bool exists(const QString& path);
    bool isDirectory(const QString& path);
    bool isFile(const QString& path);
    bool createDirectory(const QString& path);
    bool removeFile(const QString& path);
    bool removeDirectory(const QString& path);
    bool copyFile(const QString& source, const QString& destination);
    bool moveFile(const QString& source, const QString& destination);

    // Reading
    QString readTextFile(const QString& path);
    QByteArray readBinaryFile(const QString& path);
    QStringList readLines(const QString& path);

    // Writing
    bool writeTextFile(const QString& path, const QString& content);
    bool writeBinaryFile(const QString& path, const QByteArray& data);
    bool appendTextFile(const QString& path, const QString& content);

    // Path operations
    QString getFileName(const QString& path);
    QString getBaseName(const QString& path);
    QString getExtension(const QString& path);
    QString getDirectory(const QString& path);
    QString joinPath(const QString& base, const QString& path);
    QString normalizePath(const QString& path);
    QString getAbsolutePath(const QString& path);
    QString getRelativePath(const QString& path, const QString& base);

    // Directory operations
    QStringList listFiles(const QString& path, const QStringList& filters = QStringList());
    QStringList listDirectories(const QString& path);
    QStringList listAllFilesRecursive(const QString& path);
    qint64 getFileSize(const QString& path);
    qint64 getDirectorySize(const QString& path);
    QDateTime getLastModified(const QString& path);

    // Temporary files
    QString createTempFile(const QString& prefix = "tmp", const QString& suffix = ".tmp");
    QString createTempDirectory(const QString& prefix = "tmpdir");
    QString getTempPath();

    // Path validation
    bool isValidPath(const QString& path);
    bool isValidFilename(const QString& filename);
    QString sanitizeFilename(const QString& filename);

    // Audio specific
    QStringList getSupportedAudioExtensions();
    bool isAudioFile(const QString& path);

    // Project specific
    QString getProjectFileExtension();
    bool isProjectFile(const QString& path);
    QString getProjectDirectory(const QString& projectPath);

} // namespace FileUtils

#endif // FILEUTILS_H