#include "update_service.h"
#include "kites_version.h"
#include "update_downloader.h"
#include "update_installer.h"

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QWidget>

namespace Kites
{
namespace
{
// Own keys in the existing "Kites/Kites" settings file, so this module adds no
// coupling to AppSettings.
constexpr const char *kLastCheckKey = "updates/lastAutomaticCheck";
constexpr int kAutomaticCheckIntervalHours = 24;

// Let the main window finish showing before firing off a network request.
constexpr int kStartupCheckDelayMs = 3000;
} // namespace

UpdateService::UpdateService(QWidget *parentWindow)
    : QObject(parentWindow), m_parentWindow(parentWindow), m_checker(new UpdateChecker(this))
{
    connect(m_checker, &UpdateChecker::updateAvailableSignal, this,
            &UpdateService::onUpdateAvailable);
    connect(m_checker, &UpdateChecker::upToDateSignal, this, &UpdateService::onUpToDate);
    connect(m_checker, &UpdateChecker::checkFailedSignal, this, &UpdateService::onCheckFailed);
}

UpdateService *UpdateService::attachTo(QMenu *helpMenu, QWidget *parentWindow)
{
    auto *service = new UpdateService(parentWindow);

    service->m_checkAction = new QAction(tr("Check for Updates..."), parentWindow);
    connect(service->m_checkAction, &QAction::triggered, service,
            [service]() { service->checkForUpdates(/*silent=*/false); });
    helpMenu->addAction(service->m_checkAction);

    // Quiet check a few seconds after launch, at most once a day.
    QTimer::singleShot(kStartupCheckDelayMs, service,
                       [service]()
                       {
                           if (service->shouldRunAutomaticCheck())
                           {
                               service->recordAutomaticCheck();
                               service->checkForUpdates(/*silent=*/true);
                           }
                       });

    return service;
}

bool UpdateService::shouldRunAutomaticCheck() const
{
    QSettings settings("Kites", "Kites");
    const QDateTime lastCheck = settings.value(kLastCheckKey).toDateTime();
    if (!lastCheck.isValid())
        return true;

    return lastCheck.secsTo(QDateTime::currentDateTime()) >= kAutomaticCheckIntervalHours * 3600;
}

void UpdateService::recordAutomaticCheck()
{
    QSettings settings("Kites", "Kites");
    settings.setValue(kLastCheckKey, QDateTime::currentDateTime());
}

void UpdateService::checkForUpdates(bool silent)
{
    m_silent = silent;

    if (m_checkAction)
        m_checkAction->setEnabled(false);

    m_checker->checkForUpdates();
}

void UpdateService::onUpdateAvailable(const ReleaseInfo &release)
{
    if (m_checkAction)
        m_checkAction->setEnabled(true);

    promptForUpdate(release);
}

void UpdateService::onUpToDate()
{
    if (m_checkAction)
        m_checkAction->setEnabled(true);

    if (m_silent)
        return; // nothing to say on a background check

    QMessageBox::information(
        m_parentWindow, tr("No Updates Available"),
        tr("Kites %1 is the latest version.").arg(UpdateChecker::currentVersion()));
}

void UpdateService::onCheckFailed(const QString &errorMessage)
{
    if (m_checkAction)
        m_checkAction->setEnabled(true);

    if (m_silent)
        return; // offline at startup is not worth interrupting anyone over

    QMessageBox::warning(m_parentWindow, tr("Update Check Failed"),
                         tr("Could not check for updates.\n\n%1").arg(errorMessage));
}

QString UpdateService::updateInstructions() const
{
    switch (UpdateChecker::detectInstallKind())
    {
    case InstallKind::SystemPackage:
        return tr("Kites is installed system-wide, so updating replaces a file owned by your "
                  "package manager and you will be asked for administrator confirmation. Your "
                  "package manager will not know about this change, so it may replace Kites "
                  "again on its next upgrade.");
    case InstallKind::AppImage:
        return tr("The current AppImage will be replaced with the new version.");
    case InstallKind::LocalBuild:
        break;
    }
    return tr("The Kites executable you are running will be replaced with the new version. "
              "The previous build is kept alongside it as 'Kites.bak'.");
}

void UpdateService::promptForUpdate(const ReleaseInfo &release)
{
    QMessageBox box(m_parentWindow);
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle(tr("Update Available"));
    box.setText(tr("<b>Kites %1 is available.</b>").arg(release.version));
    box.setInformativeText(tr("You are running version %1.\n\n%2")
                               .arg(UpdateChecker::currentVersion(), updateInstructions()));

    if (!release.notes.trimmed().isEmpty())
    {
        const QString heading =
            release.title.isEmpty() ? release.version : release.title;
        box.setDetailedText(tr("%1\n\n%2").arg(heading, release.notes.trimmed()));
    }

    QPushButton *updateButton = box.addButton(tr("Update Now"), QMessageBox::AcceptRole);
    box.addButton(tr("Later"), QMessageBox::RejectRole);
    box.setDefaultButton(updateButton);

    box.exec();

    if (box.clickedButton() != updateButton)
        return;

    // Nothing we can install automatically (no Linux asset, or a platform this
    // build does not self-update on): fall back to the release page.
    if (release.linuxUpdateAsset().url.isEmpty())
    {
        QDesktopServices::openUrl(QUrl(release.downloadUrl));
        return;
    }

    startSelfUpdate(release);
}

void UpdateService::startSelfUpdate(const ReleaseInfo &release)
{
    const ReleaseAsset asset = release.linuxUpdateAsset();

    m_targetExecutablePath = QCoreApplication::applicationFilePath();
    // When running as an AppImage, applicationFilePath() points inside the
    // mounted squashfs; the file to replace is the .AppImage itself.
    const QString appImagePath = qEnvironmentVariable("APPIMAGE");
    if (!appImagePath.isEmpty())
        m_targetExecutablePath = appImagePath;

    // Download beside the target so the final install is a same-filesystem
    // rename; fall back to the temp dir when that directory is not writable
    // (the privileged path copies from there instead).
    const QString targetDir = QFileInfo(m_targetExecutablePath).absolutePath();
    const QString stagingDir = QFileInfo(targetDir).isWritable()
                                   ? targetDir
                                   : QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString destination =
        QDir(stagingDir).filePath(QStringLiteral(".kites-update-download-%1").arg(asset.name));

    m_progressDialog = new QProgressDialog(tr("Downloading Kites %1...").arg(release.version),
                                           tr("Cancel"), 0, 100, m_parentWindow);
    m_progressDialog->setWindowTitle(tr("Updating Kites"));
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setAutoClose(false);
    m_progressDialog->setAutoReset(false);
    m_progressDialog->setMinimumDuration(0);
    m_progressDialog->setValue(0);

    m_downloader = new UpdateDownloader(this);

    connect(m_downloader, &UpdateDownloader::progressSignal, this,
            [this](qint64 received, qint64 total)
            {
                if (!m_progressDialog)
                    return;
                if (total <= 0)
                {
                    m_progressDialog->setRange(0, 0); // indeterminate
                    return;
                }
                m_progressDialog->setRange(0, 100);
                m_progressDialog->setValue(static_cast<int>(received * 100 / total));
                m_progressDialog->setLabelText(tr("Downloading Kites... %1 MB of %2 MB")
                                                   .arg(received / (1024 * 1024))
                                                   .arg(total / (1024 * 1024)));
            });

    connect(m_progressDialog, &QProgressDialog::canceled, this,
            [this]()
            {
                if (m_downloader)
                    m_downloader->cancel();
            });

    connect(m_downloader, &UpdateDownloader::finishedSignal, this,
            &UpdateService::finishSelfUpdate);

    connect(m_downloader, &UpdateDownloader::failedSignal, this,
            [this](const QString &errorMessage)
            {
                if (m_progressDialog)
                {
                    m_progressDialog->close();
                    m_progressDialog->deleteLater();
                    m_progressDialog = nullptr;
                }
                QMessageBox::warning(m_parentWindow, tr("Update Failed"),
                                     tr("Could not download the update.\n\n%1").arg(errorMessage));
            });

    m_downloader->start(asset, destination);
}

void UpdateService::finishSelfUpdate(const QString &downloadedPath)
{
    if (m_progressDialog)
    {
        m_progressDialog->setRange(0, 0); // unpacking/installing is not measurable
        m_progressDialog->setLabelText(tr("Installing..."));
        m_progressDialog->setCancelButton(nullptr);
        QApplication::processEvents();
    }

    const UpdateInstaller::Result result =
        UpdateInstaller::install(downloadedPath, m_targetExecutablePath);

    QFile::remove(downloadedPath);

    if (m_progressDialog)
    {
        m_progressDialog->close();
        m_progressDialog->deleteLater();
        m_progressDialog = nullptr;
    }

    if (!result.success)
    {
        QMessageBox::warning(m_parentWindow, tr("Update Failed"),
                             tr("Kites could not be updated.\n\n%1").arg(result.errorMessage));
        return;
    }

    QMessageBox box(m_parentWindow);
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle(tr("Update Installed"));
    box.setText(tr("<b>Kites has been updated.</b>"));
    box.setInformativeText(tr("Restart now to use the new version. The previous build was "
                              "kept as '%1.bak'.")
                               .arg(QFileInfo(m_targetExecutablePath).fileName()));

    QPushButton *restartButton = box.addButton(tr("Restart Now"), QMessageBox::AcceptRole);
    box.addButton(tr("Later"), QMessageBox::RejectRole);
    box.setDefaultButton(restartButton);
    box.exec();

    if (box.clickedButton() != restartButton)
        return;

    if (UpdateInstaller::relaunch(m_targetExecutablePath))
        QApplication::quit();
    else
        QMessageBox::warning(m_parentWindow, tr("Restart Failed"),
                             tr("Could not restart Kites automatically. Please close and "
                                "reopen it to use the new version."));
}

} // namespace Kites
