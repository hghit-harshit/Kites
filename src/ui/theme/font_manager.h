#pragma once
#include <QFont>
#include <QObject>
#include <QString>

namespace Kites
{

/**
 * @brief FontManager resolves the editor font (family/size) from AppSettings,
 * falling back through a preferred family chain when nothing is configured.
 */
class FontManager : public QObject
{
    Q_OBJECT
public:
    static FontManager &getInstance();
    ~FontManager() = default;

    QFont currentFont() const;
    QString currentFamily() const;
    int currentSize() const;

    void setFamily(const QString &family);
    void setSize(int size);

    // Families considered "recommended" for the editor: what's actually
    // installed/bundled, in preferred order, for populating settings UI.
    QStringList recommendedFamilies() const;

signals:
    void fontChangedSignal(const QFont &font);

private:
    FontManager();

    QString resolveFamily() const;
};

} // namespace Kites
