#ifndef FLASH_ECU_SUBARU_DENSO_SH7055_02_H
#define FLASH_ECU_SUBARU_DENSO_SH7055_02_H

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
#include <ui_ecu_operations.h>

//Forward declaration
class SerialPortActions;

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

    #define KERNEL_MAXSIZE_SUB 8*1024U

    uint32_t CRC32 = 0x5AA5A55A;
    bool crc_tab32_init = 0;
    uint32_t crc_tab32[256];

    bool kill_process = false;
    bool kernel_alive = false;
    bool test_write = false;
    bool request_denso_kernel_id = false;
    bool flash_write_init = false;

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

    uint32_t flashmsgsize = 0;
    uint32_t flashblocksize = 0;
    uint32_t flashmessagesize = 0;
    uint32_t flashbytescount = 0;
    uint32_t flashbytesindex = 0;

    QString mcu_type_string;
    QString flash_method;
    QString kernel;

    QByteArray bootloader_init_request_fxt02 = { "\x4D\xFF\xB4" };
    QByteArray bootloader_init_response_stock_fxt02_ok = { "\x4D\x00\xB3" };
    QByteArray bootloader_init_response_ecutek_fxt02_ok = { "\x4D\x00\xB3" };
    QByteArray bootloader_init_response_cobb_fxt02_ok = { "\x4D\x00\xB3" };
    QByteArray bootloader_init_response_fxt02_ok;

    void closeEvent(QCloseEvent *bar);

    int connect_bootloader();
    int upload_kernel(QString kernel, uint32_t kernel_start_addr);
    int read_mem(uint32_t start_addr, uint32_t length);
    int write_mem(bool test_write);
    int get_changed_blocks(const uint8_t *src, int *modified);
    int check_romcrc(const uint8_t *src, uint32_t start, uint32_t len, int *modified);
    int init_flash_write();
    int flash_block(const uint8_t *src, uint32_t start, uint32_t len);
    int reflash_block(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write);

    unsigned int crc32(const unsigned char *buf, unsigned int len);
    void init_crc32_tab(void);
    uint8_t cks_add8(QByteArray chksum_data, unsigned len);

    QByteArray send_sid_bf_ssm_init();
    QByteArray send_sid_27_request_seed();
    QByteArray send_sid_27_send_seed_key(QByteArray seed_key);

    QByteArray request_kernel_init();
    QByteArray request_kernel_id();

    QByteArray add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100);
    uint8_t calculate_checksum(QByteArray output, bool dec_0x100);
    int check_received_message(QByteArray msg, QByteArray received);
    QString parse_message_to_hex(QByteArray received);
    void set_progressbar_value(int value);
    void delay(int timeout);

    SerialPortActions *serial;
    Ui::EcuOperationsWindow *ui;

};

#endif // FLASHDENSOFXT02_H
