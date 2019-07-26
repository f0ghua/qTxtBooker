#include "xsled.h"

#include <QPainter>

XSLed::XSLed(QWidget *parent) :
    QWidget(parent)
{
    m_isOn = false;
    ledOnColor = Qt::green;
    ledOffColor = Qt::red;
    ledOnPattern = Qt::SolidPattern;
    ledOffPattern = Qt::SolidPattern;
}

void XSLed::paintEvent(QPaintEvent *)
{
    int width = this->width();
    int height = this->height();
    QPainter p(this);
    m_isOn ?  p.setBrush(QBrush(ledOnColor, ledOnPattern)) :
            p.setBrush(QBrush(ledOffColor, ledOffPattern));
    p.drawEllipse(0, 0, width-1, height-1);
}

void XSLed::switchXSLed()
{
    m_isOn = ! m_isOn;
    repaint();
}

void XSLed::setState(bool state)
{
    m_isOn = state;
    repaint();
}

void XSLed::toggle()
{
    m_isOn = ! m_isOn;
    repaint();
}

void XSLed::setOnColor(QColor onColor)
{
    ledOnColor=onColor;
    repaint();
}

void XSLed::setOffColor(QColor offColor)
{
    ledOffColor=offColor;
    repaint();
}

void XSLed::setOnPattern(Qt::BrushStyle onPattern)
{
    ledOnPattern=onPattern;
    repaint();
}

void XSLed::setOffPattern(Qt::BrushStyle offPattern)
{
    ledOffPattern=offPattern;
    repaint();
}

void XSLed::setLedSize(int size)
{
    setFixedSize(size+1, size+1);
    repaint();
}
