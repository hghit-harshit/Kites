#include "ui/cacheconfigwidget.h"
#include "ui_cacheconfigwidget.h"

namespace Kites
{

CacheConfigWidget::CacheConfigWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CacheConfigWidget)
{
    ui->setupUi(this);
}

CacheConfigWidget::~CacheConfigWidget()
{
    delete ui;
}
} // namespace Kites