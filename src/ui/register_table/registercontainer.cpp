#include "registercontainer.h"
#include "ui_registercontainer.h"
#include "vm/registers.h"
namespace Kites
{
RegisterContainer::RegisterContainer(QWidget *parent, RegisterFile *regfile)
    : QWidget(parent), ui(new Ui::RegisterContainer),
      m_registerModel(new RegisterModel(this, regfile))
{
    ui->setupUi(this);
    ui->tableView->setModel(m_registerModel);
    ui->tableView->verticalHeader()->setVisible(false);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // setupRegisterTable();
    ui->displayType->setCurrentText("Hexadecimal");
    connect(ui->displayType, &QComboBox::currentTextChanged, this,
            [this](const QString &text)
            {
                if (text == "Hexadecimal")
                {
                    m_registerModel->setDisplayBase(Base::Hexadecimal);
                }
                else if (text == "Binary")
                {
                    m_registerModel->setDisplayBase(Base::Binary);
                }
                else if (text == "Decimal/Float")
                {
                    m_registerModel->setDisplayBase(Base::Decimal);
                }
            });
}

void RegisterContainer::setRegisterFile(RegisterFile *regfile)
{
    m_registerModel->changeRegisterFile(regfile);
}

RegisterContainer::~RegisterContainer()
{
    delete ui;
}

} // namespace Kites
