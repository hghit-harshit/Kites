#ifndef PROFILERTAB_H
#define PROFILERTAB_H

#include "kitestab.h"
#include <QString>
#include <QWidget>
#include <map>
#include <string>


class ProfilerManager;

namespace Kites
{
namespace Ui
{
class ProfilerTab;
}

class ProfilerTab : public KitesTab
{
    Q_OBJECT

  public:
    explicit ProfilerTab(QWidget *parent = nullptr, ProfilerManager *profilerManager = nullptr);
    ~ProfilerTab();

  public slots:
    void setSourceText(const QString &sourceText);
    void updateLineExecutionCounts(const std::map<int, int> &lineExecutionCounts);
    void updateInstructionTypes(const std::map<int, std::string> &instructionTypes);
    void updateStatistics(const std::map<std::string, int> &statistics);
    void resetProfilerView();

  private:
    Ui::ProfilerTab *ui;
    ProfilerManager *profiler_manager_ = nullptr;
};
} // namespace Kites
#endif // PROFILERTAB_H
