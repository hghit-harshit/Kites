#pragma once
#include <QPalette>
#include <QWidget>
#include <QString>
#include <QColor>
#include <array>
#include <QTextCharFormat>
#include "utils/to_index.hpp"
namespace Kites
{

enum class ThemeType
{
    Light,
    Dark,
    ThemeTypeCount
};

#define ENUM_FROM_STRING_LIST \
X(R32Instruction, "R32Instruction")\
X(R64Instruction, "R64Instruction")\
X(MInstruction, "MInstruction")\
X(FInstruction, "FInstruction")\
X(DInstruction, "DInstruction")\
X(PseudoInstruction, "PseudoInstruction")\

enum class SyntaxStyle
{
    #define X(enumValue, stringValue) enumValue,
    ENUM_FROM_STRING_LIST 
    //wow this magic!!
    #undef X
    Register,
    Comment,
    Label,
    NumberLiteral,
    SyntaxStyleCount
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

    std::array<QTextCharFormat, toIndex(SyntaxStyle::SyntaxStyleCount)> syntaxFormats;
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
    QTextCharFormat getSyntaxFormat(SyntaxStyle style)const;
signals:
    void themeChangedSignal(ThemeType theme);

private: 
    ThemeManager();
    QString loadStyleSheet(const QString &path);
    void setUpThemes();

    ThemeType m_currentThemeType{ThemeType::ThemeTypeCount}; // no theme selected initiallys
    std::array<ThemeData, toIndex(ThemeType::ThemeTypeCount)> m_themes;

};
}//namespace Kites