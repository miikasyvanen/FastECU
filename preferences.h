#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QMainWindow>
#include <QDebug>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class Preferences;
}
QT_END_NAMESPACE

class Preferences : public QWidget
{
    Q_OBJECT

public:
    Preferences(QWidget *parent = nullptr);
    ~Preferences();

private slots:

private:
    Ui::Preferences *ui;
};

#endif // PREFERENCES_H
