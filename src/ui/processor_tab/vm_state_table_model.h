#pragma once
#include "processor/processor_manager.h"
#include <QAbstractTableModel>


namespace Kites
{
class VMStateTableModel : public QAbstractTableModel
{
    Q_OBJECT
  public:
    explicit VMStateTableModel(QObject *parent = nullptr, ProcessorManager * = nullptr);
    ~VMStateTableModel() = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

  private:
    ProcessorManager *m_vmManager = nullptr;

  public slots:
    void vmStateChangedSlot(const QMap<QString, QVariant> &processorState);
};
} // namespace kites

