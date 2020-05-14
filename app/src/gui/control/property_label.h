#ifndef PROPERTY_LABEL_H
#define PROPERTY_LABEL_H

#include <QHBoxLayout>
#include <DLabel>
#include <DFrame>
#include <DLineEdit>

DWIDGET_USE_NAMESPACE

class PropertyLabel : public DFrame
{
    Q_OBJECT

public:
    PropertyLabel(QWidget *parent = nullptr, int margin = 10, int spacing = 10);

    inline void setPropertyName(const QString &name)
    {
        if (m_name) {
            m_name->setText(name);
            m_name->adjustSize();
        }
    }

    void addPropertyValueWidget(QWidget *widget, DPalette::ColorType ct = DPalette::TextTips);
    void addPropertyValueWidget(QWidget *widget, DPalette::ColorRole cr = DPalette::Text);
    void adjustWidth(int width);

    virtual QSize minimumSizeHint() const;
    virtual QSize sizeHint() const;

private:
    QHBoxLayout *m_layout       {};
    DLabel      *m_name         {};
    QWidget     *m_value        {};
};

#endif // PROPERTY_LABEL_H
