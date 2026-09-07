#pragma once
#include <QObject>
#include <QString>
#include <functional>

class QTimer;
class QWidget;

namespace Kites
{

/**
 * @brief Owns everything about "the file the user is editing".
 *
 * Tracks the current path and modified state, implements Open / Save / Save As,
 * and runs two independent kinds of automatic writing:
 *
 *  - **Crash recovery** (always on): the buffer is mirrored to a recovery file
 *    under the app data dir on a short timer. This never touches the user's own
 *    file and works for untitled buffers too, so unsaved work survives a crash.
 *    The recovery file is deleted on a clean save or a clean exit; anything left
 *    behind on the next launch means the previous session died, and the user is
 *    offered a restore.
 *
 *  - **Autosave to the real file** (off by default, see AppSettings): once the
 *    buffer has a path, write it back after typing has paused.
 *
 * Deliberately UI-agnostic: it reads and writes the document through the two
 * callbacks given to setDocumentAccessors(), so it has no dependency on
 * EditorTab and can be tested without a GUI.
 */
class FileService : public QObject
{
    Q_OBJECT
  public:
    /**
     * @param parentWidget Used only as the parent for file/message dialogs.
     */
    explicit FileService(QWidget *parentWidget, QObject *parent = nullptr);
    ~FileService() override;

    /**
     * @brief Wires the service to the document it manages.
     * @param readText   Returns the editor's current contents.
     * @param writeText  Replaces the editor's contents.
     */
    void setDocumentAccessors(std::function<QString()> readText,
                              std::function<void(const QString &)> writeText);

    /// Call whenever the document is edited, so dirty state and timers update.
    void markDirty();

    QString currentFilePath() const; ///< Empty when the buffer is untitled.
    bool isDirty() const;

    /// Display name for the window title, e.g. "mul.asm" or "Untitled".
    QString displayName() const;

  public slots:
    /// Prompts for a file, loads it. Offers to save unsaved changes first.
    void open();
    /// Loads @p path directly (no prompt). Offers to save unsaved changes first.
    void openPath(const QString &path);
    /// Writes to the current path, falling back to saveAs() when untitled.
    bool save();
    /// Always prompts for a destination.
    bool saveAs();

    /**
     * @brief Offers to save if dirty. Call before quitting or loading a file.
     * @return false if the user chose Cancel, i.e. the caller should abort.
     */
    bool maybeSaveChanges();

    /**
     * @brief If a recovery file from a previous session exists, offer to restore it.
     *
     * Call once at startup, after the editor exists.
     */
    void offerRecoveryIfPresent();

    /// Clean-exit hook: stops autosaving and removes the recovery file.
    void shutdown();

  signals:
    /// Path and/or dirty state changed - used to refresh the window title.
    void documentStateChangedSignal();
    /// Transient status text ("Saved mul.asm", "Autosaved", ...).
    void statusMessageSignal(const QString &message);

  private slots:
    void onRecoveryTimerTimeout();
    void onAutosaveTimerTimeout();
    void applyAutosaveSettings();

  private:
    bool writeToPath(const QString &path, QString &errorOut) const;
    void setCurrentFilePath(const QString &path);
    void setDirty(bool dirty);
    QString recoveryFilePath() const;
    QString recoveryMetaPath() const;
    void writeRecoverySnapshot();
    void clearRecoverySnapshot();

    QWidget *m_parentWidget;
    std::function<QString()> m_readText;
    std::function<void(const QString &)> m_writeText;

    QString m_currentFilePath;
    bool m_dirty{false};
    /// Suppresses dirty-marking while we are the ones changing the document.
    bool m_loading{false};

    QTimer *m_recoveryTimer;
    QTimer *m_autosaveTimer;
};

} // namespace Kites
