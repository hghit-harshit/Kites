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
    void OnCustomPolicyClicked();
    ReplacementPolicy m_lastSelectedPolicy; // to be used when loading custom script fails
    
public slots:
    void CacheStatsUpdated(CacheStats newStats);
    void CustomPolicyScriptLoaded(bool success, const std::string& message);
    
signals:
    void configChanged();
    void customPolicyScriptSelected(const std::string& scriptPath);
    
    
};
} // namespace Kites
#endif // CACHECONFIGWIDGET_H

