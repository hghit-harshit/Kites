#pragma once
#include <QProxyStyle>

namespace Kites
{
/**
 * @brief This class is a custom QProxyStyle that will provide styling for the application,
 * regardless of what theme we are using.
 */
class AppStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    int pixelMetric(PixelMetric metric,
                    const QStyleOption *option,
                    const QWidget *widget) const override
    {
        if (metric == PM_SplitterWidth)
            return 1;

        return QProxyStyle::pixelMetric(metric, option, widget);
    }
};
} // namespace Kites