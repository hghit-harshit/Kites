#ifndef CACHECONFIGWIDGET_H
#define CACHECONFIGWIDGET_H

#include <QWidget>
namespace Kites
{
namespace Ui {
class CacheConfigWidget;
}

class CacheConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CacheConfigWidget(QWidget *parent = nullptr);
    ~CacheConfigWidget();

private:
    Ui::CacheConfigWidget *ui;
};
} // namespace Kites
#endif // CACHECONFIGWIDGET_H

