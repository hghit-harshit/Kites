#pragma once
#include "gutter_column.h"

namespace Kites
{
class LineNumberGutterColumn : public GutterColumn
{
    Q_OBJECT
public:
    explicit LineNumberGutterColumn(KitesEditor* editor = nullptr);
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent *event) override;
};
}//namespace Kites