#include "ui/profilertab.h"
#include "ui_profilertab.h"

namespace Kites
{
ProfilerTab::ProfilerTab(QWidget *parent)
    : KitesTab(parent)
    , ui(new Ui::ProfilerTab)
{
    ui->setupUi(this);
    qDebug() << "ProfilerTab initialized";
}

ProfilerTab::~ProfilerTab()
{
    delete ui;
}
}// namespace Kites
