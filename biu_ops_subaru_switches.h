#ifndef BIUOPSSUBARUSWITCHES_H
#define BIUOPSSUBARUSWITCHES_H

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QMainWindow>
#include <QSerialPort>
#include <QTime>
#include <QTimer>
#include <QWidget>
#include <QDialog>
#include <QTableWidget>
#include <QGroupBox>
#include <QLabel>

#include <serial_port_actions.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class BiuOpsSubaruSwitchesWindow;
}
QT_END_NAMESPACE

class BiuOpsSubaruSwitches : public QWidget
{
    Q_OBJECT

public:
    explicit BiuOpsSubaruSwitches(QStringList *switch_result, QWidget *parent = nullptr);
    ~BiuOpsSubaruSwitches();

    void update_switch_results(QStringList *switch_result);

private:

    //QGroupBox *gridGroupBox;
    //QLabel *label;

    QStringList *switch_result;

    //SerialPortActions *serial;
    Ui::BiuOpsSubaruSwitchesWindow *ui;

private slots:

signals:



};

#endif // BIUOPSSUBARUSWITCHES_H
