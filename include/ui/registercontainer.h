#ifndef REGISTERCONTAINER_H
#define REGISTERCONTAINER_H

#include <QWidget>
#include "registermodel.h"
#include <memory>
namespace Kites {
namespace Ui {
class RegisterContainer;
}

class RegisterContainer : public QWidget
{
    Q_OBJECT

public:
    explicit RegisterContainer(QWidget *parent = nullptr,RegisterFile* regfile = nullptr);
    ~RegisterContainer();
// private slots:
//     void updateRegisterValue(size_t regIndex, uint64_t value);
private:
    void setupRegisterTable();
    Ui::RegisterContainer *ui;
    RegisterModel* m_registerModel;
};
}// namespace Kites
#endif // REGISTERCONTAINER_H
