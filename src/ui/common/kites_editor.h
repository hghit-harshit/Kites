#pragma once
#include <QPlainTextEdit>
namespace Kites
{
class GutterColumn; //forward declaration
class KitesEditor : public QPlainTextEdit
{
    Q_OBJECT
    friend class GutterColumn;
public:
    explicit KitesEditor(QWidget *parent = nullptr);
    ~KitesEditor() {};
protected:
    void updateViewPortMargins();
    void updateGutterColumns(const QRect &rect, int dy);
    void highlightCurrentLine();
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

    void addRightGutterColumn(GutterColumn* column);
    void addLeftGutterColumn(GutterColumn* column);

    int rightViewMargin() const;
    int leftViewMargin() const;
private slots:
    void themeChangedSlot(const QString &themeId);
private:
    void applyEditorTheme();
    std::vector<GutterColumn*> m_rightGutterColumns{};
    std::vector<GutterColumn*> m_leftGutterColumns{};
};
} // namespace Kites