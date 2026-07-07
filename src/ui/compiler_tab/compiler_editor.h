#include "ui/common/kites_editor.h"

namespace Kites
{
class CompilerEditor : public KitesEditor
{
    Q_OBJECT
  public:
    explicit CompilerEditor(QWidget *parent = nullptr) {Q_UNUSED(parent);};
    ~CompilerEditor() {};
};
} // namespace Kites
