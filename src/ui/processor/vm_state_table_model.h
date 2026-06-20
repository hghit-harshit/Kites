#pragma once
#include "vm/vm_manager.h"
#include <QAbstractTableModel>


namespace Kites
{
class VMStateTableModel : public QAbstractTableModel
{
    Q_OBJECT
  public:
    explicit VMStateTableModel(QObject *parent = nullptr, VMManager * = nullptr);
    ~VMStateTableModel() = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

  private:
    VMManager *m_vmManager = nullptr;

  public slots:
    void vmStateChangedSlot(const QMap<QString, QVariant> &vmState);
};
} // namespace kites

