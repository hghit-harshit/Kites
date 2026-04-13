#include "ui/kites_editor.h"


namespace Kites
{
class CompilerEditor : public KitesEditor
{
    Q_OBJECT
public:
    explicit CompilerEditor(QWidget *parent = nullptr){};
    ~CompilerEditor(){};
};
} // namespace Kites