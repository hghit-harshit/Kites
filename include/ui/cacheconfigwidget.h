#ifndef CACHECONFIGWIDGET_H
#define CACHECONFIGWIDGET_H

#include <QWidget>
#include "vm/cache/cacheconfig.h"
namespace Kites
{
namespace Ui {
class CacheConfigWidget;
}

class CacheConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CacheConfigWidget(QWidget *parent = nullptr);
    ~CacheConfigWidget();
    CacheConfig GetConfig() const;

    int GetLinesExponent() const;
    int GetWaysExponent() const;
    int GetWordsExponent() const;

    void SetLinesExponent(int value, bool notify = true);
    void SetWaysExponent(int value, bool notify = true);
    void SetWordsExponent(int value, bool notify = true);
private:
    Ui::CacheConfigWidget *ui;
signals:
    void configChanged();
    
};
} // namespace Kites
#endif // CACHECONFIGWIDGET_H

