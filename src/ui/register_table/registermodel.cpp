#include "registermodel.h"
#include <QApplication>
#include <QBrush>
#include <QDebug>
#include <QPalette>

namespace Kites
{

RegisterModel::RegisterModel(QObject *parent, RegisterFile *regfile) : QAbstractTableModel(parent)
{
    m_currentRegisterFile = regfile;
    connect(m_currentRegisterFile, &RegisterFile::updateRegister, this,
            &RegisterModel::updateRegisterValue);
    connect(m_currentRegisterFile, &RegisterFile::updateFRegister, this,
            &RegisterModel::updateFRegisterValue);
    connect(m_currentRegisterFile, &RegisterFile::registerResetSignal, this,
            &RegisterModel::registerResetSlot);
}

int RegisterModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 64;
}

int RegisterModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3;
} // Register name, alias, value

void RegisterModel::setDisplayBase(Base base)
{
    beginResetModel();
    m_displayBase = base;
    endResetModel();
}

QVariant RegisterModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant{};

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return QString("Register");
        case 1:
            return QString("Alias");
        case 2:
            return QString("Value");
        }
    }

    return QVariant{};
}

QVariant RegisterModel::data(const QModelIndex &index, int role) const
{
    if (!m_currentRegisterFile)
        return QVariant{};

    if (role == Qt::TextAlignmentRole)
    {
        return Qt::AlignCenter;
    }
    if (role == Qt::BackgroundRole)
    {
        if (static_cast<size_t>(index.row()) == m_highlightedRegisterIndex)
        {
            return QBrush(QColor(80, 20, 20)); // Highlight color for the updated register
        }
    }
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole)
    {
        size_t row = static_cast<size_t>(index.row());
        switch (index.column())
        {
        case 0: // Register Name
            if (row < 32)
            {
                return QString("x%1").arg(row);
            }
            else
            {
                return QString("f%1").arg(row - 32);
            }
        case 1: // Register Alias
        {
            // QString expect char* that why we don use vectoor of string here
            // or array of string
            static const char *reg_aliases[64] = {
                // Integer registers x0–x31
                "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0", "a1", "a2",
                "a3", "a4", "a5", "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10",
                "s11", "t3", "t4", "t5", "t6",

                // Floating-point registers f0–f31
                "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7", "fs0", "fs1", "fa0", "fa1",
                "fa2", "fa3", "fa4", "fa5", "fa6", "fa7", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7",
                "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11"};
            if (row < 64)
                return QString(reg_aliases[row]);
            else
                return QString("");
        }
        case 2: // Register Value
        {
            if (row < 32)
            {
                uint64_t value = m_currentRegisterFile->ReadGpr(row);
                switch (m_displayBase)
                {
                case Base::Binary:
                    return QString("0b%1").arg(QString::number(value, 2));
                case Base::Decimal:
                    return QString::number(static_cast<int64_t>(value));
                case Base::Hexadecimal:
                    return QString("0x%1").arg(QString::number(value, 16).toUpper());
                }
            }
            else if (row < 64)
            {
                int fIndex = row - 32;

                // read raw bits of FP register (you must have this function)
                uint64_t raw = m_currentRegisterFile->ReadFpr(fIndex);

                switch (m_displayBase)
                {
                case Base::Binary:
                {
                    return QString("0b%1").arg(QString::number(raw, 2));
                }

                case Base::Decimal:
                {
                    uint32_t upper32 = static_cast<uint32_t>(raw >> 32);
                    if (upper32 == 0)
                    {
                        // Single-precision float (F-extension): value in lower 32 bits
                        uint32_t lower32 = static_cast<uint32_t>(raw & 0xFFFFFFFF);
                        float fval;
                        memcpy(&fval, &lower32, sizeof(float));
                        return QString::number(static_cast<double>(fval), 'g', 8);
                    }
                    else
                    {
                        // Double-precision (D-extension): full 64 bits
                        double dval;
                        memcpy(&dval, &raw, sizeof(double));
                        return QString::number(dval, 'g', 16);
                    }
                }

                case Base::Hexadecimal:
                {
                    return QString("0x%1").arg(QString::number(raw, 16).toUpper());
                }
                }
            }
            else
            {
                return QString("");
            } // TODO : add floating point register support
        }
        default:
            return QVariant{};
        }
    }
    return QVariant{};
}

void RegisterModel::changeRegisterFile(RegisterFile *regfile)
{
    beginResetModel();
    m_currentRegisterFile = regfile;
    connect(m_currentRegisterFile, &RegisterFile::updateRegister, this,
            &RegisterModel::updateRegisterValue);
    connect(m_currentRegisterFile, &RegisterFile::updateFRegister, this,
            &RegisterModel::updateFRegisterValue);
    // maybe we can directly connect it to
    connect(m_currentRegisterFile, &RegisterFile::registerResetSignal, this,
            &RegisterModel::registerResetSlot);
    endResetModel();
}

void RegisterModel::registerResetSlot()
{
    qDebug() << "RegisterModel::registerResetSlot called";
    beginResetModel();
    endResetModel();
}

void RegisterModel::updateRegisterValue(size_t regIndex, uint64_t value)
{
    qDebug() << "RegisterModel::updateRegisterValue called for regIndex:" << regIndex
             << " new value:" << value;
    //QModelIndex index = this->index(static_cast<int>(regIndex), 2);
    m_highlightedRegisterIndex = regIndex; // Update highlighted register index
    // emit dataChanged(index,index);
    beginResetModel();
    endResetModel();
}

void RegisterModel::updateFRegisterValue(size_t regIndex, uint64_t value)
{
    qDebug() << "RegisterModel::updateFRegisterValue called for regIndex:" << regIndex
             << " new value:" << value;
    //QModelIndex index = this->index(static_cast<int>(regIndex + 32), 2);
    m_highlightedRegisterIndex = regIndex + 32; // Update highlighted register index
    // emit dataChanged(index,index);
    beginResetModel();
    endResetModel();
}
} // namespace Kites
