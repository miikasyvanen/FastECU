#ifndef FLASH_ECU_SUBARU_UNISIA_JECS_H
#define FLASH_ECU_SUBARU_UNISIA_JECS_H

#include <QWidget>

#include <kernelcomms.h>
#include <kernelmemorymodels.h>
#include <file_actions.h>
#include <serial_port/serial_port_actions.h>
#include <ui_ecu_operations.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
class EcuOperationsWindow;
}
QT_END_NAMESPACE

class FlashEcuSubaruUnisiaJecs : public QDialog
{
    Q_OBJECT

public:
    FlashEcuSubaruUnisiaJecs(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent = nullptr);
    ~FlashEcuSubaruUnisiaJecs();

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

    #define STATUS_SUCCESS	0x00
    #define STATUS_ERROR	0x01

    bool kill_process = false;

    int mcu_type_index;
    QString mcu_type_string;
    QString flash_method;
    QString kernel;

    uint16_t receive_timeout = 500;
    uint16_t serial_read_timeout = 2000;
    uint16_t serial_read_extra_short_timeout = 50;
    uint16_t serial_read_short_timeout = 200;
    uint16_t serial_read_medium_timeout = 400;
    uint16_t serial_read_long_timeout = 800;
    uint16_t serial_read_extra_long_timeout = 3000;

    void closeEvent(QCloseEvent *bar);

    int read_mem(uint32_t start_addr, uint32_t length);

    QString parse_message_to_hex(QByteArray received);
    void set_progressbar_value(int value);
    void delay(int timeout);


    SerialPortActions *serial;
    Ui::EcuOperationsWindow *ui;

};

#endif // FLASH_ECU_SUBARU_UNISIA_JECS_H
