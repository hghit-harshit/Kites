#include "registercontainer.h"
#include "ui_registercontainer.h"
#include "processor/registers.h"
#include "ui/common/collapsible_section.h"
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QVBoxLayout>
#include <array>

namespace Kites
{
namespace
{
// Filters a RegisterModel down to a contiguous row range, so General
// Purpose / Floating Point registers can be shown in separate collapsible
// sections while sharing the same underlying model.
class RegisterRangeProxyModel : public QSortFilterProxyModel
{
  public:
    RegisterRangeProxyModel(int firstRow, int lastRowExclusive, QObject *parent = nullptr)
        : QSortFilterProxyModel(parent), m_firstRow(firstRow), m_lastRowExclusive(lastRowExclusive)
    {
    }

  protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        Q_UNUSED(sourceParent);
        return sourceRow >= m_firstRow && sourceRow < m_lastRowExclusive;
    }

  private:
    int m_firstRow;
    int m_lastRowExclusive;
};
} // namespace

RegisterContainer::RegisterContainer(QWidget *parent, RegisterFile *regfile)
    : QWidget(parent), ui(new Ui::RegisterContainer),
      m_registerModel(new RegisterModel(this, regfile))
{
    ui->setupUi(this);
    setupRegisterTable();

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

QSortFilterProxyModel *RegisterContainer::makeRangeProxy(int firstRow, int lastRowExclusive)
{
    auto *proxy = new RegisterRangeProxyModel(firstRow, lastRowExclusive, this);
    proxy->setSourceModel(m_registerModel);
    return proxy;
}

void RegisterContainer::setupRegisterTable()
{
    auto *sectionsLayout = new QVBoxLayout(ui->sectionsContainer);
    sectionsLayout->setContentsMargins(0, 0, 0, 0);
    sectionsLayout->setSpacing(4);

    struct SectionSpec
    {
        QString title;
        int firstRow;
        int lastRowExclusive;
    };
    const std::array<SectionSpec, 2> sections = {
        SectionSpec{"General Purpose", 0, 32},
        SectionSpec{"Floating Point", 32, 64},
    };

    for (const SectionSpec &spec : sections)
    {
        auto *section = new CollapsibleSection(spec.title, ui->sectionsContainer);

        auto *tableView = new QTableView(section);
        tableView->setModel(makeRangeProxy(spec.firstRow, spec.lastRowExclusive));
        tableView->verticalHeader()->setVisible(false);
        tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

        section->setContentWidget(tableView);
        sectionsLayout->addWidget(section);
    }
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
