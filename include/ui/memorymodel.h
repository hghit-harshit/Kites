#pragma once
#include "vm/memory_controller.h"
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
        //int rowCount(const QModelIndex &parent= QModelIndex()) const override;
        //int columnCount(const QModelIndex &parent= QModelIndex()) const override;
        //QVariant data(const QModelIndex &index,int role = Qt::DisplayRole) const override;

        //void changeMemoryController(std::shared_ptr<MemoryController> memoryController);
        //void changeMemoryController(MemoryController* memoryController);
    private:
        //std::shared_ptr<MemoryController> m_memoryController;
        MemoryController* m_memoryController;
    public slots:
       // void updateMemory(uint64_t address);
};
} // namespace Kites