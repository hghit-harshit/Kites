#pragma once
#include <QWidget>
#include <QTableWidget>

namespace Kites
{
class RegisterContainer : public QWidget
{
    public:
    explicit RegisterContainer(QWidget* parent = nullptr);
    void resetRegisters();

    private:
    QTableWidget* m_registerTable;
    void setup();

    public slots: 
    void updateRegisterValue(int regIndex, uint64_t);
};
}
