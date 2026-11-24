#ifndef FLASH_TCU_HITACHI_KLINE_H
#define FLASH_TCU_HITACHI_KLINE_H

#include <QApplication>
#include <QByteArray>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QList>
#include <QMainWindow>
#include <QMessageBox>
#include <QSerialPort>
#include <QTime>
#include <QTimer>
#include <QWidget>

#include <kernelcomms.h>
#include <kernelmemorymodels.h>
#include <file_actions.h>
#include <ui_ecu_operations.h>

//Forward declaration
class SerialPortActions;

QT_BEGIN_NAMESPACE
namespace Ui
{
    class EcuOperationsWindow;
}
QT_END_NAMESPACE

class FlashTcuSubaruHitachiM32rKline : public QDialog
{
    Q_OBJECT

public:
    explicit FlashTcuSubaruHitachiM32rKline(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent = nullptr);
    ~FlashTcuSubaruHitachiM32rKline();

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
    bool flash_in_progress = false;     // erase/write running: window close is refused
    bool test_write = false;            // "test_write" = dry run: unlock + start the flash loader, stop before erasing
    bool kernel_alive = false;
    bool request_denso_kernel_init = false;
    bool request_denso_kernel_id = false;

    int result;
    int mcu_type_index;
    int bootloader_start_countdown = 3;

    uint8_t tester_id;
    uint8_t target_id;

    uint16_t receive_timeout = 500;
    uint16_t serial_read_timeout = 2000;
    uint16_t serial_read_extra_short_timeout = 50;
    uint16_t serial_read_short_timeout = 200;
    uint16_t serial_read_medium_timeout = 500;
    uint16_t serial_read_long_timeout = 800;
    uint16_t serial_read_extra_long_timeout = 3000;

    uint32_t flashbytescount = 0;
    uint32_t flashbytesindex = 0;

    QString mcu_type_string;
    QString flash_method;
    QString kernel;

    QByteArray tcu_rom_id;

    /*
     * On-board flash kernel of the 0x5FFFF-generation K-Line TCU (ROM 0x4778-0x5AE8,
     * run from RAM 0x804000 after SSM 10 85 02). See write_tcu() for the sequence.
     */
    const QString kernel_baudrate = "15625";        // kernel/boot listener: S0BAUR 0x3F, no prescale, 16 MHz
    const uint32_t tcu_write_start = 0x00008000;    // first erased/written address
    const uint32_t tcu_write_end = 0x00060000;      // end of erased/written region (exclusive)
    const uint32_t tcu_range_length = 0x00060000;   // 34 range: start must be 0 (kernel compares addr + 128 to LENGTH)
    const uint32_t tcu_block_size = 0x80;           // 36 payload size (two blocks per 256-byte flash page)
    const int tcu_write_attempts = 3;               // full erase + rewrite attempts
    const int tcu_restart_range_tries = 14;         // ~30 s: after a lost reply the loader can reset and come back as the boot listener
    const int tcu_erase_timeout = 60000;            // ms
    const int tcu_block_timeout = 2000;             // ms
    const int tcu_verify_timeout = 15000;           // ms
    const unsigned long tcu_vbatt_min_mv = 12000;
    const unsigned long tcu_vbatt_warn_mv = 12500;

    void closeEvent(QCloseEvent *event);

    int connect_bootloader();
    int read_a0_rom(uint32_t start_addr, uint32_t length);
    int read_a0_ram(uint32_t start_addr, uint32_t length);
    int read_b8(uint32_t start_addr, uint32_t length);
    int read_b0(uint32_t start_addr, uint32_t length);

    int self_test();
    int check_rom_file();
    int check_battery();
    int check_tcu_conditions();
    int confirm_write(bool recovery);
    int enter_programming_session();
    int kernel_set_range(int tries = 3);
    int kernel_erase();
    int kernel_write_region(const QByteArray &encrypted);
    int kernel_verify();
    int kernel_flash(const QByteArray &encrypted, bool erased);
    int write_tcu();
    int write_tcu_recovery();
    void show_recovery_instructions(QString reason, bool flash_erased);

    QByteArray send_sid_a8_read(QList<uint32_t> addresses);
    QByteArray send_kernel_request(QByteArray payload, int timeout);
    QByteArray read_ssm_response(int timeout);
    bool is_valid_response(const QByteArray &frame);

    QByteArray encrypt_payload(QByteArray buf);

    QByteArray send_sid_a0_block_read(uint32_t dataaddr, uint32_t datalen);
    QByteArray send_sid_b8_byte_read(uint32_t dataaddr);
    QByteArray send_sid_b0_block_write(uint32_t dataaddr, uint32_t datalen);

    QByteArray generate_kline_seed_key(QByteArray seed);
    QByteArray generate_can_seed_key(QByteArray requested_seed);
    QByteArray calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation);

    QByteArray add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100);
    uint8_t calculate_checksum(QByteArray output, bool dec_0x100);

    QString parse_message_to_hex(QByteArray received);
    void set_progressbar_value(int value);
    void delay(int timeout);

    SerialPortActions *serial;
    Ui::EcuOperationsWindow *ui;

};

#endif // FLASH_TCU_HITACHI_KLINE_H
