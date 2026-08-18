#pragma once
#include "update_checker.h"
#include <QObject>

class QAction;
class QMenu;
class QProgressDialog;
class QWidget;

namespace Kites
{

class UpdateDownloader;

/**
 * @brief Presents update checks to the user.
 *
 * This is the whole integration surface of the updater: call attachTo() once
 * with the Help menu and it wires up everything - a "Check for Updates..."
 * action plus a quiet check shortly after startup.
 *
 * "Update Now" downloads the new release, verifies it, replaces the running
 * executable in place and offers to restart. When the executable lives in a
 * root-owned location (a distro package) the replace is done through pkexec.
 * If no self-updatable asset exists for this platform, it falls back to opening
 * the release page.
 */
class UpdateService : public QObject
{
    Q_OBJECT
  public:
    /**
     * @brief Adds "Check for Updates..." to @p helpMenu and starts background checking.
     *
     * The returned service is parented to @p parentWindow, so it is cleaned up
     * with the window; callers can ignore the return value.
     */
    static UpdateService *attachTo(QMenu *helpMenu, QWidget *parentWindow);

    explicit UpdateService(QWidget *parentWindow);

    /**
     * @brief Runs a check.
     * @param silent When true (the automatic startup check), stays quiet unless
     *        there is genuinely an update: no "you are up to date" popup and no
     *        error popup if the machine happens to be offline.
     */
    void checkForUpdates(bool silent);

  private slots:
    void onUpdateAvailable(const ReleaseInfo &release);
    void onUpToDate();
    void onCheckFailed(const QString &errorMessage);

  private:
    void promptForUpdate(const ReleaseInfo &release);
    /// Downloads and installs @p release over the running executable.
    void startSelfUpdate(const ReleaseInfo &release);
    void finishSelfUpdate(const QString &downloadedPath);
    /// Guidance text matching how this copy was installed.
    QString updateInstructions() const;
    /// Rate-limits the automatic check so we don't hit the API on every launch.
    bool shouldRunAutomaticCheck() const;
    void recordAutomaticCheck();

    QWidget *m_parentWindow;
    UpdateChecker *m_checker;
    QAction *m_checkAction{nullptr};
    bool m_silent{true};

    UpdateDownloader *m_downloader{nullptr};
    QProgressDialog *m_progressDialog{nullptr};
    QString m_targetExecutablePath;
};

} // namespace Kites
