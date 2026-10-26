#pragma once
#include "update_checker.h"
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;
class QFile;

namespace Kites
{

/**
 * @brief Downloads a release asset to a temporary file, reporting progress.
 *
 * The destination deliberately sits in the same directory as the executable
 * being replaced, so the final swap is a same-filesystem rename() and therefore
 * atomic. If that directory is not writable the caller falls back to the system
 * temp dir and a privileged copy.
 */
class UpdateDownloader : public QObject
{
    Q_OBJECT
  public:
    explicit UpdateDownloader(QObject *parent = nullptr);
    ~UpdateDownloader() override;

    /**
     * @brief Starts downloading @p asset into @p destinationPath.
     */
    void start(const ReleaseAsset &asset, const QString &destinationPath);

    /// Aborts an in-flight download and removes the partial file.
    void cancel();

  signals:
    void progressSignal(qint64 bytesReceived, qint64 bytesTotal);
    void finishedSignal(const QString &downloadedPath);
    void failedSignal(const QString &errorMessage);

  private slots:
    void onReadyRead();
    void onFinished();

  private:
    void cleanupPartialFile();

    QNetworkAccessManager *m_network;
    QNetworkReply *m_reply{nullptr};
    QFile *m_file{nullptr};
    QString m_destinationPath;
    bool m_cancelled{false};
};

} // namespace Kites
