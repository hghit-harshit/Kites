#ifndef PROFILERTAB_H
#define PROFILERTAB_H

#include <QWidget>
#include <QString>
#include "kitestab.h"
#include <map>

class ProfilerManager;

namespace Kites
{
namespace Ui {
class ProfilerTab;
}

class ProfilerTab : public KitesTab
{
    Q_OBJECT

public:
    explicit ProfilerTab(QWidget *parent = nullptr, ProfilerManager* profilerManager = nullptr);
    ~ProfilerTab();

public slots:
    void setSourceText(const QString& sourceText);
    void updateLineExecutionCounts(const std::map<int, int>& lineExecutionCounts);
    void resetProfilerView();

private:
    Ui::ProfilerTab *ui;
    ProfilerManager* profiler_manager_ = nullptr;
};
}// namespace Kites
#endif // PROFILERTAB_H
