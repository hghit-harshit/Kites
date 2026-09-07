#include "settings_dialog.h"
#include "config/app_settings.h"
#include "custom_pseudo_manager/custom_pseudo_manager.h"
#include "addpseudo_dialog.h"
#include "ui/theme/font_manager.h"
#include "ui/theme/theme_manager.h"
#include "ui_settings_dialog.h"
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

namespace Kites
{
SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    addPage("Appearance", createAppearancePage());
    addPage("Editor", createEditorPage());
    addPage("Custom Pseudo Instructions", createCustomPseudoInstPage());
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
    auto customPseudos = customPseudoManager::loadInstructionsFromDisk(errorMessage);

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
    auto *layout = new QFormLayout(page);

    auto *fontFamilyCombo = new QComboBox(page);
    QStringList families = FontManager::getInstance().recommendedFamilies();
    const QString currentFamily = FontManager::getInstance().currentFamily();
    if (!families.contains(currentFamily))
        families.prepend(currentFamily);
    fontFamilyCombo->addItems(families);
    fontFamilyCombo->setCurrentText(currentFamily);

    auto *fontSizeSpinBox = new QSpinBox(page);
    fontSizeSpinBox->setRange(6, 72);
    fontSizeSpinBox->setValue(FontManager::getInstance().currentSize());

    connect(fontFamilyCombo, &QComboBox::currentTextChanged, this,
            [](const QString &family) { FontManager::getInstance().setFamily(family); });
    connect(fontSizeSpinBox, qOverload<int>(&QSpinBox::valueChanged), this,
            [](int size) { FontManager::getInstance().setSize(size); });

    layout->addRow("Font Family:", fontFamilyCombo);
    layout->addRow("Font Size:", fontSizeSpinBox);

    // --- Autosave -----------------------------------------------------------
    // Crash recovery is always on and is not exposed here; these controls only
    // govern writing back to the user's own file. See src/file_service/.
    auto *autosaveCombo = new QComboBox(page);
    autosaveCombo->addItem("Off (save with Ctrl+S)", static_cast<int>(AutosaveMode::Off));
    autosaveCombo->addItem("After a delay", static_cast<int>(AutosaveMode::AfterDelay));
    autosaveCombo->setCurrentIndex(
        autosaveCombo->findData(static_cast<int>(AppSettings::getInstance().autosaveMode())));

    auto *autosaveDelaySpinBox = new QSpinBox(page);
    autosaveDelaySpinBox->setRange(1, 600);
    autosaveDelaySpinBox->setSuffix(" s");
    autosaveDelaySpinBox->setValue(AppSettings::getInstance().autosaveDelaySeconds());
    autosaveDelaySpinBox->setEnabled(AppSettings::getInstance().autosaveMode() ==
                                     AutosaveMode::AfterDelay);

    connect(autosaveCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [autosaveCombo, autosaveDelaySpinBox](int)
            {
                const auto mode = static_cast<AutosaveMode>(autosaveCombo->currentData().toInt());
                AppSettings::getInstance().setAutosaveMode(mode);
                autosaveDelaySpinBox->setEnabled(mode == AutosaveMode::AfterDelay);
            });
    connect(autosaveDelaySpinBox, qOverload<int>(&QSpinBox::valueChanged), this,
            [](int seconds) { AppSettings::getInstance().setAutosaveDelaySeconds(seconds); });

    layout->addRow("Autosave:", autosaveCombo);
    layout->addRow("Autosave After:", autosaveDelaySpinBox);

    auto *recoveryNote = new QLabel(
        "Unsaved changes are always mirrored to a recovery file and offered back "
        "if Kites exits unexpectedly, regardless of this setting.",
        page);
    recoveryNote->setWordWrap(true);
    layout->addRow(QString(), recoveryNote);

    return page;
}

QWidget *SettingsDialog::createAppearancePage()
{
    QWidget *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto themeLabel = [](const ThemeData &theme)
    {
        const QString appearance = theme.appearance == ThemeAppearance::Dark ? "Dark" : "Light";
        return QString("%1 (%2)").arg(theme.name, appearance);
    };

    auto *globalThemeCombo = new QComboBox(page);
    auto *editorThemeCombo = new QComboBox(page);

    auto repopulateThemeCombos = [themeLabel, globalThemeCombo, editorThemeCombo]()
    {
        const QSignalBlocker blockGlobal(globalThemeCombo);
        const QSignalBlocker blockEditor(editorThemeCombo);
        globalThemeCombo->clear();
        editorThemeCombo->clear();

        editorThemeCombo->addItem("Same as global theme", QString());
        for (const ThemeData &theme : ThemeManager::getInstance().availableThemes())
        {
            globalThemeCombo->addItem(themeLabel(theme), theme.id);
            editorThemeCombo->addItem(themeLabel(theme), theme.id);
        }

        globalThemeCombo->setCurrentIndex(
            globalThemeCombo->findData(ThemeManager::getInstance().currentThemeId()));
        editorThemeCombo->setCurrentIndex(
            editorThemeCombo->findData(ThemeManager::getInstance().configuredEditorThemeId()));
    };
    repopulateThemeCombos();

    connect(globalThemeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [globalThemeCombo](int)
            {
                ThemeManager::getInstance().setTheme(
                    globalThemeCombo->currentData().toString());
            });
    connect(editorThemeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [editorThemeCombo](int)
            {
                ThemeManager::getInstance().setEditorTheme(
                    editorThemeCombo->currentData().toString());
            });

    auto *themeFormLayout = new QFormLayout();
    themeFormLayout->addRow("Global Theme:", globalThemeCombo);
    themeFormLayout->addRow("Editor Theme:", editorThemeCombo);
    layout->addLayout(themeFormLayout);

    auto *installButton = new QPushButton("Install Theme...", page);
    connect(installButton, &QPushButton::clicked, this,
            [this, repopulateThemeCombos]()
            {
                const QString path = QFileDialog::getOpenFileName(
                    this, "Install Theme", QString(), "Theme Files (*.json)");
                if (path.isEmpty())
                    return;

                QString error;
                if (!ThemeManager::getInstance().installTheme(path, error))
                {
                    QMessageBox::warning(this, "Install Theme Failed", error);
                    return;
                }
                repopulateThemeCombos();
            });
    layout->addWidget(installButton);
    layout->addStretch();

    auto *borderFormLayout = new QFormLayout();
    auto *borderStyleCombo = new QComboBox(page);
    borderStyleCombo->addItem("None", static_cast<int>(BorderStyle::None));
    borderStyleCombo->addItem("Subtle", static_cast<int>(BorderStyle::Subtle));
    borderStyleCombo->addItem("Full", static_cast<int>(BorderStyle::Full));
    borderStyleCombo->setCurrentIndex(
        static_cast<int>(AppSettings::getInstance().compilerOutputBorderStyle()));
    connect(borderStyleCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [borderStyleCombo](int)
            {
                AppSettings::getInstance().setCompilerOutputBorderStyle(
                    static_cast<BorderStyle>(borderStyleCombo->currentData().toInt()));
            });
    borderFormLayout->addRow("Compiler Output Border:", borderStyleCombo);
    layout->addLayout(borderFormLayout);

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
                if (currentRow >= 0 && static_cast<size_t>(currentRow) < m_pages.size())
                {
                    ui->settingPages->setCurrentIndex(m_pages[currentRow].second);
                }
            });
}
} // namespace Kites
