#pragma once
#include <QPalette>
#include <QWidget>
#include <QString>
#include <QColor>
#include <array>
namespace Kites
{

enum class ThemeType
{
    Light,
    Dark,
    ThemeTypeCount
};

struct ThemeData
{
    ThemeType type;
    QString name;

    QString stylesheetPath;
    QPalette palette;

    QColor iconColor;
    QColor textColor;

    QColor cacheHitColor;
    QColor cacheMissColor;
};

/**
 * @brief ThemeManager is a singleton class that manages the application's theme and palette.
 * 
 */
class ThemeManager : public QObject
{
    Q_OBJECT
public:
    ~ThemeManager() = default;

    static ThemeManager &getInstance();

    ThemeType currentThemeType() const;
    void setTheme(ThemeType theme);
    QColor getIconColor()const;
    QColor getTextColor()const;
signals:
    void themeChangedSignal(ThemeType theme);

private: 
    ThemeManager();
    QString loadStyleSheet(const QString &path);
    void setUpThemes();

    ThemeType m_currentThemeType;
    std::array<ThemeData, static_cast<int>(ThemeType::ThemeTypeCount)> m_themes;

};
}//namespace Kites