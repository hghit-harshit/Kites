#pragma once
#include <QAbstractTableModel>
#include "vm/registers.h"
#include <memory>
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
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

        void changeRegisterFile(RegisterFile* regfile);
        
    private:
        RegisterFile* m_currentRegisterFile;

    public slots:
        void updateRegisterValue(size_t regIndex, uint64_t value);

};

}//namespace Kites