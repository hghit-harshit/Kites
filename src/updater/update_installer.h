#pragma once
#include <QString>

namespace Kites
{

/**
 * @brief Puts a downloaded build in place of the running executable.
 *
 * Replacing a running binary is safe on Linux: rename() swaps the directory
 * entry while the running process keeps its original inode open, so the live
 * process is unaffected and the new file is picked up on next launch.
 *
 * The previous binary is kept alongside as "<name>.bak" so a bad update can be
 * rolled back by hand (and is restored automatically if the swap half-fails).
 */
class UpdateInstaller
{
  public:
    struct Result
    {
        bool success{false};
        QString errorMessage;
        /// True when the replace needed (and got) elevated privileges.
        bool usedElevation{false};
    };

    /**
     * @brief Installs @p downloadedPath over @p targetExecutablePath.
     *
     * If the download is a .zip, the Kites AppImage is extracted from it first.
     * The candidate is validated before anything is overwritten.
     */
    static Result install(const QString &downloadedPath, const QString &targetExecutablePath);

    /**
     * @brief Relaunches @p executablePath as a detached process.
     * @return true if the new process was started; the caller should then quit.
     */
    static bool relaunch(const QString &executablePath);

    /// True if a privileged replace is possible (i.e. pkexec is present).
    static bool canElevate();

  private:
    /// Pulls Kites*.AppImage out of a release zip. Returns the extracted path.
    static QString extractAppImageFromZip(const QString &zipPath, QString &errorOut);
    /// Rejects HTML error pages, truncated downloads and non-executables.
    static bool looksLikeValidBinary(const QString &path, QString &errorOut);
};

} // namespace Kites
