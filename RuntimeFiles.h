#pragma once

// RuntimeFiles.h
//
// Utility for ensuring required runtime JSON/config files exist next to the
// executable at run time.
//
// Typical use:
//   - During development: editable files live in the project root, runtime/, or config/
//   - During execution: binary runs from build-qmake-<host>/bin or a shadow build
//
// This helper:
//   1. Checks if the file already exists in the binary directory
//   2. If not, searches a list of candidate source directories
//   3. Copies the first match into the binary directory
//   4. Logs success/failure
//
// This avoids hardcoding absolute paths and makes the binary easier to move.

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

#include <iostream>

// ---------------------------------------------------------------------------
// ensureRuntimeFile
//
// Ensures that `fileName` exists in the application directory.
//
// If missing:
//   - Searches `candidateSourceDirs` in order
//   - Copies the first found instance into the app directory
//
// Parameters:
//   fileName            : e.g. "config.json", "viz.json"
//   candidateSourceDirs : directories to search, in priority order
//
// Returns:
//   true  -> file exists or was successfully copied
//   false -> file not found or copy failed
// ---------------------------------------------------------------------------
static bool ensureRuntimeFile(const QString &fileName,
                              const QStringList &candidateSourceDirs)
{
    // Directory where the executable is running.
    const QString appDir = QCoreApplication::applicationDirPath();

    // Target path: file next to the binary.
    const QString dst = QDir(appDir).absoluteFilePath(fileName);

    // Fast path: already exists -> nothing to do.
    if (QFileInfo::exists(dst))
        return true;

    // Search candidate directories for the file.
    for (const QString &srcDir : candidateSourceDirs)
    {
        const QString src = QDir(srcDir).absoluteFilePath(fileName);

        if (!QFileInfo::exists(src))
            continue;

        // Ensure destination directory exists. It normally already does, but
        // this keeps the helper safe for custom launch locations.
        QDir().mkpath(appDir);

        if (QFile::copy(src, dst))
        {
            std::cout << "[RuntimeFiles] Copied "
                      << src.toStdString()
                      << " -> "
                      << dst.toStdString()
                      << "\n";
            return true;
        }

        std::cerr << "[RuntimeFiles] Failed to copy "
                  << src.toStdString()
                  << " -> "
                  << dst.toStdString()
                  << "\n";
    }

    std::cerr << "[RuntimeFiles] Missing required runtime file: "
              << fileName.toStdString()
              << "\n";

    return false;
}
