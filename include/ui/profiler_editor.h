#pragma once
#include "ui/syntax_highlighter.h"
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
class CountArea;
class TypeArea;
} // namespace ProfilerEditorHelpers

class ProfilerEditor : public QPlainTextEdit
{
    Q_OBJECT
  public:
    explicit ProfilerEditor(QWidget *parent = nullptr);
    ~ProfilerEditor() {};

    // called everytime profiler data is updated
    void setHitCount(const std::map<int, int> &hitCounts); // line number to hit count mapping
    void setInstructionTypes(const std::map<int, std::string>
                                 &instructionTypes); // line number to instruction type mapping
    void clearHitCount();

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
    ProfilerEditorHelpers::CountArea *m_countArea;
    ProfilerEditorHelpers::TypeArea *m_typeArea;
    SyntaxHighlighter *m_syntaxHighlighter = nullptr;
    std::map<int, int> m_hitCounts;                // line number to hit count mapping
    std::map<int, std::string> m_instructionTypes; // line number to instruction type mapping
    int m_maxHitCount = 0;                         // to determine the color intensity for heatmap

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

class ProfilerEditorHelpers::CountArea : public QWidget
{
  public:
    CountArea(ProfilerEditor *editor) : QWidget(editor), m_profilerEditor(editor)
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

class ProfilerEditorHelpers::TypeArea : public QWidget
{
  public:
    TypeArea(ProfilerEditor *editor) : QWidget(editor), m_profilerEditor(editor)
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