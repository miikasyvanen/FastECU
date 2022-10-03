#ifndef LOGBOX_H
#define LOGBOX_H

#include <QWidget>
#include <QGroupBox>
#include <QLabel>
#include <QGuiApplication>
#include <QScreen>
#include <QRect>
#include <QBoxLayout>
#include <QDebug>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class Preferences;
}
QT_END_NAMESPACE

class LogBox : public QWidget
{
    Q_OBJECT
public:
    explicit LogBox(QWidget *parent = nullptr);

    QGroupBox *drawLogBoxes(QString type, uint8_t index, uint8_t switchBoxCount, QString title, QString unit, QString value);
    QGroupBox *drawLogSwitchBox(uint8_t index, uint8_t switchBoxCount, QString title, QString unit, QString value);
    QGroupBox *drawLogValueBox(uint8_t index, uint8_t logBoxCount, QString title, QString unit, QString value);
    void updateSwitchBox();
    void updateLogBox();

private:

signals:

};

#endif // LOGBOX_H
