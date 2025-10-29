#include "ui/registermodel.h"


namespace Kites
{

RegisterModel::RegisterModel(QObject *parent,RegisterFile* regfile)
:QAbstractTableModel(parent)
{
    m_currentRegisterFile = regfile;
    connect(m_currentRegisterFile,&RegisterFile::updateRegister,this,&RegisterModel::updateRegisterValue);
}

int RegisterModel::rowCount(const QModelIndex &parent) const
{return 64;}

int RegisterModel::columnCount(const QModelIndex &parent) const
{ return 3; }// Register name, alias, value

QVariant RegisterModel::data(const QModelIndex &index, int role) const
{
    if(!m_currentRegisterFile)
        return QVariant{};
    if(role == Qt::DisplayRole)
    {
        size_t row = static_cast<size_t>(index.row());
        switch(index.column())
        {
            case 0: // Register Name
                return QString("x%1").arg(static_cast<int>(row));
            case 1: // Register Alias
            {
                static const char* reg_aliases[32] = {
                    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
                    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
                    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
                    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
                    // TODO : add alias for floating point registers
                };
                if(row < 32)
                    return QString(reg_aliases[row]);
                else
                    return QString("");
            }
            case 2: // Register Value
            {   
                if(row < 32)
                {
                    uint64_t value = m_currentRegisterFile->ReadGpr(row);
                    return QString("0x%1").arg(QString::number(value,16).toUpper());
                }
                else
                { return QString("");} // TODO : add floating point register support
            }
            default:
                return QVariant{};
        }
    }
    return QVariant{};
}

void RegisterModel::changeRegisterFile(RegisterFile* regfile)
{ 
    beginResetModel();
    m_currentRegisterFile  = regfile;
    endResetModel();
}

void RegisterModel::updateRegisterValue(size_t regIndex, uint64_t value)
{
    QModelIndex index = this->index(static_cast<int>(regIndex),2);
    emit dataChanged(index,index);
}

}// namespaec Kites