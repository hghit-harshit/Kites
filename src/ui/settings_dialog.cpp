#include "ui/settings_dialog.h"
#include "ui_settings_dialog.h"
#include "ui/addpseudo_dialog.h"
#include "assembler/custom_pseudo_manager.h"
#include <QTableWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QHeaderView>
#include <QMessageBox>
namespace Kites
{
SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    
    addPage("Custom Pseudo Instructions", createCustomPseudoInstPage());
    addPage("Editor", createEditorPage());
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

QWidget* SettingsDialog::createCustomPseudoInstPage()
{
    QWidget *page = new QWidget(this);

    QTableWidget* table = new QTableWidget(this);
    table->setColumnCount(2);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(table,&QTableWidget::itemChanged,this,[](QTableWidgetItem* item){
        item->setToolTip(item->text().trimmed());
    });

    QStringList headerLabels;
    headerLabels << "Pseudo Instruction" << "Expansion";
    table->setHorizontalHeaderLabels(headerLabels);

    QPushButton* addButton = new QPushButton("Add", this);
    QPushButton* removeButton = new QPushButton("Remove", this);

    connect(addButton, &QPushButton::clicked, this, [this, table]() {
        AddPseudoDialog dialog(this);
        dialog.setWindowTitle("Add Custom Pseudo Instruction");
        if(dialog.exec() == QDialog::Accepted)
        {
            // we will get the new pseudo instruction details from the dialog and add it to the table
            // for now we will just add a dummy row
            QString errorMessage;
            if(!CustomPseudoManager::addCustomPseudoInstruction(dialog.getPseudoInstruction(), dialog.getExpansion(), errorMessage))
            {
                QMessageBox::critical(nullptr, "Error", errorMessage);
                return;
            }
            int rowCount = table->rowCount();

            table->insertRow(rowCount);
            table->setItem(rowCount, 0, new QTableWidgetItem(dialog.getPseudoInstruction()));
            table->setItem(rowCount, 1, new QTableWidgetItem(dialog.getExpansion()));
        }
    });

    connect(removeButton, &QPushButton::clicked, this, [this, table]() {
        int row = table->currentRow();
        if (row >= 0) {
            table->removeRow(row);
        }
    });

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(table);
    mainLayout->addLayout(buttonLayout);
    page->setLayout(mainLayout);
    return page;
}

QWidget* SettingsDialog::createEditorPage()
{
    QWidget *page = new QWidget(this);
    return page;
}

void SettingsDialog::addPage(const QString &name, QWidget *page)
{
    auto scrollArea = new QScrollArea(this);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(page);

    const int index = ui->settingPages->addWidget(scrollArea);
    m_pages.emplace_back(name, index);

    auto item = new QListWidgetItem(name);
    ui->settingList->addItem(item);

    connect(ui->settingList, &QListWidget::currentRowChanged, this, [this](int currentRow) {
        if (currentRow >= 0 && currentRow < m_pages.size()) {
            ui->settingPages->setCurrentIndex(m_pages[currentRow].second);
        }
    });
} 
}//namespace Kites
