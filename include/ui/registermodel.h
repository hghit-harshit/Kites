#pragma once
#include <QAbstractTableModel>
#include "vm/registers.h"
#include <memory>
#include "ui/display_base_types.h"
namespace Kites
{

/**
 * @brief This class represents a model for displayin and
 * managing the register file in a tabular format.
 * 
 */
class RegisterModel : public QAbstractTableModel
{
    Q_OBJECT
    public:
        explicit RegisterModel(QObject *parent = nullptr,RegisterFile* regfile = nullptr);
        int rowCount(const QModelIndex &parent= QModelIndex()) const override;
        int columnCount(const QModelIndex &parent= QModelIndex()) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

        void setDisplayBase(Base base);
        void changeRegisterFile(RegisterFile* regfile);
        
    private:
        RegisterFile* m_currentRegisterFile;
        Base m_displayBase = Base::Hexadecimal;
        size_t m_highlightedRegisterIndex = -1; // No register highlighted by default
    public slots:
        void updateRegisterValue(size_t regIndex, uint64_t value);
        void registerResetSlot();
        void updateFRegisterValue(size_t regIndex, uint64_t value);

};

}//namespace Kites