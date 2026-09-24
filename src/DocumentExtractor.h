#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QAtomicInt>

class QTextStream;
class QAxObject;

enum class ScanMode
{
    Folder,
    SingleFile
};

class DocumentExtractorWorker : public QObject
{
    Q_OBJECT

public:
    explicit DocumentExtractorWorker(
        QObject *parent = nullptr);

public slots:

    void process(
        const QString &scanPath,
        const QString &searchText,
        const QString &outputFilePath,
        bool scanWord,
        bool scanText,
        bool scanPdf,
        bool includeSubfolders,
        ScanMode scanMode);

    void cancel();

signals:

    void progressChanged(
        int value);

    void progressInfoChanged(
        int currentFile,
        int totalFiles,
        int matchesFound);

    void statusChanged(
        const QString &status);

    void logMessage(
        const QString &message);

    void finished(
        int filesFound,
        int filesScanned,
        int matchesFound,
        int emptyFiles,
        int failedFiles,
        const QStringList &failedFilePaths);

    void cancelled(
        int filesFound,
        int filesScanned,
        int matchesFound,
        int emptyFiles,
        int failedFiles,
        const QStringList &failedFilePaths);

    void errorOccurred(
        const QString &errorMessage);

private:

    bool processTextFile(
        const QString &filePath,
        const QString &searchText,
        QTextStream &output,
        int &matchesFound);

    bool processWordFile(
        QAxObject *word,
        const QString &filePath,
        const QString &searchText,
        QTextStream &output,
        int &matchesFound);

    bool processPdfFile(
        const QString &filePath,
        const QString &searchText,
        QTextStream &output,
        int &matchesFound);

    void writeMatch(
        QTextStream &output,
        const QString &filePath,
        const QString &text,
        int pageNumber = -1);

private:

    QAtomicInt m_cancelRequested;
};
