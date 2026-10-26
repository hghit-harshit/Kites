#pragma once
#include <QObject>
#include <QSettings>
#include <QString>

namespace Kites
{

enum class BorderStyle
{
    None,
    Subtle,
    Full
};

/**
 * @brief When (if ever) the editor writes the buffer back to its own file.
 *
 * This is separate from crash recovery, which is always on and never touches
 * the user's file - see FileService.
 */
enum class AutosaveMode
{
    Off,       ///< Only an explicit Ctrl+S writes the file.
    AfterDelay ///< Write the file once typing has paused for autosaveDelaySeconds().
};

/**
 * @brief AppSettings persists user-facing UI preferences (theme, font, panel
 * styling) across application restarts using QSettings.
 */
class AppSettings : public QObject
{
    Q_OBJECT
  public:
    static AppSettings &getInstance();

    QString themeId() const;
    void setThemeId(const QString &id);

    // empty string = editor follows the global theme
    QString editorThemeId() const;
    void setEditorThemeId(const QString &id);

    QString editorFontFamily() const;
    void setEditorFontFamily(const QString &family);

    int editorFontSize() const;
    void setEditorFontSize(int size);

    BorderStyle compilerOutputBorderStyle() const;
    void setCompilerOutputBorderStyle(BorderStyle style);

    AutosaveMode autosaveMode() const;         ///< Defaults to Off.
    void setAutosaveMode(AutosaveMode mode);

    int autosaveDelaySeconds() const;          ///< Defaults to 5.
    void setAutosaveDelaySeconds(int seconds);

  signals:
    void compilerOutputBorderStyleChangedSignal(BorderStyle style);
    void autosaveSettingsChangedSignal();

  private:
    AppSettings();

    QSettings m_settings;
};

} // namespace Kites
