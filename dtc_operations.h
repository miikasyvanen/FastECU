#ifndef DTC_OPERATIONS_H
#define DTC_OPERATIONS_H

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QMainWindow>
#include <QSerialPort>
#include <QTime>
#include <QTimer>
#include <QWidget>

#include <kernelcomms.h>
#include <kernelmemorymodels.h>
#include <file_actions.h>
#include <serial_port_actions.h>
#include <ui_dtc_operations.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
class DtcOperationsWindow;
}
QT_END_NAMESPACE

class DtcOperations : public QDialog
{
    Q_OBJECT

public:
    explicit DtcOperations(SerialPortActions *serial, QWidget *parent = nullptr);
    ~DtcOperations();

    void run();

signals:
    void external_logger(QString message);
    void external_logger(int value);
    void LOG_E(QString message, bool timestamp, bool linefeed);
    void LOG_W(QString message, bool timestamp, bool linefeed);
    void LOG_I(QString message, bool timestamp, bool linefeed);
    void LOG_D(QString message, bool timestamp, bool linefeed);

private:
    FileActions::EcuCalDefStructure *ecuCalDef;
    QString cmd_type;

#define STATUS_SUCCESS							0x00
#define STATUS_ERROR							0x01


    bool kill_process = false;

    int result;

    uint8_t tester_id;
    uint8_t target_id;

    uint16_t receive_timeout = 500;
    uint16_t serial_read_timeout = 2000;
    uint16_t serial_read_extra_short_timeout = 50;
    uint16_t serial_read_short_timeout = 200;
    uint16_t serial_read_medium_timeout = 400;
    uint16_t serial_read_long_timeout = 800;
    uint16_t serial_read_extra_long_timeout = 3000;

    void closeEvent(QCloseEvent *event);

    int init_obd();



    QByteArray add_header(QByteArray output);
    uint8_t calculate_checksum(QByteArray output);

    QString parse_message_to_hex(QByteArray received);
    void delay(int timeout);

    SerialPortActions *serial;
    Ui::DtcOperationsWindow *ui;

};

#endif // DTC_OPERATIONS_H
