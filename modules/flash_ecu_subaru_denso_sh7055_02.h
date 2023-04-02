#ifndef FLASHDENSOFXT02_H
#define FLASHDENSOFXT02_H

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
#include <serial_port_actions.h>
#include <ui_ecu_operations.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class EcuOperationsWindow;
}
QT_END_NAMESPACE

class FlashEcuSubaruDensoSH7055_02 : public QDialog
{
    Q_OBJECT

public:
    explicit FlashEcuSubaruDensoSH7055_02(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent = nullptr);
    ~FlashEcuSubaruDensoSH7055_02();

private:
    #define STATUS_SUCCESS	0x00
    #define STATUS_ERROR	0x01

    #define KERNEL_MAXSIZE_SUB 8*1024U

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

    QByteArray denso_bootloader_init_request_fxt02 = { "\x4D\xFF\xB4" };
    QByteArray denso_bootloader_init_response_stock_fxt02_ok = { "\x4D\x00\xB3" };
    QByteArray denso_bootloader_init_response_ecutek_fxt02_ok = { "\x4D\x00\xB3" };
    QByteArray denso_bootloader_init_response_cobb_fxt02_ok = { "\x4D\x00\xB3" };
    QByteArray denso_bootloader_init_response_fxt02_ok;

    void closeEvent(QCloseEvent *bar);

    int init_flash_denso_kline_fxt02(FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type);
    int connect_bootloader_subaru_denso_kline_fxt02();
    int upload_kernel_subaru_denso_kline_fxt02(QString kernel, uint32_t kernel_start_addr);
    int read_mem_subaru_denso_kline_32bit(FileActions::EcuCalDefStructure *ecuCalDef, uint32_t start_addr, uint32_t length);
    int write_mem_subaru_denso_kline_32bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write);
    int get_changed_blocks_kline_32bit(const uint8_t *src, int *modified);
    int check_romcrc_kline_32bit(const uint8_t *src, uint32_t start, uint32_t len, int *modified);
    int flash_block_kline_32bit(const uint8_t *src, uint32_t start, uint32_t len);
    int reflash_block_kline_32bit(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write);

    uint8_t cks_add8(QByteArray chksum_data, unsigned len);
    void init_crc16_tab(void);
    uint16_t crc16(const uint8_t *data, uint32_t siz);

    QByteArray send_subaru_sid_bf_ssm_init();
    QByteArray send_subaru_denso_sid_27_request_seed();
    QByteArray send_subaru_denso_sid_27_send_seed_key(QByteArray seed_key);

    QByteArray subaru_denso_generate_kline_seed_key(QByteArray seed);
    QByteArray subaru_denso_generate_ecutek_kline_seed_key(QByteArray requested_seed);
    QByteArray subaru_denso_encrypt_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation);
    QByteArray subaru_denso_decrypt_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation);

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

#endif // FLASHDENSOFXT02_H
