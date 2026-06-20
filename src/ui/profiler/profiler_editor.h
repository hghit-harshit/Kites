#pragma once
#include "ui/common/syntax_highlighter.h"
#include <QColor>
#include <QPlainTextEdit>
#include <QWidget>
#include <map>
#include <string>

namespace Kites
{

namespace ProfilerEditorHelpers
{
class LineNumberArea;
class ExecutionCountArea;
class InstructionTypeArea;
} // namespace ProfilerEditorHelpers

class ProfilerEditor : public QPlainTextEdit
{
    Q_OBJECT
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

    int lineNumberAreaWidth() const;
    int countAreaWidth() const;
    int typeAreaWidth() const;

    void paintLineNumberArea(QPaintEvent *event);
    void paintCountArea(QPaintEvent *event);
    void paintTypeArea(QPaintEvent *event);

  protected:
    void resizeEvent(QResizeEvent *event) override;
  public slots:
    void updateAreaWidths();
    void updateAreas(const QRect &rect, int dy);

  private:
    ProfilerEditorHelpers::LineNumberArea *m_lineNumberArea;
    ProfilerEditorHelpers::ExecutionCountArea *m_ExecutionCountArea;
    ProfilerEditorHelpers::InstructionTypeArea *m_instructionTypeArea;
    
    SyntaxHighlighter *m_syntaxHighlighter = nullptr;
    std::map<int, int> m_line_number_execution_count_mapping; // line number to 
                                                              //execution count mapping
    std::map<int, std::string> m_instructionTypes; // line number to instruction type mapping
    int m_maxExecutionCount = 0; // to determine the color intensity for heatmap

    QColor heatColor(int hitCount) const; // Helper function to determine color based on hit count
    QColor heatBackgroundColor(
        int hitCount) const; // Helper function to determine background color based on hit count
};

class ProfilerEditorHelpers::LineNumberArea : public QWidget
{
  public:
    LineNumberArea(ProfilerEditor *editor) : QWidget(editor), m_profilerEditor(editor)
    {
    }

    QSize sizeHint() const override
    {
        return QSize(m_profilerEditor->lineNumberAreaWidth(), 0);
    }

  protected:
    void paintEvent(QPaintEvent *event) override
    {
        m_profilerEditor->paintLineNumberArea(event);
    }

  private:
    ProfilerEditor *m_profilerEditor;
};

class ProfilerEditorHelpers::ExecutionCountArea : public QWidget
{
  public:
    ExecutionCountArea(ProfilerEditor *editor) : QWidget(editor), m_profilerEditor(editor)
    {
    }

    QSize sizeHint() const override
    {
        return QSize(m_profilerEditor->countAreaWidth(), 0);
    }

  protected:
    void paintEvent(QPaintEvent *event) override
    {
        m_profilerEditor->paintCountArea(event);
    }

  private:
    ProfilerEditor *m_profilerEditor;
};

class ProfilerEditorHelpers::InstructionTypeArea : public QWidget
{
  public:
    InstructionTypeArea(ProfilerEditor *editor) : QWidget(editor), m_profilerEditor(editor)
    {
    }

    QSize sizeHint() const override
    {
        return QSize(m_profilerEditor->typeAreaWidth(), 0);
    }

  protected:
    void paintEvent(QPaintEvent *event) override
    {
        m_profilerEditor->paintTypeArea(event);
    }

  private:
    ProfilerEditor *m_profilerEditor;
};
}; // namespace Kites
