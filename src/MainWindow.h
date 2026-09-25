#pragma once

#include <QMainWindow>
#include <QStringList>

class QLineEdit;
class QCheckBox;
class QPushButton;
class QProgressBar;
class QLabel;
class QThread;
class QTextEdit;
class QRadioButton;
class QEvent;
class QFrame;

class DocumentExtractorWorker;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void browseFolder();
    void browseOutput();
    void startExtraction();
    void cancelExtraction();
    void updateProgress(int value);
    void updateProgressInfo(int currentFile, int totalFiles, int matchesFound);
    void updateStatus(const QString &status);
    void appendLog(const QString &message);
    void extractionFinished(int filesFound, int filesScanned, int matchesFound, int emptyFiles, int failedFiles, const QStringList &failedFilePaths);
    void extractionCancelled(int filesFound, int filesScanned, int matchesFound, int emptyFiles, int failedFiles, const QStringList &failedFilePaths);
    void extractionError(const QString &errorMessage);
    void setExtractionUiLocked(bool locked);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QLineEdit *folderEdit = nullptr;
    QFrame *dropZone = nullptr;
    QLabel *dropZoneTitleLabel = nullptr;
    QLabel *dropZoneHintLabel = nullptr;
    QLineEdit *searchEdit = nullptr;
    QLineEdit *outputEdit = nullptr;
    QRadioButton *folderModeRadio = nullptr;
    QRadioButton *singleFileModeRadio = nullptr;
    QCheckBox *wordCheckBox = nullptr;
    QCheckBox *textCheckBox = nullptr;
    QCheckBox *pdfCheckBox = nullptr;
    QCheckBox *subfoldersCheckBox = nullptr;
    QPushButton *browseOutputButton = nullptr;
    QPushButton *startButton = nullptr;
    QPushButton *cancelButton = nullptr;
    QProgressBar *progressBar = nullptr;
    QLabel *progressInfoLabel = nullptr;
    QLabel *statusLabel = nullptr;
    QTextEdit *logTextEdit = nullptr;
    QThread *workerThread = nullptr;
    DocumentExtractorWorker *extractorWorker = nullptr;
    QStringList selectedFiles;
};
