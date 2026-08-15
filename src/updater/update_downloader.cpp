#include "update_downloader.h"
#include "kites_version.h"

#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace Kites
{

UpdateDownloader::UpdateDownloader(QObject *parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this))
{
}

UpdateDownloader::~UpdateDownloader()
{
    if (m_reply)
        m_reply->abort();
}

void UpdateDownloader::start(const ReleaseAsset &asset, const QString &destinationPath)
{
    m_cancelled = false;
    m_destinationPath = destinationPath;

    m_file = new QFile(destinationPath, this);
    if (!m_file->open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        const QString error = m_file->errorString();
        delete m_file;
        m_file = nullptr;
        emit failedSignal(tr("Could not write to %1: %2").arg(destinationPath, error));
        return;
    }

    QNetworkRequest request{QUrl(asset.url)};
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("Kites/%1").arg(QString::fromLatin1(KITES_VERSION)));
    // Release asset URLs redirect to a storage CDN.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    m_reply = m_network->get(request);
    connect(m_reply, &QNetworkReply::readyRead, this, &UpdateDownloader::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &UpdateDownloader::onFinished);
    connect(m_reply, &QNetworkReply::downloadProgress, this, &UpdateDownloader::progressSignal);
}

void UpdateDownloader::onReadyRead()
{
    // Stream straight to disk rather than buffering ~50 MB in memory.
    if (m_file && m_reply)
        m_file->write(m_reply->readAll());
}

void UpdateDownloader::onFinished()
{
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    reply->deleteLater();

    if (m_file)
    {
        m_file->write(reply->readAll());
        m_file->flush();
        m_file->close();
    }

    if (m_cancelled)
    {
        cleanupPartialFile();
        return;
    }

    if (reply->error() != QNetworkReply::NoError)
    {
        const QString error = reply->errorString();
        cleanupPartialFile();
        emit failedSignal(error);
        return;
    }

    const QString path = m_destinationPath;
    delete m_file;
    m_file = nullptr;

    if (QFileInfo(path).size() == 0)
    {
        QFile::remove(path);
        emit failedSignal(tr("The downloaded file was empty."));
        return;
    }

    emit finishedSignal(path);
}

void UpdateDownloader::cancel()
{
    m_cancelled = true;
    if (m_reply)
        m_reply->abort(); // triggers onFinished(), which cleans up
    else
        cleanupPartialFile();
}

void UpdateDownloader::cleanupPartialFile()
{
    if (m_file)
    {
        m_file->close();
        delete m_file;
        m_file = nullptr;
    }
    if (!m_destinationPath.isEmpty())
        QFile::remove(m_destinationPath);
}

} // namespace Kites
