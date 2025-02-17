#ifndef FLASH_ECU_SUBARU_HITACHI_M32R_JTAG_H
#define FLASH_ECU_SUBARU_HITACHI_M32R_JTAG_H

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
#include <ui_ecu_operations.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
class EcuOperationsWindow;
}
QT_END_NAMESPACE

class FlashEcuSubaruHitachiM32rJtag : public QDialog
{
    Q_OBJECT

public:
    explicit FlashEcuSubaruHitachiM32rJtag(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent = nullptr);
    ~FlashEcuSubaruHitachiM32rJtag();

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

    QString IDCODE = "02";
    QString USERCODE = "03";
    QString MDM_SYSTEM = "08";
    QString MDM_CONTROL = "09";
    QString MDM_SETUP = "0a";
    QString MTM_CONTROL = "0f";
    QString MON_CODE = "10";
    QString MON_DATA = "11";
    QString MON_PARAM = "12";
    QString MON_ACCESS = "13";
    QString DMA_RADDR = "18";
    QString DMA_RDATA = "19";
    QString DMA_RTYPE = "1a";
    QString DMA_ACCESS = "1b";
    QString RTDENB = "20";

    bool kill_process = false;
    bool kernel_alive = false;
    bool test_write = false;

    int result;
    int mcu_type_index;
    int bootloader_start_countdown = 3;

    uint8_t comm_try_timeout = 50;
    uint8_t comm_try_count = 4;

    uint16_t receive_timeout = 500;
    uint16_t serial_read_timeout = 2000;
    uint16_t serial_read_extra_short_timeout = 50;
    uint16_t serial_read_short_timeout = 200;
    uint16_t serial_read_medium_timeout = 400;
    uint16_t serial_read_long_timeout = 800;
    uint16_t serial_read_extra_long_timeout = 3000;

    uint16_t jtag_timeout = 5;
    uint16_t jtag_loopcount_max = 2000;

    uint32_t flashbytescount = 0;
    uint32_t flashbytesindex = 0;

    QString mcu_type_string;
    QString flash_method;
    QString kernel;

    void closeEvent(QCloseEvent *event);

    int init_jtag();
    int read_mem(uint32_t start_addr, uint32_t length);
    int write_mem(bool test_write);

    QString parse_message_to_hex(QByteArray received);
    void set_progressbar_value(int value);
    void delay(int timeout);

    void hard_reset_jtag();
    void read_idcode();
    void read_usercode();
    void set_rtdenb();
    void read_tool_rom_code();



    void write_jtag_ir(QString desc, QString code);
    QByteArray read_jtag_dr(QString desc);
    QByteArray write_jtag_dr(QString desc, QString data);
    QByteArray read_response();

    SerialPortActions *serial;
    Ui::EcuOperationsWindow *ui;

};

#endif // FLASH_ECU_SUBARU_HITACHI_M32R_JTAG_H
