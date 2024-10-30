#ifndef FLASH_ECU_SUBARU_UNISIA_JECS_M32R_BOOT_MODE_H
#define FLASH_ECU_SUBARU_UNISIA_JECS_M32R_BOOT_MODE_H

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QMainWindow>
#include <QSerialPort>
#include <QTime>
#include <QTimer>
#include <QWidget>

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

class FlashEcuSubaruUnisiaJecsM32rBootMode : public QDialog
{
    Q_OBJECT

public:
    explicit FlashEcuSubaruUnisiaJecsM32rBootMode(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent);
    ~FlashEcuSubaruUnisiaJecsM32rBootMode();

    void run();

signals:
    void external_logger(QString message);
    void external_logger(int value);

private:
    FileActions::EcuCalDefStructure *ecuCalDef;
    QString cmd_type;

#define STATUS_SUCCESS	0x00
#define STATUS_ERROR	0x01

#define SID_UNISIA_JECS_BLOCK_READ                  0xA0
#define SID_UNISIA_JECS_ADDR_READ                   0xA8
#define SID_UNISIA_JECS_BLOCK_WRITE                 0xB0
#define SID_UNISIA_JECS_ADDR_WRITE                  0xB8
#define SID_UNISIA_JECS_FLASH_READ                  0x00//???

#define SID_UNISIA_JECS_FLASH_ERASE                 0x31//???
#define SID_UNISIA_JECS_FLASH_WRITE                 0x61
#define SID_UNISIA_JECS_FLASH_WRITE_END             0x69//???

    bool kill_process = false;
    bool test_write = false;
    int result;
    int mcu_type_index;
    int bootloader_start_countdown = 3;

    uint8_t tester_id;
    uint8_t target_id;

    uint8_t comm_try_timeout = 50;
    uint8_t comm_try_count = 4;

    uint16_t receive_timeout = 500;
    uint16_t serial_read_extra_short_timeout = 50;
    uint16_t serial_read_short_timeout = 200;
    uint16_t serial_read_medium_timeout = 400;
    uint16_t serial_read_long_timeout = 800;
    uint16_t serial_read_extra_long_timeout = 3000;

    uint32_t flashbytescount = 0;
    uint32_t flashbytesindex = 0;

    QString mcu_type_string;
    QString flash_method;

    void closeEvent(QCloseEvent *event);

    int upload_kernel(QString kernel);
    int read_mem(uint32_t start_addr, uint32_t length);
    int write_mem();

    QByteArray send_subaru_sid_bf_ssm_init();
    QByteArray send_subaru_sid_b8_change_baudrate_4800();
    QByteArray send_subaru_sid_b8_change_baudrate_38400();

    QByteArray add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100);
    uint8_t calculate_checksum(QByteArray output, bool dec_0x100);

    QString parse_message_to_hex(QByteArray received);
    int send_log_window_message(QString message, bool timestamp, bool linefeed);
    void set_progressbar_value(int value);
    void delay(int timeout);

    SerialPortActions *serial;
    Ui::EcuOperationsWindow *ui;

};

#endif // FLASH_ECU_SUBARU_UNISIA_JECS_M32R_BOOT_MODE_H
