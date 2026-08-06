#include "font_manager.h"
#include "config/app_settings.h"
#include <QFontDatabase>

namespace Kites
{
namespace
{
constexpr int kDefaultFontSize = 11;

// Bundled IBM Plex Mono (OFL) — the typeface Zed's own "Zed Plex Mono" derives from.
QString bundledPlexMonoFamily()
{
    static const QString family = []() -> QString
    {
        const int id = QFontDatabase::addApplicationFont(":/fonts/IBMPlexMono-Regular.ttf");
        QFontDatabase::addApplicationFont(":/fonts/IBMPlexMono-Bold.ttf");
        QFontDatabase::addApplicationFont(":/fonts/IBMPlexMono-Italic.ttf");
        QFontDatabase::addApplicationFont(":/fonts/IBMPlexMono-BoldItalic.ttf");
        const QStringList families = QFontDatabase::applicationFontFamilies(id);
        return families.isEmpty() ? QString() : families.at(0);
    }();
    return family;
}

QString bundledMonacoFamily()
{
    static const QString family = []() -> QString
    {
        const int id = QFontDatabase::addApplicationFont(":/fonts/Monaco.ttf");
        const QStringList families = QFontDatabase::applicationFontFamilies(id);
        return families.isEmpty() ? QString() : families.at(0);
    }();
    return family;
}
} // namespace

FontManager::FontManager() = default;

FontManager &FontManager::getInstance()
{
    static FontManager instance;
    return instance;
}

QString FontManager::resolveFamily() const
{
    const QString configured = AppSettings::getInstance().editorFontFamily();
    const QStringList systemFamilies = QFontDatabase::families();

    if (!configured.isEmpty() && systemFamilies.contains(configured))
        return configured;

    const QString plexMono = bundledPlexMonoFamily();
    if (!plexMono.isEmpty())
        return plexMono;

    const QString monaco = bundledMonacoFamily();
    if (!monaco.isEmpty())
        return monaco;

    return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
}

QString FontManager::currentFamily() const
{
    return resolveFamily();
}

int FontManager::currentSize() const
{
    return AppSettings::getInstance().editorFontSize();
}

QFont FontManager::currentFont() const
{
    QFont font(resolveFamily(), currentSize());
    font.setFixedPitch(true);
    return font;
}

void FontManager::setFamily(const QString &family)
{
    AppSettings::getInstance().setEditorFontFamily(family);
    emit fontChangedSignal(currentFont());
}

void FontManager::setSize(int size)
{
    AppSettings::getInstance().setEditorFontSize(size);
    emit fontChangedSignal(currentFont());
}

QStringList FontManager::recommendedFamilies() const
{
    QStringList result;
    const QStringList systemFamilies = QFontDatabase::families();

    const QString plexMono = bundledPlexMonoFamily();
    if (!plexMono.isEmpty())
        result << plexMono;

    const QString monaco = bundledMonacoFamily();
    if (!monaco.isEmpty())
        result << monaco;

    for (const QString &family : systemFamilies)
    {
        if (QFontDatabase::isFixedPitch(family) && !result.contains(family))
            result << family;
    }

    return result;
}

} // namespace Kites
