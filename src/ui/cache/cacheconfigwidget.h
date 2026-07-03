#ifndef CACHECONFIGWIDGET_H
#define CACHECONFIGWIDGET_H

#include "vm/cache/cacheconfig.h"
#include <QWidget>

namespace Kites
{
namespace Ui
{
class CacheConfigWidget;
}

class CacheConfigWidget : public QWidget
{
    Q_OBJECT

public:
explicit CacheConfigWidget(QWidget *parent = nullptr);
    ~CacheConfigWidget();
    CacheConfig getConfig() const;

    int getLinesExponent() const;
    int getWaysExponent() const;
    int getWordsExponent() const;

    void setLinesExponent(int value);
    void setWaysExponent(int value);
    void setWordsExponent(int value);
	
	void setConfig(CacheConfig config);

private:
    Ui::CacheConfigWidget *ui;
    void onCustomPolicyClicked();
    ReplacementPolicy m_lastSelectedPolicy; // to be used when loading custom script fails

public slots:
    void cacheStatsUpdatedSlot(CacheStats newStats);
    void customPolicyScriptLoadedSlot(bool success, const std::string &message);
    // void UpdateSize(); // to update size from ui changes

signals:
    void configChangedSignal();
    void customPolicyScriptSelectedSignal(const std::string &scriptPath);
};
} // namespace Kites
#endif // CACHECONFIGWIDGET_H
