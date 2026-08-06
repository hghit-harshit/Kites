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

  signals:
    void compilerOutputBorderStyleChangedSignal(BorderStyle style);

  private:
    AppSettings();

    QSettings m_settings;
};

} // namespace Kites
