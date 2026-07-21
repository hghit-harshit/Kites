#include "ui/common/kites_editor.h"
#include "ui/common/line_number_gutter_column.h"
namespace Kites
{
class CompilerEditor : public KitesEditor
{
    Q_OBJECT
public:
    explicit CompilerEditor(QWidget *parent = nullptr)
    : KitesEditor(parent)
    {
        addLeftGutterColumn(new LineNumberGutterColumn(this));
    }
    ~CompilerEditor() {};
};
} // namespace Kites
