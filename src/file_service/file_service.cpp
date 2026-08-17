#include "file_service.h"
#include "config/app_settings.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>
#include <QWidget>

namespace Kites
{
namespace
{
constexpr const char *kFileFilter = "Assembly Files (*.asm *.s);;All Files (*)";

// How often the crash-recovery mirror is refreshed. Short enough that little is
// lost in a crash, long enough to be irrelevant to typing latency.
constexpr int kRecoveryIntervalMs = 10000;

QString appDataDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}
} // namespace

FileService::FileService(QWidget *parentWidget, QObject *parent)
    : QObject(parent), m_parentWidget(parentWidget), m_recoveryTimer(new QTimer(this)),
      m_autosaveTimer(new QTimer(this))
{
    // Recovery runs on a repeating timer and is never user-configurable: it is
    // cheap and it is the only thing protecting an untitled buffer.
    m_recoveryTimer->setInterval(kRecoveryIntervalMs);
    connect(m_recoveryTimer, &QTimer::timeout, this, &FileService::onRecoveryTimerTimeout);
    m_recoveryTimer->start();

    // Autosave is a single-shot restarted on each keystroke, so it fires once
    // typing has actually paused rather than mid-word.
    m_autosaveTimer->setSingleShot(true);
    connect(m_autosaveTimer, &QTimer::timeout, this, &FileService::onAutosaveTimerTimeout);

    connect(&AppSettings::getInstance(), &AppSettings::autosaveSettingsChangedSignal, this,
            &FileService::applyAutosaveSettings);
    applyAutosaveSettings();
}

FileService::~FileService() = default;

void FileService::setDocumentAccessors(std::function<QString()> readText,
                                       std::function<void(const QString &)> writeText)
{
    m_readText = std::move(readText);
    m_writeText = std::move(writeText);
}

QString FileService::currentFilePath() const
{
    return m_currentFilePath;
}

bool FileService::isDirty() const
{
    return m_dirty;
}

QString FileService::displayName() const
{
    if (m_currentFilePath.isEmpty())
        return tr("Untitled");
    return QFileInfo(m_currentFilePath).fileName();
}

void FileService::setCurrentFilePath(const QString &path)
{
    m_currentFilePath = path;
    emit documentStateChangedSignal();
}

void FileService::setDirty(bool dirty)
{
    if (m_dirty == dirty)
        return;
    m_dirty = dirty;
    emit documentStateChangedSignal();
}

void FileService::markDirty()
{
    if (m_loading)
        return; // our own load/restore, not a user edit

    setDirty(true);

    if (AppSettings::getInstance().autosaveMode() == AutosaveMode::AfterDelay)
        m_autosaveTimer->start(); // restart: fires only after typing pauses
}

void FileService::applyAutosaveSettings()
{
    const AppSettings &settings = AppSettings::getInstance();
    m_autosaveTimer->setInterval(settings.autosaveDelaySeconds() * 1000);
    if (settings.autosaveMode() == AutosaveMode::Off)
        m_autosaveTimer->stop();
}

// ---------------------------------------------------------------------------
// Open / Save
// ---------------------------------------------------------------------------
void FileService::open()
{
    if (!maybeSaveChanges())
        return;

    const QString path = QFileDialog::getOpenFileName(
        m_parentWidget, tr("Open File"), QFileInfo(m_currentFilePath).absolutePath(), kFileFilter);
    if (path.isEmpty())
        return;

    openPath(path);
}

void FileService::openPath(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(m_parentWidget, tr("Open Failed"),
                             tr("Could not open %1:\n\n%2").arg(path, file.errorString()));
        return;
    }

    QTextStream in(&file);
    const QString content = in.readAll();
    file.close();

    if (m_writeText)
    {
        m_loading = true;
        m_writeText(content);
        m_loading = false;
    }

    setCurrentFilePath(path);
    setDirty(false);
    clearRecoverySnapshot();
    emit statusMessageSignal(tr("Opened %1").arg(QFileInfo(path).fileName()));
}

bool FileService::save()
{
    if (m_currentFilePath.isEmpty())
        return saveAs();

    QString error;
    if (!writeToPath(m_currentFilePath, error))
    {
        QMessageBox::warning(m_parentWidget, tr("Save Failed"),
                             tr("Could not save %1:\n\n%2").arg(m_currentFilePath, error));
        return false;
    }

    setDirty(false);
    clearRecoverySnapshot();
    emit statusMessageSignal(tr("Saved %1").arg(displayName()));
    return true;
}

bool FileService::saveAs()
{
    const QString suggested =
        m_currentFilePath.isEmpty() ? QStringLiteral("untitled.asm") : m_currentFilePath;

    const QString path =
        QFileDialog::getSaveFileName(m_parentWidget, tr("Save File As"), suggested, kFileFilter);
    if (path.isEmpty())
        return false;

    QString error;
    if (!writeToPath(path, error))
    {
        QMessageBox::warning(m_parentWidget, tr("Save Failed"),
                             tr("Could not save %1:\n\n%2").arg(path, error));
        return false;
    }

    setCurrentFilePath(path);
    setDirty(false);
    clearRecoverySnapshot();
    emit statusMessageSignal(tr("Saved %1").arg(displayName()));
    return true;
}

bool FileService::writeToPath(const QString &path, QString &errorOut) const
{
    if (!m_readText)
    {
        errorOut = tr("The editor is not available.");
        return false;
    }

    // QSaveFile writes to a temporary and renames on commit, so an interrupted
    // save can never leave the user with a truncated source file.
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        errorOut = file.errorString();
        return false;
    }

    QTextStream out(&file);
    out << m_readText();
    out.flush();

    if (!file.commit())
    {
        errorOut = file.errorString();
        return false;
    }
    return true;
}

bool FileService::maybeSaveChanges()
{
    if (!m_dirty)
        return true;

    const QMessageBox::StandardButton choice = QMessageBox::warning(
        m_parentWidget, tr("Unsaved Changes"),
        tr("%1 has unsaved changes.\n\nDo you want to save them?").arg(displayName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);

    switch (choice)
    {
    case QMessageBox::Save:
        return save();
    case QMessageBox::Discard:
        return true;
    default:
        return false;
    }
}

// ---------------------------------------------------------------------------
// Autosave to the real file
// ---------------------------------------------------------------------------
void FileService::onAutosaveTimerTimeout()
{
    if (AppSettings::getInstance().autosaveMode() != AutosaveMode::AfterDelay)
        return;
    if (!m_dirty || m_currentFilePath.isEmpty())
        return; // never invent a path behind the user's back

    QString error;
    if (!writeToPath(m_currentFilePath, error))
    {
        // Autosave failing is not worth a modal interruption; the recovery
        // snapshot still has the content and Ctrl+S will report properly.
        emit statusMessageSignal(tr("Autosave failed: %1").arg(error));
        return;
    }

    setDirty(false);
    clearRecoverySnapshot();
    emit statusMessageSignal(tr("Autosaved %1").arg(displayName()));
}

// ---------------------------------------------------------------------------
// Crash recovery
// ---------------------------------------------------------------------------
QString FileService::recoveryFilePath() const
{
    return QDir(appDataDir()).filePath(QStringLiteral("recovery/buffer.asm"));
}

QString FileService::recoveryMetaPath() const
{
    return QDir(appDataDir()).filePath(QStringLiteral("recovery/buffer.meta"));
}

void FileService::onRecoveryTimerTimeout()
{
    if (m_dirty)
        writeRecoverySnapshot();
}

void FileService::writeRecoverySnapshot()
{
    if (!m_readText)
        return;

    const QString path = recoveryFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return; // recovery is best-effort; never interrupt the user over it

    QTextStream out(&file);
    out << m_readText();
    out.flush();
    if (!file.commit())
        return;

    // Remember which file this snapshot belonged to, so the restore prompt can
    // name it (and so an untitled buffer is recognisable as such).
    QSaveFile meta(recoveryMetaPath());
    if (meta.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream metaOut(&meta);
        metaOut << m_currentFilePath << "\n"
                << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
        metaOut.flush();
        meta.commit();
    }
}

void FileService::clearRecoverySnapshot()
{
    QFile::remove(recoveryFilePath());
    QFile::remove(recoveryMetaPath());
}

void FileService::offerRecoveryIfPresent()
{
    const QString path = recoveryFilePath();
    if (!QFileInfo::exists(path))
        return;

    QString originalPath;
    QString savedAt;
    QFile meta(recoveryMetaPath());
    if (meta.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream metaIn(&meta);
        originalPath = metaIn.readLine();
        savedAt = metaIn.readLine();
        meta.close();
    }

    const QString what =
        originalPath.isEmpty() ? tr("an untitled buffer") : QFileInfo(originalPath).fileName();

    const QMessageBox::StandardButton choice = QMessageBox::question(
        m_parentWidget, tr("Recover Unsaved Work"),
        tr("Kites closed unexpectedly with unsaved changes to %1%2.\n\nRestore them?")
            .arg(what, savedAt.isEmpty() ? QString() : tr(" (last autosaved %1)").arg(savedAt)),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (choice != QMessageBox::Yes)
    {
        clearRecoverySnapshot();
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(m_parentWidget, tr("Recovery Failed"),
                             tr("Could not read the recovered file."));
        return;
    }
    QTextStream in(&file);
    const QString content = in.readAll();
    file.close();

    if (m_writeText)
    {
        m_loading = true;
        m_writeText(content);
        m_loading = false;
    }

    // Keep the original path so Ctrl+S goes back to the right file, but stay
    // dirty: the recovered content has not been written there yet.
    setCurrentFilePath(originalPath);
    setDirty(true);
    emit statusMessageSignal(tr("Recovered unsaved changes"));
}

void FileService::shutdown()
{
    m_recoveryTimer->stop();
    m_autosaveTimer->stop();
    clearRecoverySnapshot();
}

} // namespace Kites
