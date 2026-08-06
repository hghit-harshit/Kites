#include "theme_manager.h"
#include "config/app_settings.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QStandardPaths>
#include <QTextStream>

namespace Kites
{
namespace
{
QColor colorOr(const QJsonObject &colors, const QString &key, const QColor &fallback)
{
    if (!colors.contains(key))
        return fallback;
    return QColor(colors.value(key).toString());
}
} // namespace

ThemeManager::ThemeManager()
{
    QFile templateFile(":/themes/template.qss");
    if (templateFile.open(QIODevice::ReadOnly | QIODevice::Text))
        m_qssTemplate = QString::fromUtf8(templateFile.readAll());
    else
        qWarning() << "ThemeManager: failed to load QSS template";

    loadBuiltinThemes();
    loadUserThemes();

    const QString savedEditorId = AppSettings::getInstance().editorThemeId();
    if (!savedEditorId.isEmpty() && indexOfTheme(savedEditorId) >= 0)
        m_editorThemeId = savedEditorId;

    QString savedId = AppSettings::getInstance().themeId();
    if (savedId.isEmpty() || indexOfTheme(savedId) < 0)
        savedId = "tokyo-night";
    setTheme(savedId);
}

ThemeManager &ThemeManager::getInstance()
{
    static ThemeManager instance;
    return instance;
}

QString ThemeManager::currentThemeId() const
{
    return m_currentThemeId;
}

QVector<ThemeData> ThemeManager::availableThemes() const
{
    return m_themes;
}

int ThemeManager::indexOfTheme(const QString &id) const
{
    for (int i = 0; i < m_themes.size(); ++i)
    {
        if (m_themes[i].id == id)
            return i;
    }
    return -1;
}

void ThemeManager::setTheme(const QString &id)
{
    if (id == m_currentThemeId)
        return;

    const int index = indexOfTheme(id);
    if (index < 0)
    {
        qWarning() << "ThemeManager: unknown theme id:" << id;
        return;
    }

    m_currentThemeId = id;
    applyTheme(m_themes[index]);
    AppSettings::getInstance().setThemeId(id);
    emit themeChangedSignal(id);
    if (m_editorThemeId.isEmpty()) // editor follows global
        emit editorThemeChangedSignal(id);
}

const ThemeData *ThemeManager::editorTheme() const
{
    const int index = indexOfTheme(currentEditorThemeId());
    return index >= 0 ? &m_themes[index] : nullptr;
}

QString ThemeManager::currentEditorThemeId() const
{
    return m_editorThemeId.isEmpty() ? m_currentThemeId : m_editorThemeId;
}

QString ThemeManager::configuredEditorThemeId() const
{
    return m_editorThemeId;
}

void ThemeManager::setEditorTheme(const QString &id)
{
    if (!id.isEmpty() && indexOfTheme(id) < 0)
    {
        qWarning() << "ThemeManager: unknown editor theme id:" << id;
        return;
    }

    const QString previousEffective = currentEditorThemeId();
    m_editorThemeId = id;
    AppSettings::getInstance().setEditorThemeId(id);

    const QString newEffective = currentEditorThemeId();
    if (newEffective != previousEffective)
        emit editorThemeChangedSignal(newEffective);
}

QColor ThemeManager::getEditorBackgroundColor() const
{
    const ThemeData *theme = editorTheme();
    return theme ? theme->surfaceColor : QColor("#16161e");
}

QColor ThemeManager::getEditorForegroundColor() const
{
    const ThemeData *theme = editorTheme();
    return theme ? theme->textColor : QColor("#c0caf5");
}

QColor ThemeManager::getEditorMutedForegroundColor() const
{
    const ThemeData *theme = editorTheme();
    return theme ? theme->mutedForegroundColor : QColor("#565f89");
}

QColor ThemeManager::getEditorErrorColor() const
{
    const ThemeData *theme = editorTheme();
    return theme ? theme->cacheMissColor : QColor("#f7768e");
}

void ThemeManager::applyTheme(const ThemeData &theme)
{
    QApplication::setPalette(theme.palette);
    qApp->setStyleSheet(theme.stylesheet);
}

QColor ThemeManager::getIconColor() const
{
    const int index = indexOfTheme(m_currentThemeId);
    return index >= 0 ? m_themes[index].iconColor : QColor("#d4d4d4");
}

QColor ThemeManager::getTextColor() const
{
    const int index = indexOfTheme(m_currentThemeId);
    return index >= 0 ? m_themes[index].textColor : QColor("#d4d4d4");
}

QColor ThemeManager::getCurrentLineColor() const
{
    const ThemeData *theme = editorTheme();
    return theme ? theme->currentLineColor : QColor("#2a2a2a");
}

QTextCharFormat ThemeManager::getSyntaxFormat(SyntaxStyle style) const
{
    const ThemeData *theme = editorTheme();
    if (!theme)
        return {};
    return theme->syntaxFormats[static_cast<int>(style)];
}

QString ThemeManager::userThemesDirectory() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/themes";
}

void ThemeManager::loadBuiltinThemes()
{
    QDir builtinDir(":/themes");
    const QStringList jsonFiles = builtinDir.entryList(QStringList() << "*.json", QDir::Files);
    for (const QString &fileName : jsonFiles)
    {
        ThemeData theme;
        QString error;
        if (parseThemeFile(builtinDir.filePath(fileName), theme, error))
            m_themes.append(theme);
        else
            qWarning() << "ThemeManager: failed to load builtin theme" << fileName << ":" << error;
    }
}

void ThemeManager::loadUserThemes()
{
    QDir userDir(userThemesDirectory());
    if (!userDir.exists())
    {
        QDir().mkpath(userDir.absolutePath());
        return;
    }

    const QStringList jsonFiles = userDir.entryList(QStringList() << "*.json", QDir::Files);
    for (const QString &fileName : jsonFiles)
    {
        ThemeData theme;
        QString error;
        if (parseThemeFile(userDir.filePath(fileName), theme, error))
        {
            if (indexOfTheme(theme.id) >= 0)
                continue; // don't let a user theme shadow a builtin id
            m_themes.append(theme);
        }
        else
        {
            qWarning() << "ThemeManager: failed to load user theme" << fileName << ":" << error;
        }
    }
}

bool ThemeManager::parseThemeFile(const QString &path, ThemeData &themeOut, QString &errorOut) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        errorOut = "could not open file";
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        errorOut = parseError.errorString();
        return false;
    }

    const QJsonObject root = doc.object();
    const QString id = root.value("id").toString();
    const QString name = root.value("name").toString();
    if (id.isEmpty() || name.isEmpty() || !root.value("colors").isObject())
    {
        errorOut = "missing required fields (id, name, colors)";
        return false;
    }

    const QJsonObject colors = root.value("colors").toObject();
    const bool isDark = root.value("appearance").toString().toLower() != "light";

    themeOut.id = id;
    themeOut.name = name;
    themeOut.appearance = isDark ? ThemeAppearance::Dark : ThemeAppearance::Light;

    const QColor background      = colorOr(colors, "background", isDark ? QColor("#1e1e1e") : QColor("#f5f5f5"));
    const QColor surface         = colorOr(colors, "surface", background);
    const QColor foreground      = colorOr(colors, "foreground", isDark ? QColor("#d4d4d4") : QColor("#1a1a1a"));
    const QColor mutedForeground = colorOr(colors, "mutedForeground", foreground.darker(150));
    const QColor accent          = colorOr(colors, "accent", QColor("#2a82da"));
    const QColor selection       = colorOr(colors, "selection", accent.darker(150));
    const QColor currentLine     = colorOr(colors, "currentLine", surface.lighter(isDark ? 115 : 97));

    themeOut.iconColor        = colorOr(colors, "iconColor", foreground);
    themeOut.textColor        = foreground;
    themeOut.currentLineColor = currentLine;
    themeOut.surfaceColor     = surface;
    themeOut.mutedForegroundColor = mutedForeground;
    themeOut.cacheHitColor    = colorOr(colors, "cacheHit", QColor("#2ada5c"));
    themeOut.cacheMissColor   = colorOr(colors, "cacheMiss", QColor("#da2a2a"));

    QPalette &p = themeOut.palette;
    p.setColor(QPalette::Window,          background);
    p.setColor(QPalette::WindowText,      foreground);
    p.setColor(QPalette::Base,            surface);
    p.setColor(QPalette::AlternateBase,   currentLine);
    p.setColor(QPalette::ToolTipBase,     surface);
    p.setColor(QPalette::ToolTipText,     foreground);
    p.setColor(QPalette::Text,            foreground);
    p.setColor(QPalette::Button,          surface);
    p.setColor(QPalette::ButtonText,      foreground);
    p.setColor(QPalette::Highlight,       selection);
    p.setColor(QPalette::HighlightedText, foreground);
    p.setColor(QPalette::Link,            accent);

    p.setColor(QPalette::Disabled, QPalette::Window,     background);
    p.setColor(QPalette::Disabled, QPalette::WindowText, mutedForeground);
    p.setColor(QPalette::Disabled, QPalette::Base,       surface);
    p.setColor(QPalette::Disabled, QPalette::Text,       mutedForeground);
    p.setColor(QPalette::Disabled, QPalette::Button,     surface);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, mutedForeground);

    const QJsonObject syntax = colors.value("syntax").toObject();
    auto &formats = themeOut.syntaxFormats;
    auto setSyntax = [&](SyntaxStyle style, const QString &key, const QColor &fallback, bool bold = false)
    {
        QTextCharFormat format;
        format.setForeground(colorOr(syntax, key, fallback));
        if (bold)
            format.setFontWeight(QFont::Bold);
        formats[static_cast<int>(style)] = format;
    };
    setSyntax(SyntaxStyle::R32Instruction, "r32Instruction", QColor("#569cd6"), true);
    setSyntax(SyntaxStyle::R64Instruction, "r64Instruction", QColor("#4fc1ff"), true);
    setSyntax(SyntaxStyle::MInstruction, "mInstruction", QColor("#c586c0"), true);
    setSyntax(SyntaxStyle::FInstruction, "fInstruction", QColor("#c586c0"), true);
    setSyntax(SyntaxStyle::DInstruction, "dInstruction", QColor("#dcdcaa"), true);
    setSyntax(SyntaxStyle::PseudoInstruction, "pseudoInstruction", QColor("#ff8c00"));
    setSyntax(SyntaxStyle::Register, "register", QColor("#9cdcfe"));
    setSyntax(SyntaxStyle::Comment, "comment", QColor("#6a9955"));
    setSyntax(SyntaxStyle::Label, "label", QColor("#d7ba7d"));
    setSyntax(SyntaxStyle::NumberLiteral, "numberLiteral", QColor("#b5cea8"));

    themeOut.stylesheet = renderStylesheet(colors);
    return true;
}

QString ThemeManager::renderStylesheet(const QJsonObject &colors) const
{
    QString result = m_qssTemplate;

    static const QStringList tokens = {
        "background", "surface", "border", "foreground",
        "mutedForeground", "accent", "selection", "currentLine"
    };

    const QColor background = colorOr(colors, "background", QColor("#1e1e1e"));
    const QColor surface     = colorOr(colors, "surface", background);
    const QColor border      = colorOr(colors, "border", background.lighter(130));
    const QColor foreground  = colorOr(colors, "foreground", QColor("#d4d4d4"));
    const QColor accent      = colorOr(colors, "accent", QColor("#2a82da"));
    const QColor selection   = colorOr(colors, "selection", accent.darker(150));

    QHash<QString, QColor> values;
    values["background"]      = background;
    values["surface"]         = surface;
    values["border"]          = border;
    values["foreground"]      = foreground;
    values["mutedForeground"] = colorOr(colors, "mutedForeground", foreground.darker(150));
    values["accent"]          = accent;
    values["selection"]       = selection;
    values["currentLine"]     = colorOr(colors, "currentLine", surface.lighter(115));

    for (const QString &token : tokens)
        result.replace("{{" + token + "}}", values[token].name());

    return result;
}

bool ThemeManager::installTheme(const QString &sourceFilePath, QString &errorOut)
{
    ThemeData theme;
    if (!parseThemeFile(sourceFilePath, theme, errorOut))
        return false;

    QDir userDir(userThemesDirectory());
    if (!userDir.exists())
        QDir().mkpath(userDir.absolutePath());

    const QString destPath = userDir.filePath(theme.id + ".json");
    if (QFile::exists(destPath))
        QFile::remove(destPath);

    if (!QFile::copy(sourceFilePath, destPath))
    {
        errorOut = "failed to copy theme into " + userDir.absolutePath();
        return false;
    }

    const int existingIndex = indexOfTheme(theme.id);
    if (existingIndex >= 0)
        m_themes[existingIndex] = theme;
    else
        m_themes.append(theme);

    if (theme.id == m_currentThemeId)
    {
        applyTheme(theme);
        AppSettings::getInstance().setThemeId(theme.id);
        emit themeChangedSignal(theme.id);
    }
    else
    {
        setTheme(theme.id);
    }
    return true;
}

}//namespace Kites
