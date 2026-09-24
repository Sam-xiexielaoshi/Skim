#include "MainWindow.h"
#include "DocumentExtractor.h"

#include <QApplication>
#include <QKeyEvent>
#include <QFile>

#include <QCheckBox>
#include <QDialog>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QRadioButton>
#include <QPushButton>
#include <QThread>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <QSizePolicy>
#include <QFrame>
#include <QIcon>
#include <QAction>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>

// =========================================================
// CONSTRUCTOR
// =========================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // =====================================================
    // WINDOW
    // =====================================================

    setWindowTitle("Document Extractor");

    setWindowIcon(
        QIcon(":/DocumentExtractor_256.png"));

    resize(1150, 1100);

    setFixedSize(1150, 1100);

    // =====================================================
    // CENTRAL WIDGET
    // =====================================================

    QWidget *centralWidget =
        new QWidget(this);

    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout =
        new QVBoxLayout(centralWidget);

    mainLayout->setContentsMargins(
        30, 24, 30, 24);

    mainLayout->setSpacing(13);

    // =====================================================
    // HEADER
    // =====================================================

    QWidget *headerWidget =
        new QWidget();

    QVBoxLayout *headerLayout =
        new QVBoxLayout(headerWidget);

    headerLayout->setContentsMargins(
        0, 0, 0, 4);

    headerLayout->setSpacing(3);

    headerWidget->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed);

    QLabel *titleLabel =
        new QLabel("Document Extractor");

    titleLabel->setObjectName(
        "titleLabel");

    // =====================================================
    // MENU BAR
    // =====================================================

    QMenu *fileMenu =
        menuBar()->addMenu("File");

    QAction *browseFolderAction =
        new QAction("Browse Scan Location", this);

    browseFolderAction->setShortcut(
        QKeySequence("Ctrl+O"));
    browseFolderAction->setShortcutContext(
        Qt::ApplicationShortcut);

    QAction *browseOutputAction =
        new QAction("Browse Output File", this);

    browseOutputAction->setShortcut(
        QKeySequence("Ctrl+Shift+O"));
    browseOutputAction->setShortcutContext(
        Qt::ApplicationShortcut);

    QAction *startAction =
        new QAction("Start Extraction", this);
    startAction->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_Return));
    startAction->setShortcutContext(
        Qt::ApplicationShortcut);

    QAction *cancelAction =
        new QAction("Cancel Extraction", this);
    cancelAction->setShortcut(
        QKeySequence(Qt::Key_Escape));
    cancelAction->setShortcutContext(
        Qt::ApplicationShortcut);

    QAction *exitAction =
        new QAction("Exit", this);
    exitAction->setShortcut(
        QKeySequence("Ctrl+Q"));

    fileMenu->addAction(browseFolderAction);
    fileMenu->addAction(browseOutputAction);
    fileMenu->addSeparator();
    fileMenu->addAction(startAction);
    fileMenu->addAction(cancelAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    QMenu *toolsMenu =
        menuBar()->addMenu("Tools");

    QAction *clearLogAction =
        new QAction("Clear Log", this);

    QAction *resetAction =
        new QAction("Reset", this);

    toolsMenu->addAction(clearLogAction);
    toolsMenu->addAction(resetAction);

    QMenu *helpMenu =
        menuBar()->addMenu("Help");

    QAction *aboutAction =
        new QAction("About Document Extractor", this);

    QAction *aboutQtAction =
        new QAction("About Qt", this);

    helpMenu->addAction(aboutAction);
    helpMenu->addAction(aboutQtAction);

    // =====================================================
    // HEADER
    // =====================================================

    headerLayout->addWidget(
        titleLabel);

    QLabel *subtitleLabel =
        new QLabel(
            "Search through your documents quickly and extract matching content.");

    subtitleLabel->setObjectName(
        "subtitleLabel");

    headerLayout->addWidget(
        subtitleLabel);

    mainLayout->addWidget(
        headerWidget);

    // Menu actions

    connect(
        browseFolderAction,
        &QAction::triggered,
        this,
        &MainWindow::browseFolder);

    connect(
        browseOutputAction,
        &QAction::triggered,
        this,
        &MainWindow::browseOutput);

    connect(
        startAction,
        &QAction::triggered,
        this,
        &MainWindow::startExtraction);

    connect(
        cancelAction,
        &QAction::triggered,
        this,
        &MainWindow::cancelExtraction);

    connect(
        exitAction,
        &QAction::triggered,
        this,
        &QWidget::close);

    connect(
        clearLogAction,
        &QAction::triggered,
        this,
        [this]()
        {
            logTextEdit->clear();
            statusLabel->setText("Status: Ready");
        });

    connect(
        resetAction,
        &QAction::triggered,
        this,
        [this]()
        {
            if (workerThread)
            {
                QMessageBox::information(
                    this,
                    "Extraction Running",
                    "Please wait for the current extraction to finish before resetting the form.");
                return;
            }

            folderEdit->clear();
            searchEdit->clear();
            outputEdit->clear();

            folderModeRadio->setChecked(true);

            wordCheckBox->setChecked(true);
            textCheckBox->setChecked(true);
            pdfCheckBox->setChecked(true);
            subfoldersCheckBox->setChecked(true);

            progressBar->setValue(0);
            progressInfoLabel->setText(
                "0 / 0 files    |    0 matches found");
            statusLabel->setText("Status: Ready");
            logTextEdit->clear();
        });

    connect(
        aboutQtAction,
        &QAction::triggered,
        this,
        [this]()
        {
            QApplication::aboutQt();
        });

    connect(
        aboutAction,
        &QAction::triggered,
        this,
        [this]()
        {
            QDialog dialog(this);

            dialog.setWindowTitle(
                "About Document Extractor");

            dialog.setWindowIcon(
                QIcon(":/DocumentExtractor_256.png"));

            dialog.setFixedSize(
                500, 330);

            QVBoxLayout *aboutLayout =
                new QVBoxLayout(&dialog);

            aboutLayout->setContentsMargins(
                28, 26, 28, 22);

            aboutLayout->setSpacing(16);

            QHBoxLayout *identityLayout =
                new QHBoxLayout();

            identityLayout->setSpacing(18);

            QLabel *iconLabel =
                new QLabel();

            iconLabel->setObjectName(
                "aboutIconLabel");

            QPixmap iconPixmap(
                ":/DocumentExtractor_256.png");

            iconLabel->setPixmap(
                iconPixmap.scaled(
                    82,
                    82,
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation));

            iconLabel->setFixedSize(
                82, 82);

            QLabel *identityText =
                new QLabel(
                    "<b>Document Extractor</b><br>"
                    "<span style='font-size:13px;'>Version 1.0.0</span>");

            identityText->setObjectName(
                "aboutIdentityLabel");

            identityText->setTextFormat(
                Qt::RichText);

            identityText->setAlignment(
                Qt::AlignVCenter | Qt::AlignLeft);

            identityLayout->addWidget(
                iconLabel);

            identityLayout->addWidget(
                identityText);

            identityLayout->addStretch();

            aboutLayout->addLayout(
                identityLayout);

            QFrame *separator =
                new QFrame();

            separator->setFrameShape(
                QFrame::HLine);

            separator->setObjectName(
                "aboutSeparator");

            aboutLayout->addWidget(
                separator);

            QLabel *descriptionLabel =
                new QLabel(
                    "A fast and lightweight utility for finding "
                    "matching text across your documents.");

            descriptionLabel->setObjectName(
                "aboutDescriptionLabel");

            descriptionLabel->setWordWrap(
                true);

            aboutLayout->addWidget(
                descriptionLabel);

            QLabel *supportLabel =
                new QLabel(
                    "Supports Word documents, text files and PDF files.<br>"
                    "Built with Qt 6 and C++.");

            supportLabel->setObjectName(
                "aboutSupportLabel");

            supportLabel->setWordWrap(
                true);

            aboutLayout->addWidget(
                supportLabel);

            aboutLayout->addStretch();

            QDialogButtonBox *buttonBox =
                new QDialogButtonBox(
                    QDialogButtonBox::Ok);

            buttonBox->setObjectName(
                "aboutButtonBox");

            connect(
                buttonBox,
                &QDialogButtonBox::accepted,
                &dialog,
                &QDialog::accept);

            aboutLayout->addWidget(
                buttonBox);

            dialog.exec();
        });

    // =====================================================
    // SCAN LOCATION CARD
    // =====================================================

    QFrame *locationCard =
        new QFrame();

    locationCard->setObjectName(
        "card");

    locationCard->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed);

    locationCard->setMinimumHeight(94);

    QVBoxLayout *locationLayout =
        new QVBoxLayout(locationCard);

    locationLayout->setContentsMargins(
        20, 18, 20, 18);

    locationLayout->setSpacing(10);

    QLabel *locationTitle =
        new QLabel("SCAN LOCATION");

    locationTitle->setObjectName(
        "sectionTitle");

    locationLayout->addWidget(
        locationTitle);

    QHBoxLayout *modeLayout =
        new QHBoxLayout();

    modeLayout->setSpacing(24);

    folderModeRadio =
        new QRadioButton("Folder");

    singleFileModeRadio =
        new QRadioButton("Single File");

    folderModeRadio->setChecked(true);

    modeLayout->addWidget(
        folderModeRadio);

    modeLayout->addWidget(
        singleFileModeRadio);

    modeLayout->addStretch();

    locationLayout->addLayout(
        modeLayout);

    QHBoxLayout *folderLayout =
        new QHBoxLayout();

    folderLayout->setSpacing(10);

    folderEdit =
        new QLineEdit();

    folderEdit->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed);

    folderEdit->setMinimumHeight(36);

    folderEdit->setPlaceholderText(
        "Select a folder to scan...");

    browseFolderButton =
        new QPushButton("Browse Folder");

    browseFolderButton->setObjectName(
        "secondaryButton");

    browseFolderButton->setMinimumHeight(36);

    browseFolderButton->setMinimumWidth(130);

    browseFolderButton->setSizePolicy(
        QSizePolicy::Fixed,
        QSizePolicy::Fixed);

    folderLayout->addWidget(
        folderEdit,
        1);

    folderLayout->addWidget(
        browseFolderButton);

    locationLayout->addLayout(
        folderLayout);

    connect(
        folderModeRadio,
        &QRadioButton::toggled,
        this,
        [this](bool folderMode)
        {
            if (!folderMode)
            {
                folderEdit->setPlaceholderText(
                    "Select a document to scan...");

                browseFolderButton->setText(
                    "Browse File");

                subfoldersCheckBox->setEnabled(
                    false);

                subfoldersCheckBox->setChecked(
                    false);
            }
            else
            {
                folderEdit->setPlaceholderText(
                    "Select a folder to scan...");

                browseFolderButton->setText(
                    "Browse Folder");

                subfoldersCheckBox->setEnabled(
                    true);
            }
        });

    locationLayout->setSpacing(8);

    mainLayout->addWidget(
        locationCard);

    // =====================================================
    // SEARCH + OUTPUT CARD
    // =====================================================

    QFrame *searchCard =
        new QFrame();

    searchCard->setObjectName(
        "card");

    searchCard->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed);

    searchCard->setMinimumHeight(174);

    QVBoxLayout *searchLayout =
        new QVBoxLayout(searchCard);

    searchLayout->setContentsMargins(
        20, 18, 20, 18);

    searchLayout->setSpacing(10);

    // Search

    QLabel *searchTitle =
        new QLabel("SEARCH TEXT");

    searchTitle->setObjectName(
        "sectionTitle");

    searchLayout->addWidget(
        searchTitle);

    searchEdit =
        new QLineEdit();

    searchEdit->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed);

    searchEdit->setMinimumHeight(36);

    searchEdit->setPlaceholderText(
        "Enter text to search for...");

    searchLayout->addWidget(
        searchEdit);

    // Output

    QLabel *outputTitle =
        new QLabel("OUTPUT FILE");

    outputTitle->setObjectName(
        "sectionTitle");

    searchLayout->addWidget(
        outputTitle);

    QHBoxLayout *outputLayout =
        new QHBoxLayout();

    outputLayout->setSpacing(10);
    outputEdit =
        new QLineEdit();

    outputEdit->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed);

    outputEdit->setMinimumHeight(36);

    outputEdit->setPlaceholderText(
        "Select output file...");

    browseOutputButton =
        new QPushButton("Browse");

    browseOutputButton->setObjectName(
        "secondaryButton");

    browseOutputButton->setMinimumHeight(36);

    browseOutputButton->setMinimumWidth(110);

    browseOutputButton->setSizePolicy(
        QSizePolicy::Fixed,
        QSizePolicy::Fixed);

    outputLayout->addWidget(
        outputEdit,
        1);

    outputLayout->addWidget(
        browseOutputButton);

    searchLayout->addLayout(
        outputLayout);

    mainLayout->addWidget(
        searchCard);

    // =====================================================
    // OPTIONS CARD
    // =====================================================

    QFrame *optionsCard =
        new QFrame();

    optionsCard->setObjectName(
        "card");

    optionsCard->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed);

    optionsCard->setMinimumHeight(90);

    QVBoxLayout *optionsLayout =
        new QVBoxLayout(optionsCard);

    optionsLayout->setContentsMargins(
        20, 18, 20, 18);

    optionsLayout->setSpacing(12);

    QLabel *optionsTitle =
        new QLabel("OPTIONS");

    optionsTitle->setObjectName(
        "sectionTitle");

    optionsLayout->addWidget(
        optionsTitle);

    QHBoxLayout *checkboxLayout =
        new QHBoxLayout();

    checkboxLayout->setSpacing(28);

    wordCheckBox =
        new QCheckBox("Word documents");
    wordCheckBox->setChecked(true);

    textCheckBox =
        new QCheckBox("Text files");
    textCheckBox->setChecked(true);

    pdfCheckBox =
        new QCheckBox("PDF documents");
    pdfCheckBox->setChecked(true);

    subfoldersCheckBox =
        new QCheckBox("Include subfolders");
    subfoldersCheckBox->setChecked(true);

    checkboxLayout->addWidget(
        wordCheckBox);

    checkboxLayout->addWidget(
        textCheckBox);

    checkboxLayout->addWidget(pdfCheckBox);

    checkboxLayout->addWidget(
        subfoldersCheckBox);

    checkboxLayout->addStretch();

    optionsLayout->addLayout(
        checkboxLayout);

    mainLayout->addWidget(
        optionsCard);

    // =====================================================
    // ACTION BUTTONS
    // =====================================================

    QHBoxLayout *buttonLayout =
        new QHBoxLayout();

    buttonLayout->setSpacing(12);
    startButton =
        new QPushButton("▶   Start Extraction");

    startButton->setObjectName("startButton");

    startButton->setMinimumHeight(48);

    startButton->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed);

    cancelButton =
        new QPushButton("✕   Cancel");

    cancelButton->setObjectName("cancelButton");

    cancelButton->setMinimumHeight(48);

    cancelButton->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed);

    cancelButton->setEnabled(false);
    buttonLayout->setSpacing(12);

    buttonLayout->addWidget(startButton, 3);
    buttonLayout->addWidget(cancelButton, 1);

    cancelButton->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed);

    mainLayout->addLayout(
        buttonLayout);

    // =====================================================
    // PROGRESS CARD
    // =====================================================

    QFrame *progressCard =
        new QFrame();

    progressCard->setObjectName(
        "card");

    progressCard->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed);

    progressCard->setMinimumHeight(120);

    QVBoxLayout *progressLayout =
        new QVBoxLayout(progressCard);

    progressLayout->setContentsMargins(
        20, 18, 20, 18);

    progressLayout->setSpacing(10);

    QHBoxLayout *progressHeader =
        new QHBoxLayout();

    QLabel *progressTitle =
        new QLabel("PROGRESS");

    progressTitle->setObjectName(
        "sectionTitle");

    progressInfoLabel =
        new QLabel(
            "0 / 0 files    |    0 matches found");

    progressInfoLabel->setObjectName(
        "progressInfoLabel");

    progressInfoLabel->setAlignment(
        Qt::AlignRight);

    progressHeader->addWidget(
        progressTitle);

    progressHeader->addStretch();

    progressHeader->addWidget(
        progressInfoLabel);

    progressLayout->addLayout(
        progressHeader);

    progressBar =
        new QProgressBar();

    progressBar->setRange(
        0, 100);

    progressBar->setValue(
        0);

    progressBar->setTextVisible(
        false);

    progressLayout->addWidget(
        progressBar);

    statusLabel =
        new QLabel(
            "Status: Ready");

    statusLabel->setObjectName(
        "statusLabel");

    progressLayout->addWidget(
        statusLabel);

    mainLayout->addWidget(
        progressCard);

    // =====================================================
    // EXTRACTION LOG
    // =====================================================

    QFrame *logCard =
        new QFrame();

    logCard->setObjectName(
        "card");

    logCard->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding);

    logCard->setMinimumHeight(220);

    QVBoxLayout *logLayout =
        new QVBoxLayout(logCard);

    logLayout->setContentsMargins(
        20, 18, 20, 18);

    logLayout->setSpacing(10);

    QLabel *logTitle =
        new QLabel(
            "EXTRACTION LOG");

    logTitle->setObjectName(
        "sectionTitle");

    logLayout->addWidget(
        logTitle);

    logTextEdit =
        new QTextEdit();

    logTextEdit->setReadOnly(
        true);

    logTextEdit->setPlaceholderText(
        "Extraction activity will appear here...");

    logTextEdit->setMinimumHeight(200);

    logTextEdit->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding);

    logLayout->addWidget(
        logTextEdit);

    mainLayout->addWidget(
        logCard,
        1);

    // =====================================================
    // CONNECTIONS
    // =====================================================

    connect(
        browseFolderButton,
        &QPushButton::clicked,
        this,
        &MainWindow::browseFolder);

    connect(
        browseOutputButton,
        &QPushButton::clicked,
        this,
        &MainWindow::browseOutput);

    connect(
        startButton,
        &QPushButton::clicked,
        this,
        &MainWindow::startExtraction);

    connect(
        cancelButton,
        &QPushButton::clicked,
        this,
        &MainWindow::cancelExtraction);

    // =====================================================
    // LOAD APPLICATION STYLE
    // =====================================================

    QFile styleFile(
        ":/DocumentExtractor.qss");

    if (styleFile.open(
            QIODevice::ReadOnly |
            QIODevice::Text))
    {
        qApp->setStyleSheet(
            QString::fromUtf8(
                styleFile.readAll()));

        styleFile.close();
    }

    // =====================================================
    // GLOBAL KEYBOARD SHORTCUTS
    // =====================================================

    qApp->installEventFilter(this);
}

// =========================================================
// DESTRUCTOR
// =========================================================

MainWindow::~MainWindow()
{
    if (extractorWorker)
    {
        extractorWorker->cancel();
    }

    if (workerThread)
    {
        workerThread->wait();
    }
}

// =========================================================
// EXTRACTION UI LOCK
// =========================================================

void MainWindow::setExtractionUiLocked(bool locked)
{
    folderModeRadio->setEnabled(!locked);
    singleFileModeRadio->setEnabled(!locked);

    folderEdit->setEnabled(!locked);
    browseFolderButton->setEnabled(!locked);

    searchEdit->setEnabled(!locked);
    outputEdit->setEnabled(!locked);
    browseOutputButton->setEnabled(!locked);

    wordCheckBox->setEnabled(!locked);
    textCheckBox->setEnabled(!locked);
    pdfCheckBox->setEnabled(!locked);
    subfoldersCheckBox->setEnabled(!locked &&
                                   folderModeRadio->isChecked());

    startButton->setEnabled(!locked);
    cancelButton->setEnabled(locked);
}

// =========================================================
// BROWSE FOLDER
// =========================================================

void MainWindow::browseFolder()
{
    if (workerThread)
    {
        return;
    }

    if (singleFileModeRadio &&
        singleFileModeRadio->isChecked())
    {
        QStringList filters;

        if (wordCheckBox->isChecked())
        {
            filters << "*.doc"
                    << "*.docx";
        }

        if (textCheckBox->isChecked())
        {
            filters << "*.txt";
        }

        if (pdfCheckBox->isChecked())
        {
            filters << "*.pdf";
        }

        QString filterText;

        if (filters.isEmpty())
        {
            filterText =
                "Supported Documents (*.doc *.docx *.txt *.pdf);;"
                "Word Documents (*.doc *.docx);;"
                "Text Files (*.txt);;"
                "PDF Documents (*.pdf);;"
                "All Files (*)";
        }
        else
        {
            filterText =
                "Selected File Types (" +
                filters.join(' ') +
                ");;All Files (*)";
        }

        QString file =
            QFileDialog::getOpenFileName(
                this,
                "Select Document",
                QString(),
                filterText);

        if (!file.isEmpty())
        {
            folderEdit->setText(
                file);
        }

        return;
    }

    QString folder =
        QFileDialog::getExistingDirectory(
            this,
            "Select Folder");

    if (!folder.isEmpty())
    {
        folderEdit->setText(
            folder);
    }
}

// =========================================================
// BROWSE OUTPUT
// =========================================================

void MainWindow::browseOutput()
{
    if (workerThread)
    {
        return;
    }

    QString file =
        QFileDialog::getSaveFileName(
            this,
            "Select Output File",
            "ExtractedText.txt",
            "Text Files (*.txt);;All Files (*)");

    if (!file.isEmpty())
    {
        outputEdit->setText(
            file);
    }
}

// =========================================================
// START EXTRACTION
// =========================================================

void MainWindow::startExtraction()
{
    if (workerThread)
    {
        QMessageBox::information(
            this,
            "Extraction Running",
            "An extraction is already running.");

        return;
    }

    QString folderPath =
        folderEdit->text().trimmed();

    QString searchText =
        searchEdit->text().trimmed();

    QString outputFilePath =
        outputEdit->text().trimmed();

    bool scanWord =
        wordCheckBox->isChecked();

    bool scanText =
        textCheckBox->isChecked();

    bool scanPdf =
        pdfCheckBox->isChecked();

    bool includeSubfolders =
        subfoldersCheckBox->isChecked();

    const ScanMode scanMode =
        (singleFileModeRadio &&
         singleFileModeRadio->isChecked())
            ? ScanMode::SingleFile
            : ScanMode::Folder;

    // =====================================================
    // VALIDATION
    // =====================================================

    if (folderPath.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Missing Folder",
            "Please select a folder to scan.");

        return;
    }

    // =====================================================
    // SCAN LOCATION VALIDATION
    // =====================================================

    const QFileInfo scanInfo(folderPath);

    if (!scanInfo.exists())
    {
        QMessageBox::warning(
            this,
            scanMode == ScanMode::SingleFile
                ? "File Not Found"
                : "Folder Not Found",
            scanMode == ScanMode::SingleFile
                ? "The selected file does not exist.\n\n"
                  "Please choose a valid document."
                : "The selected input folder does not exist.\n\n"
                  "Please choose a valid folder.");

        return;
    }

    if (scanMode == ScanMode::SingleFile)
    {
        if (!scanInfo.isFile())
        {
            QMessageBox::warning(
                this,
                "Invalid Input File",
                "The selected scan location is not a file.\n\n"
                "Please choose a document.");

            return;
        }

        if (!scanInfo.isReadable())
        {
            QMessageBox::warning(
                this,
                "File Not Accessible",
                "The selected file cannot be read.\n\n"
                "Please choose a file you have permission to access.");

            return;
        }

        const QString extension =
            scanInfo.suffix().toLower();

        const bool supported =
            (extension == "doc" || extension == "docx")
                ? scanWord
            : (extension == "txt")
                ? scanText
            : (extension == "pdf")
                ? scanPdf
                : false;

        if (!supported)
        {
            QMessageBox::warning(
                this,
                "File Type Not Selected",
                "The selected file type is not enabled in Options.\n\n"
                "Please enable the corresponding file type and try again.");

            return;
        }

        includeSubfolders = false;
    }
    else
    {
        if (!scanInfo.isDir())
        {
            QMessageBox::warning(
                this,
                "Invalid Input Folder",
                "The selected input path is not a folder.\n\n"
                "Please choose a valid folder.");

            return;
        }

        if (!scanInfo.isReadable())
        {
            QMessageBox::warning(
                this,
                "Folder Not Accessible",
                "The selected input folder cannot be read.\n\n"
                "Please choose a folder you have permission to access.");

            return;
        }
    }

    if (searchText.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Missing Search Text",
            "Please enter text to search for.");

        return;
    }

    if (outputFilePath.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Missing Output File",
            "Please select an output file.");

        return;
    }

    // =====================================================
    // OUTPUT FILE VALIDATION
    // =====================================================

    const QFileInfo outputInfo(outputFilePath);
    const QDir outputDirectory =
        outputInfo.absoluteDir();

    if (outputInfo.exists() && outputInfo.isDir())
    {
        QMessageBox::warning(
            this,
            "Invalid Output File",
            "The selected output path is a folder.\n\n"
            "Please choose a file name.");

        return;
    }

    if (!outputDirectory.exists())
    {
        QMessageBox::warning(
            this,
            "Output Folder Not Found",
            "The folder containing the output file does not exist.\n\n"
            "Please choose an existing output folder.");

        return;
    }

    if (!outputDirectory.isReadable())
    {
        QMessageBox::warning(
            this,
            "Output Folder Not Accessible",
            "The output folder cannot be accessed.\n\n"
            "Please choose a folder you have permission to use.");

        return;
    }

    if (outputInfo.exists() && !outputInfo.isWritable())
    {
        QMessageBox::warning(
            this,
            "Output File Not Writable",
            "The selected output file cannot be modified.\n\n"
            "It may be read-only or currently in use.");

        return;
    }

    if (!outputInfo.exists() && !outputDirectory.isReadable())
    {
        QMessageBox::warning(
            this,
            "Output Folder Not Writable",
            "The output folder cannot be used for creating the output file.");

        return;
    }

    if (outputInfo.exists())
    {
        const QMessageBox::StandardButton overwrite =
            QMessageBox::question(
                this,
                "Overwrite Existing File",
                QString(
                    "The output file already exists.\n\n"
                    "%1\n\n"
                    "Do you want to overwrite it?")
                    .arg(outputInfo.absoluteFilePath()),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);

        if (overwrite != QMessageBox::Yes)
        {
            statusLabel->setText(
                "Status: Ready");

            return;
        }
    }

    if (!scanWord && !scanText && !scanPdf)
    {
        QMessageBox::warning(
            this,
            "No File Type",
            "Please select at least one file type.");

        return;
    }

    // =====================================================
    // RESET UI
    // =====================================================

    progressBar->setValue(
        0);

    progressInfoLabel->setText(
        "0 / 0 files    |    0 matches found");

    statusLabel->setText(
        "Status: Starting...");

    logTextEdit->clear();

    startButton->setEnabled(
        false);

    cancelButton->setEnabled(
        true);

    // =====================================================
    // CREATE THREAD
    // =====================================================

    // Lock configuration controls for the duration of the extraction.
    setExtractionUiLocked(true);

    workerThread =
        new QThread(this);

    // =====================================================
    // CREATE WORKER
    // =====================================================

    extractorWorker =
        new DocumentExtractorWorker();

    extractorWorker->moveToThread(
        workerThread);

    DocumentExtractorWorker *worker =
        extractorWorker;

    // =====================================================
    // PROGRESS
    // =====================================================

    connect(
        extractorWorker,
        &DocumentExtractorWorker::progressChanged,
        this,
        &MainWindow::updateProgress);

    connect(
        extractorWorker,
        &DocumentExtractorWorker::progressInfoChanged,
        this,
        &MainWindow::updateProgressInfo);

    // =====================================================
    // STATUS
    // =====================================================

    connect(
        extractorWorker,
        &DocumentExtractorWorker::statusChanged,
        this,
        &MainWindow::updateStatus);

    // =====================================================
    // LOG
    // =====================================================

    connect(
        extractorWorker,
        &DocumentExtractorWorker::logMessage,
        this,
        &MainWindow::appendLog);

    // =====================================================
    // FINISHED
    // =====================================================

    connect(
        extractorWorker,
        &DocumentExtractorWorker::finished,
        this,
        &MainWindow::extractionFinished);

    // =====================================================
    // CANCELLED
    // =====================================================

    connect(
        extractorWorker,
        &DocumentExtractorWorker::cancelled,
        this,
        &MainWindow::extractionCancelled);

    // =====================================================
    // ERROR
    // =====================================================

    connect(
        extractorWorker,
        &DocumentExtractorWorker::errorOccurred,
        this,
        &MainWindow::extractionError);

    // =====================================================
    // DELETE WORKER
    // =====================================================

    connect(
        workerThread,
        &QThread::finished,
        extractorWorker,
        &QObject::deleteLater);

    // =====================================================
    // THREAD CLEANUP
    // =====================================================

    connect(
        workerThread,
        &QThread::finished,
        this,
        [this]()
        {
            QThread *finishedThread =
                workerThread;

            workerThread =
                nullptr;

            extractorWorker =
                nullptr;

            startButton->setEnabled(
                true);

            cancelButton->setEnabled(
                false);

            setExtractionUiLocked(false);

            finishedThread->deleteLater();
        });

    // =====================================================
    // START PROCESSING
    // =====================================================

    connect(
        workerThread,
        &QThread::started,
        worker,
        [worker,
         folderPath,
         searchText,
         outputFilePath,
         scanWord,
         scanText,
         scanPdf,
         includeSubfolders,
         scanMode]()
        {
            worker->process(
                folderPath,
                searchText,
                outputFilePath,
                scanWord,
                scanText,
                scanPdf,
                includeSubfolders,
                scanMode);
        });

    // =====================================================
    // QUIT THREAD AFTER FINISH
    // =====================================================

    connect(
        extractorWorker,
        &DocumentExtractorWorker::finished,
        workerThread,
        &QThread::quit);

    // =====================================================
    // QUIT THREAD AFTER CANCEL
    // =====================================================

    connect(
        extractorWorker,
        &DocumentExtractorWorker::cancelled,
        workerThread,
        &QThread::quit);

    // =====================================================
    // QUIT THREAD AFTER ERROR
    // =====================================================

    connect(
        extractorWorker,
        &DocumentExtractorWorker::errorOccurred,
        workerThread,
        &QThread::quit);

    // =====================================================
    // START THREAD
    // =====================================================

    workerThread->start();
}

// =========================================================
// CANCEL EXTRACTION
// =========================================================
// GLOBAL KEYBOARD SHORTCUTS
// =========================================================

bool MainWindow::eventFilter(
    QObject *watched,
    QEvent *event)
{
    Q_UNUSED(watched);

    if (event->type() != QEvent::KeyPress)
    {
        return QMainWindow::eventFilter(
            watched,
            event);
    }

    if (QApplication::activeWindow() != this ||
        QApplication::activeModalWidget() != nullptr)
    {
        return QMainWindow::eventFilter(
            watched,
            event);
    }

    QKeyEvent *keyEvent =
        static_cast<QKeyEvent *>(event);

    const int key =
        keyEvent->key();

    const Qt::KeyboardModifiers modifiers =
        keyEvent->modifiers();

    // Ctrl + O -> Browse input folder
    if (key == Qt::Key_O &&
        (modifiers & Qt::ControlModifier) &&
        !(modifiers & Qt::ShiftModifier))
    {
        browseFolder();
        return true;
    }

    // Ctrl + Shift + O -> Browse output file
    if (key == Qt::Key_O &&
        (modifiers & Qt::ControlModifier) &&
        (modifiers & Qt::ShiftModifier))
    {
        browseOutput();
        return true;
    }

    if ((key == Qt::Key_Return ||
         key == Qt::Key_Enter) &&
        (modifiers & Qt::ControlModifier))
    {
        startExtraction();
        return true;
    }

    if (key == Qt::Key_Escape)
    {
        cancelExtraction();
        return true;
    }

    return QMainWindow::eventFilter(
        watched,
        event);
}

// =========================================================

void MainWindow::cancelExtraction()
{
    if (!extractorWorker)
    {
        return;
    }

    statusLabel->setText(
        "Status: Cancellation requested...");

    cancelButton->setEnabled(
        false);

    /*
        cancel() only sets an atomic flag, so this direct
        call is safe even though the worker lives in the
        background thread.
    */

    extractorWorker->cancel();
}

// =========================================================
// PROGRESS
// =========================================================

void MainWindow::updateProgress(
    int value)
{
    progressBar->setValue(
        value);
}

void MainWindow::updateProgressInfo(
    int currentFile,
    int totalFiles,
    int matchesFound)
{
    progressInfoLabel->setText(
        QString(
            "%1 / %2 files    |    %3 matches found")
            .arg(currentFile)
            .arg(totalFiles)
            .arg(matchesFound));
}

// =========================================================
// STATUS and log
// =========================================================

void MainWindow::updateStatus(
    const QString &status)
{
    statusLabel->setText(
        "Status: " + status);
}

void MainWindow::appendLog(
    const QString &message)
{
    logTextEdit->append(
        message);

    logTextEdit->ensureCursorVisible();
}

// =========================================================
// COMPLETION DIALOG
// =========================================================

void MainWindow::extractionFinished(
    int filesFound,
    int filesScanned,
    int matchesFound,
    int emptyFiles,
    int failedFiles,
    const QStringList &failedFilePaths)
{
    progressBar->setValue(100);

    statusLabel->setText(
        QString(
            "Finished — %1 files scanned, "
            "%2 matches found, "
            "%3 failed.")
            .arg(filesScanned)
            .arg(matchesFound)
            .arg(failedFiles));

    QDialog dialog(this);

    dialog.setWindowTitle("Extraction Complete");
    dialog.setWindowIcon(
        QIcon(":/DocumentExtractor_256.png"));
    dialog.setFixedSize(560, 470);

    QVBoxLayout *layout =
        new QVBoxLayout(&dialog);

    layout->setContentsMargins(30, 26, 30, 26);
    layout->setSpacing(16);

    QLabel *iconLabel =
        new QLabel();

    iconLabel->setObjectName(
        "completionIconLabel");

    iconLabel->setAlignment(
        Qt::AlignCenter);

    QPixmap iconPixmap(
        ":/DocumentExtractor_256.png");

    iconLabel->setPixmap(
        iconPixmap.scaled(
            58,
            58,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));

    layout->addWidget(iconLabel);

    QLabel *titleLabel =
        new QLabel("Extraction Complete");

    titleLabel->setObjectName(
        "completionTitleLabel");

    titleLabel->setAlignment(
        Qt::AlignCenter);

    layout->addWidget(titleLabel);

    QString completionSubtitle;

    if (filesFound == 0)
    {
        completionSubtitle =
            "No files matching your selected file types were found.";
    }
    else if (failedFiles == 0 && matchesFound == 0)
    {
        completionSubtitle =
            "The files were processed, but no matching text was found.";
    }
    else if (failedFiles == 0)
    {
        completionSubtitle =
            "All selected files were processed successfully.";
    }
    else
    {
        completionSubtitle =
            "Extraction finished with some files that could not be processed.";
    }

    QLabel *subtitleLabel =
        new QLabel(completionSubtitle);

    subtitleLabel->setObjectName(
        "completionSubtitleLabel");

    subtitleLabel->setWordWrap(true);
    subtitleLabel->setAlignment(
        Qt::AlignCenter);

    layout->addWidget(subtitleLabel);

    QFrame *statsCard =
        new QFrame();

    statsCard->setObjectName(
        "completionStatsCard");

    QGridLayout *statsLayout =
        new QGridLayout(statsCard);

    statsLayout->setContentsMargins(
        20, 16, 20, 16);

    statsLayout->setHorizontalSpacing(18);
    statsLayout->setVerticalSpacing(10);

    auto addStat =
        [statsLayout](
            int row,
            const QString &label,
            const QString &value)
    {
        QLabel *labelWidget =
            new QLabel(label);

        labelWidget->setObjectName(
            "completionStatLabel");

        QLabel *valueWidget =
            new QLabel(value);

        valueWidget->setObjectName(
            "completionStatValue");

        valueWidget->setAlignment(
            Qt::AlignRight);

        statsLayout->addWidget(
            labelWidget,
            row,
            0);

        statsLayout->addWidget(
            valueWidget,
            row,
            1);
    };

    addStat(0, "Files found", QString::number(filesFound));
    addStat(1, "Files scanned", QString::number(filesScanned));
    addStat(2, "Matches found", QString::number(matchesFound));
    addStat(3, "Empty files", QString::number(emptyFiles));
    addStat(4, "Failed files", QString::number(failedFiles));

    layout->addWidget(statsCard);

    if (!failedFilePaths.isEmpty())
    {
        QLabel *failedTitle =
            new QLabel("Failed files");

        failedTitle->setObjectName(
            "completionFailedTitle");

        layout->addWidget(
            failedTitle);

        QTextEdit *failedEdit =
            new QTextEdit();

        failedEdit->setObjectName(
            "completionFailedList");

        failedEdit->setReadOnly(true);
        failedEdit->setMaximumHeight(90);

        for (const QString &path : failedFilePaths)
        {
            failedEdit->append(
                "• " + path);
        }

        layout->addWidget(
            failedEdit);
    }

    layout->addStretch();

    QHBoxLayout *buttonLayout =
        new QHBoxLayout();

    buttonLayout->setSpacing(10);

    QPushButton *openFolderButton =
        new QPushButton(
            "Open Output Folder");

    openFolderButton->setObjectName(
        "completionSecondaryButton");

    QPushButton *closeButton =
        new QPushButton("Done");

    closeButton->setObjectName(
        "completionPrimaryButton");

    buttonLayout->addWidget(
        openFolderButton);

    buttonLayout->addStretch();

    buttonLayout->addWidget(
        closeButton);

    layout->addLayout(
        buttonLayout);

    connect(
        openFolderButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            const QString outputPath =
                outputEdit->text().trimmed();

            if (outputPath.isEmpty())
            {
                return;
            }

            const QFileInfo outputInfo(
                outputPath);

            QDesktopServices::openUrl(
                QUrl::fromLocalFile(
                    outputInfo.absolutePath()));
        });

    connect(
        closeButton,
        &QPushButton::clicked,
        &dialog,
        &QDialog::accept);

    dialog.exec();
}

// =========================================================
// EXTRACTION CANCELLED
// =========================================================

void MainWindow::extractionCancelled(
    int filesFound,
    int filesScanned,
    int matchesFound,
    int emptyFiles,
    int failedFiles,
    const QStringList &failedFilePaths)
{
    QString status =
        QString(
            "Cancelled — %1 files scanned, "
            "%2 matches found.")
            .arg(filesScanned)
            .arg(matchesFound);

    if (failedFiles > 0)
    {
        status +=
            QString(" %1 failed.")
                .arg(failedFiles);
    }

    statusLabel->setText(
        status);

    QDialog dialog(this);

    dialog.setWindowTitle(
        "Extraction Cancelled");

    dialog.setWindowIcon(
        QIcon(":/DocumentExtractor_256.png"));

    dialog.setFixedSize(
        560,
        failedFilePaths.isEmpty() ? 410 : 500);

    QVBoxLayout *layout =
        new QVBoxLayout(&dialog);

    layout->setContentsMargins(
        30, 26, 30, 26);

    layout->setSpacing(16);

    QLabel *iconLabel =
        new QLabel();

    iconLabel->setObjectName(
        "completionIconLabel");

    iconLabel->setAlignment(
        Qt::AlignCenter);

    QPixmap iconPixmap(
        ":/DocumentExtractor_256.png");

    iconLabel->setPixmap(
        iconPixmap.scaled(
            58,
            58,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));

    layout->addWidget(
        iconLabel);

    QLabel *titleLabel =
        new QLabel(
            "Extraction Cancelled");

    titleLabel->setObjectName(
        "completionTitleLabel");

    titleLabel->setAlignment(
        Qt::AlignCenter);

    layout->addWidget(
        titleLabel);

    QLabel *subtitleLabel =
        new QLabel(
            "The extraction stopped after your cancellation request. "
            "The results collected so far are shown below.");

    subtitleLabel->setObjectName(
        "completionSubtitleLabel");

    subtitleLabel->setWordWrap(true);

    subtitleLabel->setAlignment(
        Qt::AlignCenter);

    layout->addWidget(
        subtitleLabel);

    QFrame *statsCard =
        new QFrame();

    statsCard->setObjectName(
        "completionStatsCard");

    QGridLayout *statsLayout =
        new QGridLayout(statsCard);

    statsLayout->setContentsMargins(
        20, 16, 20, 16);

    statsLayout->setHorizontalSpacing(18);
    statsLayout->setVerticalSpacing(10);

    auto addStat =
        [statsLayout](
            int row,
            const QString &label,
            const QString &value)
    {
        QLabel *labelWidget =
            new QLabel(label);

        labelWidget->setObjectName(
            "completionStatLabel");

        QLabel *valueWidget =
            new QLabel(value);

        valueWidget->setObjectName(
            "completionStatValue");

        valueWidget->setAlignment(
            Qt::AlignRight);

        statsLayout->addWidget(
            labelWidget,
            row,
            0);

        statsLayout->addWidget(
            valueWidget,
            row,
            1);
    };

    addStat(0, "Files found", QString::number(filesFound));
    addStat(1, "Files scanned", QString::number(filesScanned));
    addStat(2, "Matches found", QString::number(matchesFound));
    addStat(3, "Empty files", QString::number(emptyFiles));
    addStat(4, "Failed files", QString::number(failedFiles));

    layout->addWidget(
        statsCard);

    if (!failedFilePaths.isEmpty())
    {
        QLabel *failedTitle =
            new QLabel("Failed files");

        failedTitle->setObjectName(
            "completionFailedTitle");

        layout->addWidget(
            failedTitle);

        QTextEdit *failedEdit =
            new QTextEdit();

        failedEdit->setObjectName(
            "completionFailedList");

        failedEdit->setReadOnly(true);
        failedEdit->setMaximumHeight(90);

        for (const QString &path : failedFilePaths)
        {
            failedEdit->append(
                "• " + path);
        }

        layout->addWidget(
            failedEdit);
    }

    layout->addStretch();

    QPushButton *closeButton =
        new QPushButton("Done");

    closeButton->setObjectName(
        "completionPrimaryButton");

    closeButton->setMinimumHeight(
        40);

    connect(
        closeButton,
        &QPushButton::clicked,
        &dialog,
        &QDialog::accept);

    QHBoxLayout *buttonLayout =
        new QHBoxLayout();

    buttonLayout->addStretch();
    buttonLayout->addWidget(
        closeButton);

    layout->addLayout(
        buttonLayout);

    dialog.exec();
}

// =========================================================
// EXTRACTION ERROR
// =========================================================

void MainWindow::extractionError(
    const QString &errorMessage)
{
    statusLabel->setText(
        "Status: Error");

    startButton->setEnabled(
        true);

    cancelButton->setEnabled(
        false);

    QMessageBox::critical(
        this,
        "Extraction Error",
        errorMessage);
}