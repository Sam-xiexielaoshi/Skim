# Skim

### Fast, simple document extraction for Windows

Skim is a Windows desktop application for searching multiple documents for a specific term and extracting all matching lines into a consolidated output file.

It was originally created as a replacement for a VBA-based document scanning workflow and has evolved into a standalone C++/Qt application with support for TXT, Word, and PDF documents.

---

## ✨ Features

- 📄 **Multiple document formats**
  - `.txt`
  - `.doc`
  - `.docx`
  - `.pdf`
- 🔎 **Case-insensitive search**
- 📁 **Folder scanning**
- 📂 **Recursive subfolder scanning**
- 📄 **Individual file selection**
- 📚 **Multiple-file scanning**
- 🖱️ **Drag and drop**
- 🔄 **Background document processing**
- ⏹️ **Cooperative cancellation**
- 📊 **Live progress reporting**
- 📝 **Live extraction log**
- 📈 **Extraction statistics**
- ⚠️ **Failed-file reporting**
- 🗂️ **Empty-file detection**
- 🛡️ **Corrupt/unreadable document handling**
- 🎨 **Modern Qt-based interface**
- ⌨️ **Keyboard shortcuts**
- 🪟 **Native Windows application**

---

## 🖥️ How It Works

Skim follows a simple workflow:

```text
        Select documents
               │
               ▼
        Enter search term
               │
               ▼
        Start extraction
               │
               ▼
      ┌───────────────────┐
      │   Skim Worker     │
      └─────────┬─────────┘
                │
       ┌────────┼────────┐
       ▼        ▼        ▼
      TXT     Word      PDF
       │        │        │
       └────────┼────────┘
                ▼
        Search for matches
                │
                ▼
       Write matching lines
                │
                ▼
        Generate summary
```

The application processes documents in a background worker so that the graphical interface remains responsive during extraction.

---

# 📁 Scan Modes

Skim provides two ways to select documents.

## Folder Mode

Select a directory and allow Skim to scan supported documents inside it.

You can choose whether to include subfolders.

```text
Folder
 ├── document1.pdf
 ├── document2.docx
 ├── notes.txt
 │
 └── Subfolder
      ├── report.pdf
      └── data.docx
```

When **Include subfolders** is enabled, documents inside nested directories are also processed.

---

## Files Mode

Files mode allows you to select individual documents instead of an entire folder.

You can:

- Select one file
- Select multiple files
- Drag multiple files into Skim
- Add files through multiple drag-and-drop operations
- Select files from different folders
- Select files from different drives

Duplicate files are automatically ignored.

---

# 📄 Supported Formats

| Format | Extraction Method |
|--------|-------------------|
| `.txt` | Native text processing |
| `.doc` | Microsoft Word automation |
| `.docx` | Microsoft Word automation |
| `.pdf` | Qt PDF text extraction |

### PDF limitation

Skim currently extracts embedded text from PDFs.

Image-only/scanned PDFs are **not OCR processed**.

OCR support may be considered as a future feature.

---

# 🔎 Searching

Searches are performed **case-insensitively**.

For example, searching for:

```text
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

Matching lines are written to the output file.

---

# 📊 Extraction Summary

After an extraction finishes, Skim provides a summary containing information such as:

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

If the operation is cancelled, the result is reported as:

```text
EXTRACTION CANCELLED
```

This makes it easy to understand what happened during a batch operation.

---

# ⚡ Background Processing

Document extraction is performed outside the main GUI thread.

This means the interface remains responsive while Skim is processing documents.

The worker communicates with the GUI through Qt's signal/slot system.

The architecture is approximately:

```text
MainWindow
    │
    │ starts extraction
    ▼
DocumentExtractorWorker
    │
    ├── TXT processing
    ├── Word processing
    ├── PDF processing
    ├── Output generation
    ├── Progress reporting
    └── Cancellation
```

---

# ⏹️ Cancellation

Extraction can be cancelled while a scan is running.

Skim uses **cooperative cancellation** rather than forcibly terminating the worker thread.

The worker periodically checks a cancellation flag and stops safely between processing operations.

This allows resources such as PDF documents, output streams, and Microsoft Word automation objects to be cleaned up correctly.

---

# 🛠️ Technology Stack

| Technology | Purpose |
|------------|---------|
| **C++17** | Application logic |
| **Qt 6.11.2** | GUI framework |
| **Qt Widgets** | User interface |
| **Qt PDF** | PDF text extraction |
| **ActiveQt / QAxContainer** | Microsoft Word automation |
| **CMake** | Build system |
| **MinGW / GCC** | C++ compiler |
| **Visual Studio Code** | Development environment |

---

# 🏗️ Project Structure

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

The internal `DocumentExtractor` naming is retained in parts of the source code for continuity with the original project.

---

# 🔨 Building From Source

## Requirements

The current development environment uses:

- Windows
- C++17-compatible compiler
- MinGW
- CMake
- Qt 6.11.2
- Qt Widgets
- Qt PDF
- Qt ActiveQt
- Microsoft Word for `.doc` / `.docx` processing

### Development toolchain

```text
Qt:
D:\Dev\Tools\Qt\6.11.2\mingw_64

MinGW:
C:\msys64\mingw64\bin

CMake:
C:\Program Files\CMake\bin
```

These paths are specific to the development environment and may need to be changed on another machine.

---

## Configure

From the project directory:

```powershell
cmake -S . -B build -G "MinGW Makefiles" `
    -DCMAKE_C_COMPILER="C:\msys64\mingw64\bin\gcc.exe" `
    -DCMAKE_CXX_COMPILER="C:\msys64\mingw64\bin\g++.exe" `
    -DCMAKE_PREFIX_PATH="D:\Dev\Tools\Qt\6.11.2\mingw_64"
```

## Build

```powershell
cmake --build build
```

The executable is generated as:

```text
build/Skim.exe
```

---

# ▶️ Running From the Build Directory

When running directly from the build directory, the Qt binary directory may need to be added to `PATH`:

```powershell
$env:Path = "D:\Dev\Tools\Qt\6.11.2\mingw_64\bin;$env:Path"
```

Then:

```powershell
.\build\Skim.exe
```

---

# ⌨️ Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl + O` | Choose scan location |
| `Ctrl + Shift + O` | Browse output file |
| `Ctrl + Enter` | Start extraction |
| `Esc` | Cancel extraction |
| `Ctrl + Q` | Exit |

---

# 🧪 Testing

The application has been tested against the major workflows required by the project.

Current validation includes:

- TXT-only extraction
- PDF-only extraction
- Word extraction
- Mixed-format extraction
- Recursive folder scanning
- Non-recursive scanning
- Case-insensitive searching
- Cancellation
- Output file inside the input folder
- Empty documents
- Corrupt/unreadable documents
- Multiple-file selection
- Multiple drag-and-drop operations
- Files from different folders
- Files from different drives
- Duplicate-file prevention

---

# 🗺️ Roadmap

## Core Extraction

- [x] TXT extraction
- [x] Word extraction
- [x] PDF extraction
- [x] Case-insensitive search
- [x] Output generation

## Reliability

- [x] Background processing
- [x] Cancellation
- [x] Empty-file handling
- [x] Failed-file handling
- [x] Extraction statistics
- [x] Completion summary

## User Interface

- [x] Qt GUI
- [x] Custom QSS styling
- [x] Progress display
- [x] Extraction log
- [x] Menus
- [x] Keyboard shortcuts
- [x] Drag and drop
- [x] Application icon
- [x] Completion dialog

## Input Expansion

- [x] Folder mode
- [x] Files mode
- [x] Multiple-file selection
- [x] Additive drag and drop
- [x] Duplicate prevention

## Productization

- [x] Rename application to Skim
- [x] Skim application title
- [x] Skim executable target
- [x] Windows application icon
- [ ] Final product cleanup
- [ ] Release packaging
- [ ] Public release

---

# 🚀 Future Ideas

Potential future improvements include:

- OCR support for scanned/image-only PDFs
- Regular-expression search
- Whole-word matching
- Phrase searching
- Additional document formats
- CSV/JSON output
- More advanced filtering
- Persistent application preferences
- Improved Windows packaging
- Installer/distribution support
- Accessibility improvements
- Performance optimization for very large document collections

These features are intentionally not part of the current core implementation.

---

# 🎯 Project Philosophy

Skim is designed around a simple idea:

> **Make document searching fast, simple, and reliable.**

The application started as a replacement for a repetitive VBA workflow. Instead of requiring users to run a macro and manage its limitations, Skim provides a dedicated interface for selecting documents, searching them, and receiving a consolidated result.

The project prioritizes:

- Reliability over unnecessary complexity
- Clear feedback over hidden processing
- Safe cancellation over forced termination
- Batch processing over one-file-at-a-time workflows
- A clean GUI over a collection of disconnected utilities

---

# 📜 License

License information will be added before the first public release.

---

# 👨‍💻 Development

Skim is actively being developed as a C++/Qt desktop application.

The project is currently focused on:

1. Final UI and product cleanup
2. Reliability testing
3. Release packaging
4. Documentation
5. Preparing the first distributable version

---

<p align="center">
  <b>Skim</b><br>
  Document extraction, without the hassle.
</p>
