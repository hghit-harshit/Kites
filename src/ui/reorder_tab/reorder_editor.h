#pragma once
#include "ui/common/kites_editor.h"
#include <QString>
#include <vector>

namespace Kites
{
class SyntaxHighlighter;
class ReorderMapGutterColumn;

// Read-only view of the reordered program. Looks like the Editor / Profiler tabs:
//   left gutter  : line number (shared LineNumberGutterColumn)
//   text         : "<addr>: <hex word>   <instruction>", syntax highlighted
//   right gutter : "from -> to" (original instruction # -> new instruction #)
class ReorderEditor : public KitesEditor
{
    Q_OBJECT
    friend class ReorderMapGutterColumn;

  public:
    explicit ReorderEditor(QWidget *parent = nullptr);

    // text: one line per instruction, origIndex[k] = original instruction # of line k
    void setReordered(const QString &text, const std::vector<int> &origIndex);
    void clearReordered();

  private:
    void applyMovedHighlights();
    void relayoutGutters();

    SyntaxHighlighter *m_syntaxHighlighter = nullptr;
    std::vector<int> m_origIndex;
};
} // namespace Kites
