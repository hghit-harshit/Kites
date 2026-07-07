#pragma once
#include "ui/common/display_base_types.h"
#include "processor/memory_block.h"
#include "processor/memory_controller.h"
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
    explicit MemoryModel(QObject *parent = nullptr, MemoryController *memoryController = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    // bool canFetchMore(const QModelIndex &index) const;
    // void fetchMore(const QModelIndex &index);

    // void changeMemoryController(std::shared_ptr<MemoryController> memoryController);
    void setDisplayBase(Base base);
    void changeMemoryController(MemoryController *memoryController);
    void setRowsVisible(int rows);
    void offsetCentralAddress(int offset);
    void setCentralAddress(const uint64_t &address);
    // this will be used to search memory for a specific value

  private:
    // bool isValidAddress(const uint64_t& address, int offset) const;
    bool canOffset(int offset);

    int m_rowsVisible = 10; // it does not matter as it will be calculater later anyways
    // well caculate which row to show when user scrolls pass
    // the current window with this
    uint64_t m_currentCentralAddress = 64; // please keep this a multiple of 8
    // other wise  this crashes fuck my life
    Base m_displayBase = Base::Hexadecimal;
    MemoryController *m_memoryController;
  public slots:
    void memoryResetSlot();
    void updateMemory(uint64_t address);
};
} // namespace Kites
