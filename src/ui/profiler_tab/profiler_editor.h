#pragma once
#include "ui/common/syntax_highlighter.h"
#include "ui/common/kites_editor.h"
#include <QColor>
#include <QPlainTextEdit>
#include <QWidget>
#include <map>
#include <string>

namespace Kites
{
class ExecutionCountGutterColumn; 
class InstructionTypeGutterColumn; // forward declaration
class ProfilerEditor : public KitesEditor
{
    Q_OBJECT
    friend class ExecutionCountGutterColumn;
    friend class InstructionTypeGutterColumn;
public:
    explicit ProfilerEditor(QWidget *parent = nullptr);
    ~ProfilerEditor() {};

    // called everytime profiler data is updated
    void setExecutionCount(const std::map<int, int> &hitCounts); // line number to hit count mapping
    void setLineExecutionCount(const int &lineNumber, const int &hitCount); // update hit count for a 
                                                                      //specific line
    void setInstructionTypes(const std::map<int, std::string>
                                 &instructionTypes); // line number to instruction type mapping
    void clearExecutionCount();

    // int countAreaWidth() const;
    // int typeAreaWidth() const;

    // void paintCountArea(QPaintEvent *event);
    // void paintTypeArea(QPaintEvent *event);

private:
    
    SyntaxHighlighter *m_syntaxHighlighter = nullptr;
    std::map<int, int> m_lineNumberToExecutionCounts;
    std::map<int, std::string> m_lineNumberToInstructionType;
    int m_maxExecutionCount = 0; // to determine the color intensity for heatmap

    QColor heatColor(int hitCount) const; // Helper function to determine color based on hit count
    QColor heatBackgroundColor(
        int hitCount) const; // Helper function to determine background color based on hit count
};
}; // namespace Kites
