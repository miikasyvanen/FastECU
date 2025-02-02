#ifndef FLASH_ECU_SUBARU_DENSO_SH7058_CAN_H
#define FLASH_ECU_SUBARU_DENSO_SH7058_CAN_H

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
#include <serial_port/serial_port_actions.h>
#include <ui_ecu_operations.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class EcuOperationsWindow;
}
QT_END_NAMESPACE

class FlashEcuSubaruDensoSH7058Can : public QDialog
{
    Q_OBJECT

public:
    explicit FlashEcuSubaruDensoSH7058Can(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent = nullptr);
    ~FlashEcuSubaruDensoSH7058Can();

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

    #define CRC32   0x5AA5A55A
    bool crc_tab32_init = 0;
    uint32_t crc_tab32[256];

    bool kill_process = false;
    bool kernel_alive = false;
    bool test_write = false;
    bool request_denso_kernel_init = false;
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
    uint16_t serial_read_medium_timeout = 400;
    uint16_t serial_read_long_timeout = 800;
    uint16_t serial_read_extra_long_timeout = 3000;

    uint32_t flashblocksize = 0;
    uint32_t flashmessagesize = 0;
    uint32_t flashbytescount = 0;
    uint32_t flashbytesindex = 0;

    QString mcu_type_string;
    QString flash_method;
    QString kernel;

    uint32_t PTR_DAT_000fa93c = 0;
    uint32_t DAT_000fa924 = 0;

    void closeEvent(QCloseEvent *event);

    int connect_bootloader_subaru_denso_subarucan();
    int upload_kernel_subaru_denso_subarucan(QString kernel, uint32_t kernel_start_addr);
    int read_mem_subaru_denso_subarucan(uint32_t start_addr, uint32_t length);
    int write_mem_subaru_denso_subarucan(bool test_write);
    int get_changed_blocks_denso_subarucan(const uint8_t *src, int *modified);
    int check_romcrc_denso_subarucan(const uint8_t *src, uint32_t start_addr, uint32_t len, int *modified);
    int init_flash_write();
    int flash_block_denso_subarucan(const uint8_t *src, uint32_t start, uint32_t len);
    int reflash_block_denso_subarucan(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write);

    unsigned int crc32(const unsigned char *buf, unsigned int len);
    void init_crc32_tab( void );
    uint8_t cks_add8(QByteArray chksum_data, unsigned len);

    QByteArray send_subaru_sid_bf_ssm_init();
    QByteArray send_subaru_denso_sid_81_start_communication();
    QByteArray send_subaru_denso_sid_83_request_timings();
    QByteArray send_subaru_denso_sid_27_request_seed();
    QByteArray send_subaru_denso_sid_27_send_seed_key(QByteArray seed_key);
    QByteArray send_subaru_denso_sid_10_start_diagnostic();
    QByteArray send_subaru_denso_sid_34_request_upload(uint32_t dataaddr, uint32_t datalen);
    QByteArray send_subaru_denso_sid_36_transferdata(uint32_t dataaddr, QByteArray buf, uint32_t len);
    QByteArray send_subaru_denso_sid_31_start_routine();

    QByteArray subaru_denso_generate_can_seed_key(QByteArray requested_seed);
    QByteArray subaru_denso_generate_ecutek_can_seed_key(QByteArray requested_seed);
    QByteArray subaru_denso_generate_cobb_can_seed_key(QByteArray requested_seed);
    QByteArray subaru_denso_calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation);

    QByteArray request_kernel_init();
    QByteArray request_kernel_id();

    QByteArray subaru_denso_encrypt_32bit_payload(QByteArray buf, uint32_t len);
    QByteArray subaru_denso_decrypt_32bit_payload(QByteArray buf, uint32_t len);
    QByteArray subaru_denso_calculate_32bit_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation);

    QByteArray add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100);
    uint8_t calculate_checksum(QByteArray output, bool dec_0x100);

    QString parse_message_to_hex(QByteArray received);
    void set_progressbar_value(int value);
    void delay(int timeout);

    void FUN_000fac38(uint32_t *param_1,uint32_t *param_2);
    void FUN_000fac5a(uint32_t param_1,uint32_t param_2,uint32_t *param_3,uint32_t *param_4);
    int FUN_000facc8(uint32_t param_1,int param_2);
    void FUN_000facee(uint32_t param_1,uint32_t param_2,int param_3,int *param_4,uint32_t *param_5);
    uint32_t FUN_000fad48(uint32_t param_1,uint32_t param_2,int param_3);
    uint32_t FUN_000fad72(uint32_t param_1,uint32_t param_2,int param_3);
    uint32_t FUN_000fa83e(void);
    QByteArray subaru_denso_calculate_ecutek_racerom_seed_key(uint32_t req_seed);//QByteArray requested_seed);
    QByteArray subaru_denso_generate_ecutek_racerom_can_seed_key(QByteArray requested_seed);
    int decrypt_racerom_seed(int base, int expo, int m);
    unsigned long long modularPow(const unsigned long long base, const unsigned long long exponent, const unsigned long long modulus);

    SerialPortActions *serial;
    Ui::EcuOperationsWindow *ui;

};

#endif // FLASH_ECU_SUBARU_DENSO_SH7058_CAN_H
