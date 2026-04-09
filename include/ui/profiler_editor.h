#pragma once
#include <QWidget>
#include <QPlainTextEdit>
#include <map>
#include <QColor>
namespace Kites
{

namespace ProfilerEditorHelpers
{
    class LineNumberArea;
    class CountArea;
}// namespace ProfilerEdiorHelpers

    class ProfilerEditor : public QPlainTextEdit
    {
        Q_OBJECT
    public:
        explicit ProfilerEditor(QWidget *parent = nullptr);
        ~ProfilerEditor(){};

        //called everytime profiler data is updated
        void setHitCount(const std::map<int, int>& hitCounts); // line number to hit count mapping
        void clearHitCount();

        int lineNumberAreaWidth() const;
        int countAreaWidth() const;

        void paintLineNumberArea(QPaintEvent *event);
        void paintCountArea(QPaintEvent *event);

    protected:
        void resizeEvent(QResizeEvent *event) override;
    public slots:
        void updateAreaWidths();
        void updateAreas(const QRect &rect, int dy);

    private:
        ProfilerEditorHelpers::LineNumberArea *m_lineNumberArea;
        ProfilerEditorHelpers::CountArea *m_countArea;
        std::map<int, int> m_hitCounts; // line number to hit count mapping
        int m_maxHitCount = 0;          // to determine the color intensity for heatmap

        QColor heatColor(int hitCount) const;           // Helper function to determine color based on hit count
        QColor heatBackgroundColor(int hitCount) const; // Helper function to determine background color based on hit count
    };


    class ProfilerEditorHelpers::LineNumberArea : public QWidget
    {
    public:
        LineNumberArea(ProfilerEditor *editor) : QWidget(editor), m_profilerEditor(editor) {}

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
        CountArea(ProfilerEditor *editor) : QWidget(editor), m_profilerEditor(editor) {}

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
}; // namespace Kites