#include "update_installer.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QProcess>
#include <QStandardPaths>
#include <QStringList>
#include <QTemporaryDir>

namespace Kites
{
namespace
{
// A truncated download or an HTML error page would otherwise be happily
// installed and then fail to launch, so require a plausible executable size.
constexpr qint64 kMinimumPlausibleSize = 1024 * 1024; // 1 MiB

constexpr int kExtractTimeoutMs = 120000;
constexpr int kElevateTimeoutMs = 120000;

QString tr_(const char *text)
{
    return QObject::tr(text);
}

/**
 * Extracts a single member from a zip using whichever tool is available.
 * Qt ships no public zip reader, and vendoring one for this is not worth the
 * review surface, so we shell out - trying the common tools in turn.
 */
bool extractMember(const QString &zipPath, const QString &member, const QString &destinationPath,
                   QString &errorOut)
{
    struct Extractor
    {
        QString program;
        QStringList arguments;
    };

    const QList<Extractor> extractors = {
        // -p streams the member to stdout, which we redirect into the destination.
        {QStringLiteral("unzip"), {QStringLiteral("-p"), zipPath, member}},
        {QStringLiteral("bsdtar"), {QStringLiteral("-xOf"), zipPath, member}},
        {QStringLiteral("python3"),
         {QStringLiteral("-c"),
          QStringLiteral("import sys,zipfile;sys.stdout.buffer.write("
                         "zipfile.ZipFile(sys.argv[1]).read(sys.argv[2]))"),
          zipPath, member}},
    };

    QStringList tried;
    for (const Extractor &extractor : extractors)
    {
        if (QStandardPaths::findExecutable(extractor.program).isEmpty())
            continue;
        tried << extractor.program;

        QProcess process;
        process.setStandardOutputFile(destinationPath, QIODevice::Truncate);
        process.start(extractor.program, extractor.arguments);
        if (!process.waitForFinished(kExtractTimeoutMs))
        {
            process.kill();
            continue;
        }
        if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0 &&
            QFileInfo(destinationPath).size() > 0)
        {
            return true;
        }
    }

    errorOut = tried.isEmpty()
                   ? tr_("No tool available to unpack the release archive. Install 'unzip' "
                         "and try again, or download the release manually.")
                   : tr_("Could not unpack the release archive.");
    return false;
}

/// Lists zip members using whichever tool is available.
QStringList listMembers(const QString &zipPath)
{
    if (!QStandardPaths::findExecutable(QStringLiteral("python3")).isEmpty())
    {
        QProcess process;
        process.start(QStringLiteral("python3"),
                      {QStringLiteral("-c"),
                       QStringLiteral("import sys,zipfile;print('\\n'.join("
                                      "zipfile.ZipFile(sys.argv[1]).namelist()))"),
                       zipPath});
        if (process.waitForFinished(kExtractTimeoutMs) && process.exitCode() == 0)
        {
            return QString::fromUtf8(process.readAllStandardOutput())
                .split('\n', Qt::SkipEmptyParts);
        }
    }

    if (!QStandardPaths::findExecutable(QStringLiteral("unzip")).isEmpty())
    {
        QProcess process;
        process.start(QStringLiteral("unzip"), {QStringLiteral("-Z1"), zipPath});
        if (process.waitForFinished(kExtractTimeoutMs) && process.exitCode() == 0)
        {
            return QString::fromUtf8(process.readAllStandardOutput())
                .split('\n', Qt::SkipEmptyParts);
        }
    }

    return {};
}
} // namespace

bool UpdateInstaller::canElevate()
{
    return !QStandardPaths::findExecutable(QStringLiteral("pkexec")).isEmpty();
}

QString UpdateInstaller::extractAppImageFromZip(const QString &zipPath, QString &errorOut)
{
    const QStringList members = listMembers(zipPath);
    if (members.isEmpty())
    {
        errorOut = tr_("Could not read the release archive.");
        return {};
    }

    // The Linux release zip currently also contains the linuxdeploy build tools,
    // so match the Kites AppImage specifically rather than any *.AppImage.
    QString wanted;
    for (const QString &member : members)
    {
        const QString leaf = member.section('/', -1);
        if (leaf.endsWith(QStringLiteral(".AppImage"), Qt::CaseInsensitive) &&
            leaf.startsWith(QStringLiteral("Kites"), Qt::CaseInsensitive))
        {
            wanted = member.trimmed();
            break;
        }
    }

    if (wanted.isEmpty())
    {
        errorOut = tr_("The release archive did not contain a Kites AppImage.");
        return {};
    }

    const QString destination = zipPath + QStringLiteral(".extracted");
    if (!extractMember(zipPath, wanted, destination, errorOut))
    {
        QFile::remove(destination);
        return {};
    }

    return destination;
}

bool UpdateInstaller::looksLikeValidBinary(const QString &path, QString &errorOut)
{
    QFileInfo info(path);
    if (!info.exists() || info.size() < kMinimumPlausibleSize)
    {
        errorOut = tr_("The downloaded file is too small to be a valid Kites build.");
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        errorOut = tr_("The downloaded file could not be read.");
        return false;
    }
    const QByteArray magic = file.read(4);
    file.close();

    // AppImages are ELF executables, so this also rejects an HTML error page
    // that a proxy or an expired URL might have handed us.
    if (!magic.startsWith("\x7F" "ELF"))
    {
        errorOut = tr_("The downloaded file is not a Linux executable.");
        return false;
    }

    return true;
}

UpdateInstaller::Result UpdateInstaller::install(const QString &downloadedPath,
                                                 const QString &targetExecutablePath)
{
    Result result;

    QString candidate = downloadedPath;
    QString cleanupExtracted;

    if (downloadedPath.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive))
    {
        QString error;
        candidate = extractAppImageFromZip(downloadedPath, error);
        if (candidate.isEmpty())
        {
            result.errorMessage = error;
            return result;
        }
        cleanupExtracted = candidate;
    }

    QString error;
    if (!looksLikeValidBinary(candidate, error))
    {
        if (!cleanupExtracted.isEmpty())
            QFile::remove(cleanupExtracted);
        result.errorMessage = error;
        return result;
    }

    QFile::setPermissions(candidate,
                          QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                              QFile::ReadGroup | QFile::ExeGroup |
                              QFile::ReadOther | QFile::ExeOther);

    const QString backupPath = targetExecutablePath + QStringLiteral(".bak");
    const QFileInfo targetInfo(targetExecutablePath);
    const bool targetDirWritable = QFileInfo(targetInfo.absolutePath()).isWritable();

    if (targetDirWritable)
    {
        QFile::remove(backupPath);
        // Keep the old build so a bad update can be undone.
        if (!QFile::rename(targetExecutablePath, backupPath))
        {
            result.errorMessage = tr_("Could not move the current Kites binary aside.");
            if (!cleanupExtracted.isEmpty())
                QFile::remove(cleanupExtracted);
            return result;
        }

        if (!QFile::rename(candidate, targetExecutablePath))
        {
            // Put things back exactly as they were.
            QFile::rename(backupPath, targetExecutablePath);
            result.errorMessage = tr_("Could not move the new Kites binary into place.");
            if (!cleanupExtracted.isEmpty())
                QFile::remove(cleanupExtracted);
            return result;
        }

        result.success = true;
        return result;
    }

    // Target directory is not writable - typically /usr/bin from a distro
    // package. Ask polkit to do the swap for us.
    if (!canElevate())
    {
        result.errorMessage =
            tr_("Kites is installed in a location that needs administrator rights to "
                "modify, and 'pkexec' is not available to request them.");
        if (!cleanupExtracted.isEmpty())
            QFile::remove(cleanupExtracted);
        return result;
    }

    // $1 = new binary, $2 = installed path. Stage next to the target and finish
    // with mv so the swap itself is atomic: an interrupted update can never
    // leave a half-written executable in place.
    QProcess elevated;
    elevated.start(QStringLiteral("pkexec"),
                   {QStringLiteral("sh"), QStringLiteral("-c"),
                    QStringLiteral("cp -f -- \"$2\" \"$2.bak\" && "
                                   "cp -f -- \"$1\" \"$2.new\" && "
                                   "chmod 0755 -- \"$2.new\" && "
                                   "mv -f -- \"$2.new\" \"$2\""),
                    QStringLiteral("sh"), candidate, targetExecutablePath});

    if (!elevated.waitForFinished(kElevateTimeoutMs))
    {
        elevated.kill();
        result.errorMessage = tr_("The privileged update step timed out.");
    }
    else if (elevated.exitStatus() != QProcess::NormalExit || elevated.exitCode() != 0)
    {
        // Exit code 126 is polkit's "authorisation dialog dismissed".
        result.errorMessage = elevated.exitCode() == 126
                                  ? tr_("Administrator authorisation was declined.")
                                  : tr_("The privileged update step failed.");
    }
    else
    {
        result.success = true;
        result.usedElevation = true;
    }

    if (!cleanupExtracted.isEmpty())
        QFile::remove(cleanupExtracted);
    return result;
}

bool UpdateInstaller::relaunch(const QString &executablePath)
{
    return QProcess::startDetached(executablePath, {});
}

} // namespace Kites
