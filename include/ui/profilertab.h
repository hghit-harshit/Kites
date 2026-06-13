#ifndef PROFILERTAB_H
#define PROFILERTAB_H

#include "kitestab.h"
#include <QString>
#include <QWidget>
#include <map>
#include <string>


class Profiler; // Forward declaration to avoid including profiler header

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
    explicit ProfilerTab(QWidget *parent = nullptr, Profiler *profiler = nullptr);
    ~ProfilerTab();

  public slots:
    void setSourceText(const QString &sourceText);
    void updateLineExecutionCount(const std::map<int, int> &lineExecutionCounts);
    void updateLineInstructionType(const std::map<int, std::string> &lineInstructionTypes);
    void updateStatistics(const std::map<std::string, int> &statistics);
    void resetProfilerView();

  private:
    Ui::ProfilerTab *ui;
    Profiler *m_profiler = nullptr;
};
} // namespace Kites
#endif // PROFILERTAB_H
