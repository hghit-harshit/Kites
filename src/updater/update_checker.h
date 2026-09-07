#pragma once
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

namespace Kites
{

/**
 * @brief How this copy of Kites was installed.
 *
 * Determines the advice we give the user: a distro-packaged build must be
 * updated through the package manager (the binary lives in a root-owned path
 * tracked by the package database), whereas an AppImage or a local build is
 * replaced by hand.
 */
enum class InstallKind
{
    SystemPackage, ///< Installed under /usr - apt/dnf/pacman owns it.
    AppImage,      ///< Running from an AppImage bundle.
    LocalBuild     ///< Run from a build tree or a user-prefix install.
};

/**
 * @brief A downloadable file attached to a release.
 */
struct ReleaseAsset
{
    QString name;
    QString url;   ///< browser_download_url
    qint64 size{0};
};

/**
 * @brief Details of a release found on GitHub.
 */
struct ReleaseInfo
{
    QString version;     ///< Tag with any leading 'v' stripped, e.g. "1.2.0".
    QString title;       ///< Human-readable release name.
    QString notes;       ///< Release body / changelog (Markdown as authored).
    QString downloadUrl; ///< Release page to send the user to.
    QList<ReleaseAsset> assets;

    /**
     * @brief Picks the asset to self-update from on Linux, or an empty asset
     *        if the release has nothing usable.
     *
     * Prefers a bare .AppImage (nothing to unpack). Otherwise falls back to the
     * kites-linux-*.zip that the current release workflow publishes.
     */
    ReleaseAsset linuxUpdateAsset() const;
};

/**
 * @brief Asks the GitHub Releases API whether a newer Kites exists.
 *
 * Network-only and UI-free: it reports what it found and lets the caller decide
 * how to present it (see UpdateService). Never downloads or modifies anything.
 */
class UpdateChecker : public QObject
{
    Q_OBJECT
  public:
    explicit UpdateChecker(QObject *parent = nullptr);

    /**
     * @brief Starts a check. Results arrive via the signals below.
     *
     * Only one check runs at a time; calling this while one is in flight is
     * ignored, so a startup check and an impatient menu click can't race.
     */
    void checkForUpdates();

    bool isChecking() const;

    /// Version of the running build, e.g. "1.0.0".
    static QString currentVersion();

    /// Best-effort detection of how this copy was installed.
    static InstallKind detectInstallKind();

    /**
     * @brief Compares two dotted version strings numerically.
     * @return >0 if a is newer than b, 0 if equal, <0 if a is older.
     *
     * Handles a leading 'v' and differing component counts ("1.2" == "1.2.0").
     * A pre-release suffix ("1.2.0-rc1") sorts before the plain release, per
     * the usual semver rule.
     */
    static int compareVersions(const QString &a, const QString &b);

  signals:
    /// A release newer than the running build is available.
    void updateAvailableSignal(const ReleaseInfo &release);
    /// The check succeeded and this build is current (or ahead of) the latest.
    void upToDateSignal();
    /// The check could not be completed (offline, rate-limited, bad response).
    void checkFailedSignal(const QString &errorMessage);

  private slots:
    void handleReply(QNetworkReply *reply);

  private:
    QNetworkAccessManager *m_network;
    bool m_checking{false};
};

} // namespace Kites
