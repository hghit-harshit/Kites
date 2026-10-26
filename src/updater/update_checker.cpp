#include "update_checker.h"
#include "kites_version.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcessEnvironment>
#include <QUrl>

namespace Kites
{
namespace
{
// The GitHub "latest release" endpoint. Excludes drafts and pre-releases,
// which is what we want - users should not be nagged towards an rc build.
QString latestReleaseEndpoint()
{
    return QStringLiteral("https://api.github.com/repos/%1/%2/releases/latest")
        .arg(QString::fromLatin1(KITES_GITHUB_OWNER), QString::fromLatin1(KITES_GITHUB_REPO));
}

// Splits "1.2.3-rc1" into numeric components {1,2,3} plus the "-rc1" suffix.
QList<int> numericComponents(const QString &version, QString &suffixOut)
{
    QString core = version.trimmed();
    if (core.startsWith('v', Qt::CaseInsensitive))
        core.remove(0, 1);

    const int dashIndex = core.indexOf('-');
    if (dashIndex >= 0)
    {
        suffixOut = core.mid(dashIndex);
        core.truncate(dashIndex);
    }
    else
    {
        suffixOut.clear();
    }

    QList<int> components;
    for (const QString &part : core.split('.'))
    {
        bool parsed = false;
        const int value = part.toInt(&parsed);
        components.append(parsed ? value : 0);
    }
    return components;
}
} // namespace

ReleaseAsset ReleaseInfo::linuxUpdateAsset() const
{
    // A bare AppImage is ideal: download, chmod, done.
    for (const ReleaseAsset &asset : assets)
    {
        if (asset.name.endsWith(QStringLiteral(".AppImage"), Qt::CaseInsensitive) &&
            asset.name.startsWith(QStringLiteral("Kites"), Qt::CaseInsensitive))
        {
            return asset;
        }
    }

    // Otherwise the release workflow's zipped Linux bundle, which we unpack.
    for (const ReleaseAsset &asset : assets)
    {
        if (asset.name.contains(QStringLiteral("linux"), Qt::CaseInsensitive) &&
            asset.name.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive))
        {
            return asset;
        }
    }

    return {};
}

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this))
{
    connect(m_network, &QNetworkAccessManager::finished, this, &UpdateChecker::handleReply);
}

QString UpdateChecker::currentVersion()
{
    return QString::fromLatin1(KITES_VERSION);
}

int UpdateChecker::compareVersions(const QString &a, const QString &b)
{
    QString suffixA;
    QString suffixB;
    const QList<int> componentsA = numericComponents(a, suffixA);
    const QList<int> componentsB = numericComponents(b, suffixB);

    const int count = qMax(componentsA.size(), componentsB.size());
    for (int i = 0; i < count; ++i)
    {
        // Missing trailing components are zero, so "1.2" == "1.2.0".
        const int valueA = i < componentsA.size() ? componentsA[i] : 0;
        const int valueB = i < componentsB.size() ? componentsB[i] : 0;
        if (valueA != valueB)
            return valueA < valueB ? -1 : 1;
    }

    // Numerically equal: a pre-release is older than the plain release.
    if (suffixA.isEmpty() != suffixB.isEmpty())
        return suffixA.isEmpty() ? 1 : -1;

    return QString::compare(suffixA, suffixB);
}

InstallKind UpdateChecker::detectInstallKind()
{
    // AppImage runtimes export APPIMAGE (path to the bundle) into the process.
    if (!QProcessEnvironment::systemEnvironment().value(QStringLiteral("APPIMAGE")).isEmpty())
        return InstallKind::AppImage;

    const QString executablePath =
        QFileInfo(QCoreApplication::applicationFilePath()).canonicalFilePath();

    // /usr/bin and /usr/local/bin are package-manager (or root installer)
    // territory; a user-prefix or build-tree binary is ours to replace.
    if (executablePath.startsWith(QStringLiteral("/usr/")))
        return InstallKind::SystemPackage;

    return InstallKind::LocalBuild;
}

bool UpdateChecker::isChecking() const
{
    return m_checking;
}

void UpdateChecker::checkForUpdates()
{
    if (m_checking)
        return;

    m_checking = true;

    QNetworkRequest request{QUrl(latestReleaseEndpoint())};
    request.setRawHeader("Accept", "application/vnd.github+json");
    // GitHub rejects API requests without a User-Agent.
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("Kites/%1").arg(currentVersion()));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    m_network->get(request);
}

void UpdateChecker::handleReply(QNetworkReply *reply)
{
    reply->deleteLater();
    m_checking = false;

    if (reply->error() != QNetworkReply::NoError)
    {
        emit checkFailedSignal(reply->errorString());
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        emit checkFailedSignal(tr("Could not read the response from GitHub."));
        return;
    }

    const QJsonObject root = document.object();
    const QString tag = root.value(QStringLiteral("tag_name")).toString();
    if (tag.isEmpty())
    {
        emit checkFailedSignal(tr("No published releases were found."));
        return;
    }

    ReleaseInfo release;
    release.version = tag.startsWith('v', Qt::CaseInsensitive) ? tag.mid(1) : tag;
    release.title = root.value(QStringLiteral("name")).toString();
    release.notes = root.value(QStringLiteral("body")).toString();
    release.downloadUrl = root.value(QStringLiteral("html_url")).toString();

    for (const QJsonValue &value : root.value(QStringLiteral("assets")).toArray())
    {
        const QJsonObject assetObject = value.toObject();
        ReleaseAsset asset;
        asset.name = assetObject.value(QStringLiteral("name")).toString();
        asset.url = assetObject.value(QStringLiteral("browser_download_url")).toString();
        asset.size = static_cast<qint64>(assetObject.value(QStringLiteral("size")).toDouble());
        if (!asset.name.isEmpty() && !asset.url.isEmpty())
            release.assets.append(asset);
    }

    if (release.downloadUrl.isEmpty())
    {
        release.downloadUrl =
            QStringLiteral("%1/releases").arg(QString::fromLatin1(KITES_HOMEPAGE_URL));
    }

    if (compareVersions(release.version, currentVersion()) > 0)
        emit updateAvailableSignal(release);
    else
        emit upToDateSignal();
}

} // namespace Kites
