#ifndef ECUOPERATIONSRENAULT_H
#define ECUOPERATIONSRENAULT_H

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QMainWindow>
#include <QSerialPort>
#include <QTime>
#include <QTimer>
#include <QWidget>

#include <file_actions.h>
#include <serial_port_actions.h>
#include <ecu_operations.h>
#include <ecu_operations_iso14230.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class EcuOperationsWindow;
}
QT_END_NAMESPACE

class EcuOperationsMercedes : public QWidget
{
    Q_OBJECT

public:
    EcuOperationsMercedes(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent = nullptr);
    ~EcuOperationsMercedes();

private:
    void closeEvent(QCloseEvent *bar);
    int ecu_functions(FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type);

    #define STATUS_SUCCESS							0x00
    #define STATUS_ERROR							0x01

    bool kill_process = false;

    SerialPortActions *serial;

    EcuOperationsIso14230 *ecuOperationsIso14230;
    Ui::EcuOperationsWindow *ui;

    QString parse_message_to_hex(QByteArray received);
    int send_log_window_message(QString message, bool timestamp, bool linefeed);
    void delay(int n);

    ulong HexToLong(QString hex);
    QByteArray sub_calculate_seed_key_crd3(QByteArray requested_seed);
    QByteArray sub_calculate_seed_key_cr3_up_sid27_01(QByteArray requested_seed);
    QByteArray sub_calculate_seed_key_cr3_up_sid27_10(QByteArray requested_seed);
    QByteArray sub_calculate_seed_key_med17_5_sid27_01(QByteArray requested_seed);
    QByteArray sub_calculate_seed_key_med91_sid27_01(QByteArray requested_seed);
    QByteArray sub_calculate_seed_key_TEST_sid27_01(QByteArray requested_seed);
    QByteArray sub_calculate_seed_key_FORCE_sid27_10(QByteArray requested_seed);

};

#endif // ECUOPERATIONSRENAULT_H
