#include "ui/registercontainer.h"
#include "ui_registercontainer.h"
#include "vm/registers.h"
namespace Kites {
RegisterContainer::RegisterContainer(QWidget *parent,RegisterFile* regfile)
    : QWidget(parent)
    , ui(new Ui::RegisterContainer)
    , m_registerModel(new RegisterModel(this,regfile))
{
    ui->setupUi(this);
    ui->tableView->setModel(m_registerModel);
    ui->tableView->verticalHeader()->setVisible(false);
    //setupRegisterTable();
    
}

// void RegisterContainer::setupRegisterTable()
// {
//     ui->registerTable->setRowCount(32);
//     ui->registerTable->setColumnCount(2);

//     QStringList headers;
//     headers << "Register" << "Value";
//     ui->registerTable->setHorizontalHeaderLabels(headers);
//     ui->registerTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
//     ui->registerTable->verticalHeader()->setVisible(false);

//     for(int i = 0;i < 32; ++i)
//     {
//         QTableWidgetItem *regName = new QTableWidgetItem(QString("x%1").arg(i));
//         regName->setTextAlignment(Qt::AlignCenter);
//         ui->registerTable->setItem(i, 0, regName);

//         QTableWidgetItem *regValue = new QTableWidgetItem("0x00000000");
//         regValue->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
//         ui->registerTable->setItem(i, 1, regValue);
//     }
// }
RegisterContainer::~RegisterContainer()
{
    delete ui;
}

}// namespace Kites
