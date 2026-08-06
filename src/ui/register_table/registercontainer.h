#ifndef REGISTERCONTAINER_H
#define REGISTERCONTAINER_H

#include "registermodel.h"
#include <QWidget>
#include <memory>

class QSortFilterProxyModel;

namespace Kites
{
namespace Ui
{
class RegisterContainer;
}
class CollapsibleSection;

class RegisterContainer : public QWidget
{
    Q_OBJECT

  public:
    explicit RegisterContainer(QWidget *parent = nullptr, RegisterFile *regfile = nullptr);
    ~RegisterContainer();
    void setRegisterFile(RegisterFile *regfile);
    // private slots:
    //     void updateRegisterValue(size_t regIndex, uint64_t value);
  private:
    void setupRegisterTable();
    QSortFilterProxyModel *makeRangeProxy(int firstRow, int lastRowExclusive);

    Ui::RegisterContainer *ui;
    RegisterModel *m_registerModel;
};
} // namespace Kites
#endif // REGISTERCONTAINER_H
