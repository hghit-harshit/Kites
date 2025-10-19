#include "ui/registercontainer.h"
#include <QheaderView>
#include <QVBoxLayout>
namespace Kites
{
    RegisterContainer::RegisterContainer(QWidget* parent)
        : QWidget(parent)
    {
        setup();
    }


    void RegisterContainer::setup()
    {
        m_registerTable = new QTableWidget();
        m_registerTable->setRowCount(32);
        m_registerTable->setColumnCount(2);

        QStringList headers;
        headers << "Register" << "Value";
        m_registerTable->setHorizontalHeaderLabels(headers);
        m_registerTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_registerTable->verticalHeader()->setVisible(false);


        for (int i = 0; i < 32; ++i) 
        {
            QTableWidgetItem *regName = new QTableWidgetItem(QString("x%1").arg(i));
            regName->setTextAlignment(Qt::AlignCenter);
            m_registerTable->setItem(i, 0, regName);

            QTableWidgetItem *regValue = new QTableWidgetItem("0x00000000");
            regValue->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            m_registerTable->setItem(i, 1, regValue);
        }

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(m_registerTable);
        layout->setContentsMargins(0, 0, 0, 0);
        setLayout(layout);

    }
}