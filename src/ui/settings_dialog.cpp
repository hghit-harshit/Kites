#include "ui/settings_dialog.h"
#include "assembler/custom_pseudo_manager.h"
#include "ui/addpseudo_dialog.h"
#include "ui_settings_dialog.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTableWidget>
#include <QVBoxLayout>

namespace Kites
{
SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    addPage("Custom Pseudo Instructions", createCustomPseudoInstPage());
    addPage("Editor", createEditorPage());
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

QWidget *SettingsDialog::createCustomPseudoInstPage()
{
    QWidget *page = new QWidget(this);

    QTableWidget *table = new QTableWidget(this);
    table->setColumnCount(2);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(table, &QTableWidget::itemChanged, this,
            [](QTableWidgetItem *item) { item->setToolTip(item->text().trimmed()); });

    QStringList headerLabels;
    headerLabels << "Pseudo Instruction" << "Expansion";
    table->setHorizontalHeaderLabels(headerLabels);

    // fill the table with existing custom pseudo instructions
    QString errorMessage;
    auto customPseudos = CustomPseudoManager::loadInstructionsFromDisk(errorMessage);

    /*TODO*/
    /*For now we are just call loadInstructionsFromDisk
    in future maek a public static function that return psuedo instructions
    and handle the error message inside custom_pseudo_manager*/

    for (const QString &key : customPseudos.keys())
    {
        const QJsonObject instructionObject = customPseudos[key].toObject();
        const QStringList args = instructionObject["args"].toVariant().toStringList();
        const QStringList expansion = instructionObject["expansion"].toVariant().toStringList();

        const QString pseudoInst = key + " " + args.join(" ");
        const QString expansionStr = expansion.join("\n");

        int rowCount = table->rowCount();
        table->insertRow(rowCount);
        table->setItem(rowCount, 0, new QTableWidgetItem(pseudoInst));
        table->setItem(rowCount, 1, new QTableWidgetItem(expansionStr));
    }

    QPushButton *addButton = new QPushButton("Add", this);
    QPushButton *updateButton = new QPushButton("Update", this);
    QPushButton *removeButton = new QPushButton("Remove", this);

    connect(addButton, &QPushButton::clicked, this,
            [this, table]()
            {
                AddPseudoDialog dialog(this);
                dialog.setWindowTitle("Add Custom Pseudo Instruction");
                if (dialog.exec() == QDialog::Accepted)
                {
                    // we will get the new pseudo instruction details from the dialog and add it to
                    // the table for now we will just add a dummy row
                    int rowCount = table->rowCount();

                    table->insertRow(rowCount);
                    table->setItem(rowCount, 0,
                                   new QTableWidgetItem(dialog.getPseudoInstruction()));
                    table->setItem(rowCount, 1, new QTableWidgetItem(dialog.getExpansion()));
                }
            });

    connect(updateButton, &QPushButton::clicked, this,
            [this, table]()
            {
                int row = table->currentRow();
                if (row >= 0)
                {
                    QString pseudoInst = table->item(row, 0)->text().trimmed();
                    QString expansion = table->item(row, 1)->text().trimmed();

                    bool isUpdate = true;
                    AddPseudoDialog dialog(this, isUpdate);
                    dialog.setWindowTitle("Update Custom Pseudo Instruction");
                    dialog.setPseudoInstruction(pseudoInst);
                    dialog.setExpansion(expansion);
                    if (dialog.exec() == QDialog::Accepted)
                    {

                        table->item(row, 0)->setText(dialog.getPseudoInstruction());
                        table->item(row, 1)->setText(dialog.getExpansion());
                    }
                }
            });

    connect(removeButton, &QPushButton::clicked, this,
            [this, table]()
            {
                int row = table->currentRow();
                if (row >= 0)
                {
                    table->removeRow(row);
                }
            });

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(updateButton);
    buttonLayout->addWidget(removeButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(table);
    mainLayout->addLayout(buttonLayout);
    page->setLayout(mainLayout);
    return page;
}

QWidget *SettingsDialog::createEditorPage()
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

    connect(ui->settingList, &QListWidget::currentRowChanged, this,
            [this](int currentRow)
            {
                if (currentRow >= 0 && currentRow < m_pages.size())
                {
                    ui->settingPages->setCurrentIndex(m_pages[currentRow].second);
                }
            });
}
} // namespace Kites
