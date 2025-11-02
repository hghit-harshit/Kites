#pragma once
#include "vm/memory_controller.h"
#include "vm/memory_block.h"
#include <QAbstractTableModel>
#include <memory>
/**
 * @brief This claas represent the model for displaying the memory contents
 * from here we will send data to the view to be displayed
 * 
 */
namespace Kites
{
class MemoryModel : public QAbstractTableModel
{
    Q_OBJECT
    public:
        explicit MemoryModel(QObject* parent = nullptr, MemoryController* memoryController = nullptr);
        int rowCount(const QModelIndex &parent= QModelIndex()) const override;
        int columnCount(const QModelIndex &parent= QModelIndex()) const override;
        QVariant data(const QModelIndex &index,int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        // bool canFetchMore(const QModelIndex &index) const;
        // void fetchMore(const QModelIndex &index);

        //void changeMemoryController(std::shared_ptr<MemoryController> memoryController);
        void changeMemoryController(MemoryController* memoryController);
        void setRowsVisible(int rows);
        void offsetCentralAddress(int offset);
    private:

        //bool isValidAddress(const uint64_t& address, int offset) const;
        bool canOffset(int offset);
        //std::shared_ptr<MemoryController> m_memoryController;
        // MemoryBlock m_currentMemoryBlock;
        int m_rowsVisible = 0;
        uint64_t m_currentCentralAddress = 0xfffffffffffffff;
        // well caculate which row to show when user scrolls pass
        // the current window with this 
        MemoryController* m_memoryController;
    public slots:
       void updateMemory(uint64_t address);
};
} // namespace Kites
