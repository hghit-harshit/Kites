#include "memorytab.h"
#include "ui_memorytab.h"
namespace Kites
{

MemoryTab::MemoryTab(QWidget *parent, MemoryController *memoryController)
    : KitesTab(parent), ui(new Ui::MemoryTab),
      m_memoryModel(new MemoryModel(this, memoryController))
{
    ui->setupUi(this);
    ui->memoryTableView->setModel(m_memoryModel);
    ui->memoryTableView->verticalHeader()->setVisible(false);
    ui->memoryTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QRegularExpression re("0x[0-9a-fA-F]+$");
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(re, this);
    ui->searchBar->setValidator(validator);

    // connect(m_ui->Me)
    m_memoryModel->setRowsVisible(1);
    connect(ui->memoryTableView, &MemoryTableView::scrolled, m_memoryModel,
            [this](bool dir)
            {
                if (dir)
                {
                    m_memoryModel->offsetCentralAddress(1);
                }
                else
                {
                    m_memoryModel->offsetCentralAddress(-1);
                }
            });
    connect(ui->memoryTableView, &MemoryTableView::resized, m_memoryModel,
            [=, this]
            {
                int rowHeight = ui->memoryTableView->rowHeight(0);
                if (rowHeight != 0)
                {
                    const auto rows = ui->memoryTableView->height() / rowHeight;
                    m_memoryModel->setRowsVisible(rows);
                }
            });

    connect(ui->searchButton, &QPushButton::clicked, this,
            [this]
            {
                QString searchText = ui->searchBar->text();
                // int displayType = ui->displayTypeComboBox->currentIndex();
                m_memoryModel->setCentralAddress(searchText.toULongLong(nullptr, 16));
            });
    connect(ui->comboBox, &QComboBox::currentTextChanged, this,
            [this](const QString &text)
            {
                if (text == "Hex")
                {
                    m_memoryModel->setDisplayBase(Base::Hexadecimal);
                }
                else if (text == "Binary")
                {
                    m_memoryModel->setDisplayBase(Base::Binary);
                }
                else if (text == "Decimal")
                {
                    m_memoryModel->setDisplayBase(Base::Decimal);
                }
            });
}

void MemoryTab::changeMemoryController(MemoryController *memoryController)
{
    m_memoryModel->changeMemoryController(memoryController);
}

MemoryTab::~MemoryTab()
{
    delete ui;
}
} // namespace Kites
