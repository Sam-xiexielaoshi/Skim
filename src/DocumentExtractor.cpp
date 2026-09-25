#include "DocumentExtractor.h"

#include <QAxObject>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QPdfDocument>
#include <QPdfSelection>
#include <QRegularExpression>
#include <QStringConverter>
#include <QTextStream>

DocumentExtractorWorker::DocumentExtractorWorker(
    QObject *parent)
    : QObject(parent),
      m_cancelRequested(false)
{
}

// =========================================================
// CANCEL
// =========================================================

void DocumentExtractorWorker::cancel()
{
    m_cancelRequested.storeRelease(1);

    emit statusChanged(
        "Cancellation requested...");
}

// =========================================================
// PROCESS
// =========================================================

void DocumentExtractorWorker::process(
    const QString &folderPath,
    const QStringList &selectedFiles,
    const QString &searchText,
    const QString &outputFilePath,
    bool scanWord,
    bool scanText,
    bool scanPdf,
    bool includeSubfolders,
    bool filesMode)
{
    m_cancelRequested.storeRelease(0);

    int filesFound = 0;
    int filesScanned = 0;
    int matchesFound = 0;
    int emptyFiles = 0;
    int failedFiles = 0;

    QStringList failedFilePaths;
    QStringList filesToProcess;

    // =====================================================
    // VALIDATION
    // =====================================================

    if (filesMode)
    {
        if (selectedFiles.isEmpty())
        {
            emit errorOccurred(
                "No documents were selected.");
            return;
        }
    }
    else
    {
        if (folderPath.isEmpty())
        {
            emit errorOccurred(
                "Input folder is empty.");
            return;
        }

        QDir inputDir(folderPath);

        if (!inputDir.exists())
        {
            emit errorOccurred(
                "Input folder does not exist.");
            return;
        }
    }

    if (searchText.isEmpty())
    {
        emit errorOccurred(
            "Search text is empty.");
        return;
    }

    if (!scanWord && !scanText && !scanPdf)
    {
        emit errorOccurred(
            "Please select at least one file type.");
        return;
    }

    // =====================================================
    // OUTPUT FILE
    // =====================================================

    const QString absoluteOutputPath =
        QFileInfo(outputFilePath).absoluteFilePath();

    QFile outputFile(absoluteOutputPath);

    if (!outputFile.open(
            QIODevice::WriteOnly |
            QIODevice::Text))
    {
        emit errorOccurred(
            "Could not open output file:\n" +
            outputFile.errorString());
        return;
    }

    QTextStream output(&outputFile);
    output.setEncoding(QStringConverter::Utf8);

    // =====================================================
    // BUILD FILE LIST
    // =====================================================

    QStringList filters;

    if (scanWord)
    {
        filters << "*.doc" << "*.docx";
    }

    if (scanText)
    {
        filters << "*.txt";
    }

    if (scanPdf)
    {
        filters << "*.pdf";
    }

    if (filesMode)
    {
        for (const QString &path : selectedFiles)
        {
            const QFileInfo info(path);

            if (!info.exists() || !info.isFile())
            {
                continue;
            }

            const QString absolutePath =
                info.absoluteFilePath();

            if (absolutePath == absoluteOutputPath)
            {
                continue;
            }

            if (!filesToProcess.contains(absolutePath))
            {
                filesToProcess.append(absolutePath);
            }
        }
    }
    else
    {
        QDirIterator counter(
            folderPath,
            filters,
            QDir::Files | QDir::NoSymLinks,
            includeSubfolders
                ? QDirIterator::Subdirectories
                : QDirIterator::NoIteratorFlags);

        while (counter.hasNext())
        {
            const QString path = counter.next();
            const QFileInfo info(path);

            if (info.absoluteFilePath() == absoluteOutputPath)
            {
                continue;
            }

            filesToProcess.append(info.absoluteFilePath());
        }
    }

    filesFound = filesToProcess.size();

    // =====================================================
    // NOTHING TO SCAN
    // =====================================================

    if (filesFound == 0)
    {
        outputFile.close();

        emit progressChanged(100);
        emit finished(
            0, 0, 0, 0, 0, QStringList());
        return;
    }

    // =====================================================
    // START WORD
    // =====================================================

    QAxObject *word = nullptr;

    if (scanWord)
    {
        emit statusChanged(
            "Starting Microsoft Word...");

        word = new QAxObject("Word.Application");

        if (word->isNull())
        {
            delete word;
            word = nullptr;

            emit errorOccurred(
                "Could not start Microsoft Word.");

            outputFile.close();
            return;
        }

        connect(
            word,
            &QAxObject::exception,
            this,
            [this](
                int code,
                const QString &source,
                const QString &description,
                const QString &help)
            {
                Q_UNUSED(help);

                emit statusChanged(
                    QString(
                        "Word COM error %1 | %2 | %3")
                        .arg(code)
                        .arg(source)
                        .arg(description));
            });

        word->setProperty("Visible", false);
        word->setProperty("DisplayAlerts", false);
    }

    // =====================================================
    // PROCESS FILES
    // =====================================================

    bool wasCancelled = false;

    for (const QString &filePath : filesToProcess)
    {
        if (m_cancelRequested.loadAcquire())
        {
            wasCancelled = true;
            break;
        }

        emit logMessage("Scanning: " + filePath);

        const QFileInfo fileInfo(filePath);
        ++filesScanned;

        const int progress =
            static_cast<int>(
                (static_cast<double>(filesScanned) /
                 static_cast<double>(filesFound)) *
                100.0);

        emit progressChanged(progress);
        emit progressInfoChanged(
            filesScanned,
            filesFound,
            matchesFound);

        if (fileInfo.size() == 0)
        {
            ++emptyFiles;

            emit statusChanged(
                QString("Skipping empty file: %1")
                    .arg(fileInfo.fileName()));

            emit logMessage(
                QString("Skipped empty file: %1")
                    .arg(filePath));

            continue;
        }

        emit statusChanged(
            QString("Scanning %1 / %2: %3")
                .arg(filesScanned)
                .arg(filesFound)
                .arg(fileInfo.fileName()));

        const QString extension =
            fileInfo.suffix().toLower();

        bool success = false;
        const int matchesBefore = matchesFound;

        if (extension == "txt")
        {
            success = processTextFile(
                filePath,
                searchText,
                output,
                matchesFound);
        }
        else if (extension == "doc" || extension == "docx")
        {
            success = processWordFile(
                word,
                filePath,
                searchText,
                output,
                matchesFound);
        }
        else if (extension == "pdf")
        {
            success = processPdfFile(
                filePath,
                searchText,
                output,
                matchesFound);
        }

        emit progressInfoChanged(
            filesScanned,
            filesFound,
            matchesFound);

        if (!success)
        {
            ++failedFiles;
            failedFilePaths.append(filePath);

            emit statusChanged(
                QString("FAILED: %1")
                    .arg(fileInfo.fileName()));

            emit logMessage(
                QString("FAILED: %1")
                    .arg(filePath));
        }
        else
        {
            const int fileMatches =
                matchesFound - matchesBefore;

            emit logMessage(
                QString("Completed: %1 — %2 match%3")
                    .arg(filePath)
                    .arg(fileMatches)
                    .arg(fileMatches == 1 ? "" : "es"));
        }

        if (m_cancelRequested.loadAcquire())
        {
            wasCancelled = true;
            break;
        }
    }

    // =====================================================
    // CLOSE WORD
    // =====================================================

    if (word)
    {
        emit statusChanged(
            "Closing Microsoft Word...");

        word->dynamicCall("Quit()");
        delete word;
        word = nullptr;
    }

    // =====================================================
    // OUTPUT SUMMARY
    // =====================================================

    output << "\n\n"
           << "=======================================\n";

    if (wasCancelled)
    {
        output << "EXTRACTION CANCELLED\n";
    }
    else
    {
        output << "EXTRACTION SUMMARY\n";
    }

    output << "=======================================\n\n";
    output << "Files found: " << filesFound << "\n";
    output << "Files scanned: " << filesScanned << "\n";
    output << "Matches found: " << matchesFound << "\n";
    output << "Empty files: " << emptyFiles << "\n";
    output << "Failed files: " << failedFiles << "\n";

    if (!failedFilePaths.isEmpty())
    {
        output << "\n"
               << "FAILED FILES\n"
               << "---------------------------------------\n";

        for (const QString &failedPath : failedFilePaths)
        {
            output << failedPath << "\n";
        }
    }

    output.flush();
    outputFile.close();

    if (wasCancelled)
    {
        emit cancelled(
            filesFound,
            filesScanned,
            matchesFound,
            emptyFiles,
            failedFiles,
            failedFilePaths);
    }
    else
    {
        emit progressChanged(100);

        emit finished(
            filesFound,
            filesScanned,
            matchesFound,
            emptyFiles,
            failedFiles,
            failedFilePaths);
    }
}


// TEXT FILE PROCESSING
// =========================================================

bool DocumentExtractorWorker::processTextFile(
    const QString &filePath,
    const QString &searchText,
    QTextStream &output,
    int &matchesFound)
{
    QFile file(
        filePath);

    if (!file.open(
            QIODevice::ReadOnly |
            QIODevice::Text))
    {
        return false;
    }

    QTextStream input(
        &file);

    input.setEncoding(
        QStringConverter::Utf8);

    while (!input.atEnd())
    {
        // -------------------------------------------------
        // Cancellation check
        // -------------------------------------------------

        if (
            m_cancelRequested
                .loadAcquire())
        {
            break;
        }

        QString line =
            input.readLine();

        if (
            line.contains(
                searchText,
                Qt::CaseInsensitive))
        {
            QString cleaned =
                line.trimmed();

            if (!cleaned.isEmpty())
            {
                writeMatch(
                    output,
                    filePath,
                    cleaned);

                matchesFound++;
            }
        }
    }

    file.close();

    return true;
}

// =========================================================
// WORD FILE PROCESSING
// =========================================================

bool DocumentExtractorWorker::processWordFile(
    QAxObject *word,
    const QString &filePath,
    const QString &searchText,
    QTextStream &output,
    int &matchesFound)
{
    if (!word)
    {
        return false;
    }

    // -----------------------------------------------------
    // Cancellation before opening document
    // -----------------------------------------------------

    if (
        m_cancelRequested
            .loadAcquire())
    {
        return true;
    }

    QString nativePath =
        QDir::toNativeSeparators(
            QFileInfo(
                filePath)
                .absoluteFilePath());

    emit statusChanged(
        QString(
            "Opening Word document: %1")
            .arg(
                QFileInfo(
                    filePath)
                    .fileName()));

    QAxObject *documents =
        word->querySubObject(
            "Documents");

    if (!documents)
    {
        return false;
    }

    // -----------------------------------------------------
    // IMPORTANT:
    // This is the working Word Open signature.
    // -----------------------------------------------------

    QAxObject *document =
        documents->querySubObject(
            "Open(const QString&, bool)",
            nativePath,
            true);

    if (!document)
    {
        return false;
    }

    QAxObject *content =
        document->querySubObject(
            "Content");

    if (!content)
    {
        document->dynamicCall(
            "Close(bool)",
            false);

        return false;
    }

    QString text =
        content
            ->property("Text")
            .toString();

    QStringList paragraphs =
        text.split(
            QRegularExpression(
                "[\\r\\n]+"),
            Qt::SkipEmptyParts);

    for (
        const QString &paragraph :
        paragraphs)
    {
        QString cleaned =
            paragraph.trimmed();

        if (cleaned.isEmpty())
        {
            continue;
        }

        if (
            cleaned.contains(
                searchText,
                Qt::CaseInsensitive))
        {
            writeMatch(
                output,
                filePath,
                cleaned);

            matchesFound++;
        }
    }

    // -----------------------------------------------------
    // Always close current document
    // -----------------------------------------------------

    document->dynamicCall(
        "Close(bool)",
        false);

    emit statusChanged(
        QString(
            "Finished: %1")
            .arg(
                QFileInfo(
                    filePath)
                    .fileName()));

    return true;
}

// =========================================================
// WRITE MATCH
// =========================================================

void DocumentExtractorWorker::writeMatch(
    QTextStream &output,
    const QString &filePath,
    const QString &text,
    int pageNumber)
{
    output
        << "=======================================\n";

    output
        << "File: "
        << filePath
        << "\n";

    if (pageNumber > 0)
    {
        output
            << "Page: "
            << pageNumber
            << "\n";
    }

    output
        << "Match:\n";

    output
        << text
        << "\n\n";

    output.flush();
}

// =========================================================
// PDF FILE PROCESSING
// =========================================================

bool DocumentExtractorWorker::processPdfFile(
    const QString &filePath,
    const QString &searchText,
    QTextStream &output,
    int &matchesFound)
{
    QPdfDocument document;

    const QPdfDocument::Error error =
        document.load(filePath);

    if (error != QPdfDocument::Error::None)
    {
        return false;
    }

    const int pageCount =
        document.pageCount();

    for (int page = 0;
         page < pageCount;
         ++page)
    {
        if (m_cancelRequested.loadAcquire())
        {
            break;
        }

        QPdfSelection selection =
            document.getAllText(page);

        const QString pageText =
            selection.text();

        if (pageText.isEmpty())
        {
            continue;
        }

        const QStringList lines =
            pageText.split(
                QRegularExpression("[\\r\\n]+"),
                Qt::SkipEmptyParts);

        for (const QString &line : lines)
        {
            if (m_cancelRequested.loadAcquire())
            {
                break;
            }

            const QString cleanedLine =
                line.trimmed();

            if (cleanedLine.isEmpty())
            {
                continue;
            }

            if (cleanedLine.contains(
                    searchText,
                    Qt::CaseInsensitive))
            {
                writeMatch(
                    output,
                    filePath,
                    cleanedLine,
                    page + 1);

                ++matchesFound;
            }
        }
    }

    document.close();

    return true;
}
