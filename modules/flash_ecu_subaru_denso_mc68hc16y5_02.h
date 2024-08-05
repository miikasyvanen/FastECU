#ifndef FLASH_ECU_SUBARU_DENSO_MC68HC16Y5_02_H
#define FLASH_ECU_SUBARU_DENSO_MC68HC16Y5_02_H

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QMainWindow>
#include <QSerialPort>
#include <QTextEdit>
#include <QTime>
#include <QTimer>
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

class FlashEcuSubaruDensoMC68HC16Y5_02 : public QDialog
{
    Q_OBJECT

public:
    explicit FlashEcuSubaruDensoMC68HC16Y5_02(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent = nullptr);
    ~FlashEcuSubaruDensoMC68HC16Y5_02();

    void run();

signals:
    void external_logger(QString message);
    void external_logger(int value);

private:
    FileActions::EcuCalDefStructure *ecuCalDef;
    QString cmd_type;

    #define STATUS_SUCCESS	0x00
    #define STATUS_ERROR	0x01

    bool kill_process = false;
    bool kernel_alive = false;
    bool test_write = false;
    bool request_denso_kernel_init = false;
    bool request_denso_kernel_id = false;

    int result;
    int mcu_type_index;
    int bootloader_start_countdown = 3;

    uint8_t tester_id;
    uint8_t target_id;

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
    QString kernel;

    QByteArray denso_bootloader_init_request_wrx02 = { "\x4D\xFF\xB4" };
    QByteArray denso_bootloader_init_response_stock_wrx02_ok = { "\x4D\x00\xB3" };
    QByteArray denso_bootloader_init_response_ecutek_wrx02_ok = { "\x4D\x00\xB3" };
    QByteArray denso_bootloader_init_response_cobb_wrx02_ok = { "\x4D\x00\xB3" };
    QByteArray denso_bootloader_init_response_wrx02_ok;

    void closeEvent(QCloseEvent *bar);

    int init_flash_denso_kline_wrx02();
    int connect_bootloader_subaru_denso_kline_wrx02();
    int upload_kernel_subaru_denso_kline_wrx02(QString kernel, uint32_t kernel_start_addr);

    int read_mem_subaru_denso_kline_16bit(uint32_t start_addr, uint32_t length);
    int write_mem_subaru_denso_kline_16bit(bool test_write);
    int get_changed_blocks_16bit_kline(const uint8_t *src, int *modified);
    int check_romcrc_16bit_kline(const uint8_t *src, uint32_t start, uint32_t len, int *modified);
    unsigned int crc32(const unsigned char *buf, unsigned int len);

    QByteArray request_kernel_init();
    QByteArray request_kernel_id();

    QByteArray add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100);
    uint8_t calculate_checksum(QByteArray output, bool dec_0x100);
    int check_received_message(QByteArray msg, QByteArray received);
    int connect_bootloader_start_countdown(int timeout);
    QString parse_message_to_hex(QByteArray received);
    int send_log_window_message(QString message, bool timestamp, bool linefeed);
    void set_progressbar_value(int value);
    void delay(int timeout);

    SerialPortActions *serial;
    Ui::EcuOperationsWindow *ui;

};

#endif // FLASHDENSOWRX02_H
