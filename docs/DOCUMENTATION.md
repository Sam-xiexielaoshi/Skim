# Skim — Technical Documentation

> Technical reference for the Skim document extraction application.

## 1. Overview

Skim is a Windows desktop application written in C++ and Qt for searching documents for a user-supplied term and collecting matching lines into a consolidated output file.

The project began as a replacement for a VBA-based document scanning workflow. The application has since evolved into a standalone desktop program with a dedicated GUI, background processing, cancellation, multiple input workflows, format-specific extraction, statistics, and failure reporting.

The product is called **Skim**, while some internal C++ identifiers retain the original `DocumentExtractor` naming.

---

## 2. Design Goals

The implementation is built around several goals:

1. **Reliable extraction** — support the document formats required by the original workflow.
2. **Responsive UI** — keep long-running document processing away from the GUI thread.
3. **Safe cancellation** — stop processing cooperatively and clean up resources.
4. **Batch processing** — process folders or multiple individually selected files.
5. **Transparent results** — expose progress, matches, empty files, and failures.
6. **Maintainable architecture** — separate UI responsibilities from extraction responsibilities.
7. **Product-quality UX** — provide a consistent Qt interface, menus, shortcuts, drag-and-drop, dialogs, and application branding.

---

## 3. Supported Formats

| Format | Processing mechanism |
|---|---|
| `.txt` | Native text processing |
| `.doc` | Microsoft Word automation through ActiveQt |
| `.docx` | Microsoft Word automation through ActiveQt |
| `.pdf` | Qt PDF / `QPdfDocument` |

### PDF limitation

The current PDF implementation extracts embedded text. It does not perform OCR.

Consequently, an image-only/scanned PDF may contain no extractable text even though a human can visually read the document.

---

# 4. High-Level Architecture

The application is organized around a GUI layer and an extraction worker.

```text
                    ┌──────────────────────┐
                    │      MainWindow      │
                    │                      │
                    │  User interaction    │
                    │  Configuration       │
                    │  Progress / Log      │
                    │  Completion UI        │
                    └──────────┬───────────┘
                               │
                         starts worker
                               │
                               ▼
                    ┌──────────────────────┐
                    │ DocumentExtractor     │
                    │       Worker          │
                    └──────────┬───────────┘
                               │
              ┌────────────────┼────────────────┐
              │                │                │
              ▼                ▼                ▼
          TXT processor    Word processor    PDF processor
              │                │                │
              └────────────────┼────────────────┘
                               ▼
                        Output / Statistics
```

The important architectural boundary is:

- **`MainWindow`** manages presentation and user interaction.
- **`DocumentExtractorWorker`** manages extraction and processing state.

This prevents the GUI from becoming responsible for document-processing details.

---

# 5. Application Lifecycle

A normal extraction run follows this sequence:

```text
Application starts
       │
       ▼
User selects scan source
       │
       ▼
User enters search term
       │
       ▼
User chooses output destination
       │
       ▼
User starts extraction
       │
       ▼
MainWindow starts worker
       │
       ▼
Worker enumerates inputs
       │
       ▼
Format-specific processor handles each file
       │
       ▼
Matches are written to output
       │
       ▼
Statistics are updated
       │
       ▼
Worker reports completion/cancellation
       │
       ▼
MainWindow displays summary
```

---

# 6. Scan Modes

Skim has two source-selection modes.

## 6.1 Folder Mode

Folder mode represents the original directory-oriented workflow.

The user chooses one directory. Skim discovers supported documents within that directory.

The **Include subfolders** option controls whether nested directories are included.

Conceptually:

```text
Folder
├── report.pdf
├── notes.txt
├── document.docx
│
└── Subfolder
    ├── report2.pdf
    └── document2.docx
```

With subfolder scanning disabled, only files in the selected directory are considered.

With subfolder scanning enabled, the nested directory is also traversed.

---

## 6.2 Files Mode

Files mode allows individual documents to be selected.

Supported behavior includes:

- one file
- multiple files
- multiple drag-and-drop operations
- files from different folders
- files from different drives

Selected files are maintained as a collection and duplicate paths are ignored.

Switching from Files mode back to Folder mode clears the selected-file collection because the two modes represent different source models.

---

# 7. File Discovery

File discovery is responsible for determining which inputs are candidates for extraction.

The application should only dispatch supported document extensions to the appropriate processor.

The output file itself is excluded from scanning when it is located inside the scan source. This prevents Skim from reading its own generated results as a new input.

The discovery stage also supports the distinction between:

- candidate files found
- files successfully processed
- files skipped because they are empty
- files that failed processing

This distinction is reflected in the final statistics.

---

# 8. Search Model

The current search model is deliberately simple.

Searches are **case-insensitive**.

For example:

```text
Search term:
invoice
```

can match:

```text
Invoice
INVOICE
invoice
Invoice Number
Monthly invoice report
```

Matching is performed against extracted text lines.

The current implementation does not require a complex query language, regular-expression engine, or indexing system.

This keeps behavior predictable for the intended workflow.

---

# 9. TXT Processing

TXT files are the simplest processing path.

The processor reads the file's textual content, separates it into lines, and checks each relevant line for the search term.

A conceptual matching operation is:

```cpp
if (line.contains(searchText, Qt::CaseInsensitive))
{
    writeMatch(output, filePath, line);
    ++matchesFound;
}
```

The actual implementation also performs checks for empty content and cancellation.

A matching line is written to the output and the match counter is incremented.

---

# 10. Microsoft Word Processing

Word documents are processed through **Qt ActiveQt / QAxContainer** and Microsoft Word automation.

This covers:

- `.doc`
- `.docx`

The worker uses Word's automation interface to open a document and retrieve its textual content.

A known working opening sequence is:

```cpp
QAxObject *documents =
    word->querySubObject("Documents");

QAxObject *document =
    documents->querySubObject(
        "Open(const QString&, bool)",
        nativePath,
        true
    );
```

## 10.1 Word lifecycle

Word is started for the extraction run rather than repeatedly launching a new Word process for every document.

The application keeps Word hidden from the user and disables unnecessary interactive alerts.

At the end of processing, Word documents are closed and the Word automation object is shut down.

This cleanup is important both for normal completion and cancellation.

## 10.2 Why ActiveQt is used

The application needs to process traditional Word formats without implementing a complete Word document parser itself.

ActiveQt provides Qt integration with COM/ActiveX automation, allowing Skim to use Microsoft Word's own document model.

---

# 11. PDF Processing

PDF processing uses Qt PDF:

```cpp
QPdfDocument
```

The processing flow is page-aware.

```text
Load PDF
   │
   ▼
Check load result
   │
   ▼
Get page count
   │
   ▼
For each page
   │
   ├── Extract all page text
   │
   ├── Split into lines
   │
   ├── Search each line
   │
   └── Record page number for matches
   │
   ▼
Close document
```

The relevant implementation pattern is:

```cpp
QPdfDocument document;

QPdfDocument::Error error =
    document.load(filePath);

if (error != QPdfDocument::Error::None)
    return false;

const int pageCount =
    document.pageCount();

for (int page = 0; page < pageCount; ++page)
{
    QPdfSelection selection =
        document.getAllText(page);

    QString pageText =
        selection.text();

    // split into lines
    // perform case-insensitive matching
}
```

The internal page index is zero-based, while reported page numbers are one-based:

```cpp
page + 1
```

This makes the output match the page numbering users normally expect.

---

# 12. Empty Documents

Empty documents are handled separately from failed documents.

An empty file is not necessarily an error. It simply contains no useful content to search.

The application therefore records empty files separately in the summary:

```text
Empty files: ...
```

This prevents empty documents from being incorrectly represented as processing failures.

---

# 13. Failed Documents

A document can fail because it is corrupt, unreadable, unsupported internally, or because its processing mechanism reports an error.

A failed file is recorded in the worker's failed-file collection.

The application continues processing other inputs where possible.

At the end of the operation, failed paths are included in the completion summary:

```text
FAILED FILES
---------------------------------------
...
```

This is especially important for batch processing because one bad document should not prevent successful documents from being processed.

---

# 14. Output Generation

The output is a text file generated during the extraction operation.

Matching records are written as they are discovered rather than waiting until every input has been processed.

The final output contains an extraction summary.

A normal summary follows the structure:

```text
=======================================

EXTRACTION SUMMARY
=======================================

Files found: ...
Files scanned: ...
Matches found: ...
Empty files: ...
Failed files: ...

FAILED FILES
---------------------------------------
...
```

A cancelled run is identified explicitly as:

```text
EXTRACTION CANCELLED
```

This prevents a partially completed extraction from being mistaken for a fully completed run.

---

# 15. Background Processing

Document processing is performed by a worker rather than directly inside the GUI's event-processing path.

The reason is straightforward: document I/O and parsing can take an unpredictable amount of time.

If those operations were performed synchronously in the GUI thread, the interface could stop repainting and responding to user input.

The worker architecture allows:

```text
GUI thread
    │
    ├── controls
    ├── rendering
    ├── progress display
    └── user input
             │
             │ signals / worker control
             ▼
Worker
    │
    ├── file enumeration
    ├── document processing
    ├── output writing
    └── statistics
```

The GUI remains available to display progress and receive cancellation requests.

---

# 16. Cooperative Cancellation

Cancellation uses a shared atomic cancellation state.

The worker checks the cancellation request during processing.

Conceptually:

```cpp
if (m_cancelRequested.loadAcquire())
{
    // stop processing safely
}
```

The worker does not forcibly terminate its thread.

Instead, it:

1. notices the cancellation request;
2. stops starting additional work;
3. finishes/escapes the current safe processing boundary;
4. closes documents and resources;
5. finalizes the output/summary state;
6. reports cancellation to the GUI.

This is safer than terminating a thread while it may be holding a file, PDF document, or COM automation object.

---

# 17. Word Cleanup and Cancellation

Word automation requires particular care during cancellation.

The application must avoid leaving a hidden Word process running after the user cancels an extraction.

The cleanup sequence therefore includes closing the active document and shutting down the Word automation object.

This is one reason cooperative cancellation is preferred over forcefully killing the worker.

---

# 18. Progress and Logging

The extraction log provides live feedback.

Typical events include:

```text
Scanning: ...
Completed: ...
Skipped empty file: ...
FAILED: ...
```

The GUI presents these events while the worker continues processing.

The log is intentionally plain text.

A previous experiment attempted to make source filenames clickable from the log. That approach was abandoned because it introduced unnecessary widget, anchor, HTML, and build complexity for a convenience feature that was not essential to the extraction workflow.

The stable implementation keeps the extraction log simple.

---

# 19. Statistics

Skim distinguishes several statistics:

| Statistic | Meaning |
|---|---|
| Files found | Candidate inputs discovered/selected |
| Files scanned | Inputs successfully processed |
| Matches found | Matching lines discovered |
| Empty files | Files with no useful content |
| Failed files | Inputs that could not be processed successfully |

These values provide a concise operational report at the end of a batch.

---

# 20. GUI Architecture

The main GUI is implemented through `MainWindow`.

Its responsibilities include:

- source selection
- search term configuration
- output configuration
- options
- progress presentation
- extraction logging
- start/cancel controls
- completion dialogs
- menus
- keyboard shortcuts
- drag-and-drop
- reset/clear operations
- application settings
- product presentation

The extraction worker should not own these presentation responsibilities.

---

# 21. GUI Layout

The interface is organized into visual cards.

The main areas are:

```text
┌─────────────────────────────────────────────┐
│                   SKIM                      │
│        Document Extraction Application      │
├─────────────────────────────────────────────┤
│ Scan Location                               │
│ Folder ○   Files ○                          │
├─────────────────────────────────────────────┤
│ Search + Output                             │
│ Search term                                 │
│ Output destination                          │
├─────────────────────────────────────────────┤
│ Options                                     │
│ Include subfolders                          │
├─────────────────────────────────────────────┤
│ Progress                                    │
├─────────────────────────────────────────────┤
│ Extraction Log                              │
│                                             │
├─────────────────────────────────────────────┤
│          Start              Cancel          │
└─────────────────────────────────────────────┘
```

The main window is currently designed around a fixed size of approximately:

```text
1150 × 1100
```

This preserves the spacing and proportions of the polished interface.

---

# 22. Qt Styling

Skim uses a global Qt stylesheet stored as a resource.

The stylesheet controls the visual appearance of the application, including:

- cards
- labels
- input fields
- buttons
- checkboxes
- radio buttons
- progress controls
- log area
- spacing and visual states

The stylesheet is loaded through the Qt resource system rather than requiring an external stylesheet file at runtime.

---

# 23. Qt Resources

The application embeds resources such as:

```text
Resources/
├── DocumentExtractor.qss
├── checkmark.svg
└── DocumentExtractor_256.png
```

The QSS file is included as a Qt resource.

The runtime icon is loaded using a Qt resource path:

```cpp
setWindowIcon(
    QIcon(":/DocumentExtractor_256.png"));
```

This avoids requiring the application to locate the image through a machine-specific filesystem path.

---

# 24. Windows Application Icon

The project also contains a Windows resource:

```text
app_icon.rc
```

The resource points to the application's ICO file and allows the Windows executable to carry the Skim application icon.

This is separate from the runtime Qt window icon.

The two mechanisms work together:

- Windows resource → executable/file icon
- Qt resource → application window icon

---

# 25. Menus

The application currently provides:

## File

- Choose Scan Location...
- Browse Output File
- Start Extraction
- Cancel Extraction
- Exit

## Tools

- Clear Log
- Reset

## Help

- About Skim
- About Qt

This gives users both mouse-driven controls and keyboard-driven workflows.

---

# 26. Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl+O` | Choose Scan Location |
| `Ctrl+Shift+O` | Browse Output File |
| `Ctrl+Enter` | Start Extraction |
| `Esc` | Cancel Extraction |
| `Ctrl+Q` | Exit |

Keyboard handling is integrated into the application event processing.

---

# 27. Drag-and-Drop

Drag-and-drop is part of the input workflow.

## Folder mode

A folder can be dropped as the scan source.

## Files mode

One or more supported documents can be dropped.

Multiple drop operations are additive.

For example:

```text
Drop:
C:\Reports\a.pdf
C:\Reports\b.docx

then drop:

D:\Archive\c.pdf
D:\Archive\d.txt
```

The resulting selection contains all four files, provided they are supported and not duplicates.

---

# 28. Multi-File Selection

Files mode maintains a collection of selected paths rather than assuming a single location.

This enables:

- documents from separate folders
- documents from different drives
- multiple drag-and-drop operations
- file-picker multi-selection

The interface displays a concise selected-file count, while the underlying selection retains the actual paths.

---

# 29. Duplicate Prevention

A document selected through multiple operations should not be processed repeatedly.

Skim therefore prevents duplicate paths from being inserted into the selected-file collection.

This is particularly important for additive drag-and-drop, where the same file could otherwise be accidentally added several times.

---

# 30. Completion Dialog

At the end of extraction, Skim displays the result through a dedicated completion interface.

The dialog communicates:

- whether the operation completed or was cancelled
- scan statistics
- failure information

The dialog layout was deliberately stabilized after testing revealed that moving/resizing interactions could cause sections to overlap or become visually compressed.

The current design uses a fixed, controlled layout for the completion information.

---

# 31. Application Settings

Skim uses Qt's `QSettings` mechanism for application settings.

The product namespace is being transitioned to:

```cpp
QSettings settings("Skim", "Skim");
```

The intention is to keep settings associated with the product identity rather than the original development name.

Settings should remain limited to appropriate application preferences and should not contain sensitive information.

---

# 32. Project Structure

The conceptual project structure is:

```text
Skim/
│
├── CMakeLists.txt
├── app_icon.rc
│
├── Resources/
│   ├── DocumentExtractor_256.png
│   ├── DocumentExtractor.ico
│   ├── checkmark.svg
│   └── DocumentExtractor.qss
│
└── src/
    ├── main.cpp
    ├── MainWindow.cpp
    ├── MainWindow.h
    ├── DocumentExtractor.cpp
    └── DocumentExtractor.h
```

The internal source naming can continue to use `DocumentExtractor` where it represents the extraction implementation.

---

# 33. `main.cpp`

The application entry point is intentionally small.

Its main responsibilities are:

1. create the Qt application;
2. construct the main window;
3. display it;
4. enter the Qt event loop.

The entry point should not contain document-extraction logic.

Conceptually:

```cpp
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}
```

---

# 34. `MainWindow.h`

The main-window header defines the GUI class and its state.

Responsibilities represented here include:

- UI controls
- extraction state
- selected files
- worker references
- slots for user actions
- log/progress handling
- drag-and-drop behavior
- completion handling

The header should remain focused on interface/state declarations rather than implementation details.

---

# 35. `MainWindow.cpp`

`MainWindow.cpp` contains the implementation of the application's presentation layer.

It handles:

- UI construction
- stylesheet/resource setup
- input selection
- drag-and-drop
- menus
- keyboard shortcuts
- extraction startup
- worker interaction
- progress updates
- logging
- completion dialogs
- reset operations
- settings

Long-running extraction should remain in the worker rather than being moved into this class.

---

# 36. `DocumentExtractor.h`

The worker header defines the extraction interface.

The worker is responsible for the actual document-processing operation and maintains processing state such as cancellation.

The header exposes the worker's processing operations and communication interface while keeping implementation details private.

---

# 37. `DocumentExtractor.cpp`

The worker implementation contains the extraction pipeline.

Its responsibilities include:

- discovering/processing inputs
- dispatching by file extension
- TXT processing
- Word processing
- PDF processing
- match counting
- output writing
- failure tracking
- empty-file tracking
- progress/log signaling
- cancellation checks
- resource cleanup

This is the core of Skim's extraction engine.

---

# 38. Build System

Skim uses CMake.

The project requires:

```cmake
find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    AxContainer
    Pdf
)
```

The executable target is:

```cmake
qt_add_executable(Skim
    ...
)
```

The required Qt libraries are linked through:

```cmake
target_link_libraries(Skim PRIVATE
    Qt6::Widgets
    Qt6::AxContainer
    Qt6::Pdf
)
```

The target is configured as a Windows GUI application:

```cmake
set_target_properties(Skim PROPERTIES
    WIN32_EXECUTABLE ON
)
```

---

# 39. Current Development Environment

The working development environment uses:

| Component | Version / location |
|---|---|
| C++ | C++17 |
| Qt | 6.11.2 |
| GCC | MinGW GCC 16.2.0 |
| CMake | 4.4.x |
| Generator | MinGW Makefiles |
| IDE | Visual Studio Code |
| Qt installation | `D:\Dev\Tools\Qt\6.11.2\mingw_64` |
| MinGW | `C:\msys64\mingw64\bin` |
| CMake | `C:\Program Files\CMake\bin` |

The absolute paths above describe the development machine and are not expected to be identical on other systems.

---

# 40. CMake Configuration

A known working configuration command is:

```powershell
cmake -S . -B build -G "MinGW Makefiles" `
    -DCMAKE_C_COMPILER="C:\msys64\mingw64\bin\gcc.exe" `
    -DCMAKE_CXX_COMPILER="C:\msys64\mingw64\bin\g++.exe" `
    -DCMAKE_PREFIX_PATH="D:\Dev\Tools\Qt\6.11.2\mingw_64"
```

Build with:

```powershell
cmake --build build
```

The executable target is:

```text
build/Skim.exe
```

---

# 41. Runtime Environment

When running the executable directly from the build directory, Qt's binary directory may need to be available on `PATH`.

For the current development environment:

```powershell
$env:Path = "D:\Dev\Tools\Qt\6.11.2\mingw_64\bin;$env:Path"
```

Then:

```powershell
.\build\Skim.exe
```

A final distributed build should package the required Qt runtime components rather than relying on a developer's local Qt installation.

---

# 42. Testing Strategy

Testing has focused on both extraction correctness and application behavior.

The validation matrix includes:

1. TXT-only extraction
2. PDF-only extraction
3. Word extraction
4. Mixed-format extraction
5. Recursive folder scanning
6. Non-recursive scanning
7. Case-insensitive search
8. Cancellation
9. Output file inside the input folder
10. Empty files
11. Bad/corrupt documents
12. Multiple-file selection
13. Multiple drag-and-drop operations
14. Files from different folders
15. Files from different drives
16. Duplicate-file prevention

---

# 43. Test Philosophy

The test strategy does not only ask whether a valid document produces a match.

It also checks how the application behaves when conditions are imperfect.

Important cases include:

```text
Valid file
    ↓
Matches found

Valid file
    ↓
No matches

Empty file
    ↓
Skipped / counted separately

Corrupt file
    ↓
Failed / reported

Mixed batch
    ↓
Good files continue despite failures

User cancellation
    ↓
Worker stops safely and reports cancellation
```

This is important because Skim is primarily a batch-processing tool.

---

# 44. Reliability Principles

Several design decisions are deliberate.

### One bad file should not destroy a batch

Failures are recorded and reported while other documents continue processing where possible.

### Cancellation should be safe

The worker is not forcefully killed.

### The UI should remain responsive

Extraction does not belong in the GUI thread.

### Output should not become an input

The generated output file is excluded from scanning.

### Empty is not the same as failed

The statistics distinguish empty documents from processing errors.

---

# 45. Deliberately Abandoned Feature: Clickable Log Files

An experiment was made to turn filenames displayed in the extraction log into clickable links.

The intended behavior was:

```text
Scanning: C:\Reports\report.pdf
                         ↓
                    clickable
                         ↓
              open report.pdf
```

The implementation required changing the log widget behavior and introducing anchor/HTML handling.

The experiment was abandoned because the feature was optional and began introducing unnecessary complexity into a stable part of the application.

The stable design therefore retains a plain-text extraction log.

This is an intentional product decision, not an unfinished requirement.

---

# 46. Product Branding

The application was originally named **DocumentExtractor** and was later renamed at the product level to **Skim**.

Current product-facing areas use Skim, including:

- executable target
- application title
- About dialog
- product presentation
- settings namespace

Some internal source/resource names retain `DocumentExtractor` for continuity.

A complete internal rename is not currently necessary unless it provides a maintenance benefit.

---

# 47. Current Roadmap

## Completed

### Core extraction

- [x] TXT extraction
- [x] Word extraction
- [x] PDF extraction
- [x] Case-insensitive search
- [x] Output generation

### Reliability

- [x] Background processing
- [x] Cancellation
- [x] Empty-file handling
- [x] Failed-file handling
- [x] Statistics
- [x] Completion summary

### GUI

- [x] Qt GUI
- [x] Card-based interface
- [x] QSS styling
- [x] Progress display
- [x] Extraction log
- [x] Menus
- [x] Keyboard shortcuts
- [x] Drag-and-drop
- [x] Application icon
- [x] Completion dialog

### Input workflows

- [x] Folder mode
- [x] Files mode
- [x] Multiple-file selection
- [x] Additive drag-and-drop
- [x] Duplicate prevention

### Productization

- [x] Skim product name
- [x] Skim executable target
- [x] Windows executable icon
- [x] Product-facing title/about presentation

---

# 48. Next Development Stage

The project has moved beyond the initial proof-of-concept stage.

The next priority is not adding arbitrary features. It is turning the stable implementation into a release-ready application.

The immediate areas are:

1. Final UI/product cleanup
2. Consistency checks
3. Additional reliability testing
4. Clean Release build
5. Runtime dependency deployment
6. Windows packaging
7. Repository documentation
8. First public release preparation

---

# 49. Potential Future Enhancements

Potential future work includes:

- OCR for image-only PDFs
- regular-expression searching
- whole-word matching
- phrase searching
- additional document formats
- CSV/JSON output
- advanced filtering
- persistent preferences
- installer support
- improved Windows deployment
- accessibility improvements
- profiling and performance optimization for very large batches

These are future possibilities rather than requirements for the current stable version.

---

# 50. Release Checklist

Before a public release, verify:

- [ ] Clean CMake configuration from a fresh build directory
- [ ] Successful Release build
- [ ] Complete extraction test matrix
- [ ] TXT verification
- [ ] Word verification
- [ ] PDF verification
- [ ] Corrupt-file verification
- [ ] Cancellation verification
- [ ] Multi-file verification
- [ ] Recursive/non-recursive verification
- [ ] Output-file exclusion verification
- [ ] Windows icon verification
- [ ] Product-name consistency
- [ ] About dialog verification
- [ ] Qt runtime deployment
- [ ] ActiveQt/Word environment verification
- [ ] PDF plugin/runtime verification
- [ ] Final README
- [ ] Final technical documentation
- [ ] Versioned release artifact

---

# 51. Maintenance Guidelines

When modifying Skim:

### Preserve the UI/worker boundary

Do not move expensive extraction work into `MainWindow`.

### Preserve safe cancellation

Avoid forcibly terminating worker threads.

### Preserve resource cleanup

Changes to Word/PDF processing must continue to close their resources on normal completion and cancellation.

### Keep format-specific code isolated

Changes to PDF processing should not require unrelated changes to TXT processing, for example.

### Test mixed batches

A change that works for one document may still fail when several formats are processed together.

### Test failure paths

Do not validate only successful documents.

### Prefer simple features

A convenience feature should not compromise a stable extraction path.

---

# 52. Conclusion

Skim has evolved from a VBA replacement into a standalone Windows document extraction application.

Its core architecture is intentionally straightforward:

```text
                    SKIM
                      │
          ┌───────────┴───────────┐
          │                       │
      MainWindow              Worker
          │                       │
      User input              Extraction
      GUI state               File handling
      Progress                TXT
      Logging                 Word
      Dialogs                 PDF
          │                       │
          └───────────┬───────────┘
                      │
                 Output report
```

The current implementation already provides the central functionality required by the project: multi-format extraction, batch workflows, background processing, cancellation, progress reporting, statistics, failure handling, and a polished Qt interface.

The next stage is therefore focused on **stability, packaging, consistency, and release readiness** rather than unnecessary expansion of the core extraction model.

---

## Appendix — Quick Reference

### Supported formats

```text
.txt
.doc
.docx
.pdf
```

### Build

```powershell
cmake -S . -B build -G "MinGW Makefiles" `
    -DCMAKE_C_COMPILER="C:\msys64\mingw64\bin\gcc.exe" `
    -DCMAKE_CXX_COMPILER="C:\msys64\mingw64\bin\g++.exe" `
    -DCMAKE_PREFIX_PATH="D:\Dev\Tools\Qt\6.11.2\mingw_64"

cmake --build build
```

### Run

```powershell
$env:Path = "D:\Dev\Tools\Qt\6.11.2\mingw_64\bin;$env:Path"
.\build\Skim.exe
```

### Shortcuts

```text
Ctrl + O          Choose scan location
Ctrl + Shift + O  Browse output file
Ctrl + Enter      Start extraction
Esc               Cancel extraction
Ctrl + Q          Exit
```

---

**Skim — Document extraction, without the hassle.**
