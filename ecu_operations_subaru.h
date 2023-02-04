#ifndef ECU_OPERATIONS_SUBARU_H
#define ECU_OPERATIONS_SUBARU_H

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
#include <ecu_operations.h>

//#include <stdint.h>
//#include <stdio.h>
//#include <string.h>

//#include "stypes.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class EcuOperationsWindow;
}
QT_END_NAMESPACE

class EcuOperationsSubaru : public QWidget
{
    Q_OBJECT

public:
    explicit EcuOperationsSubaru(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent = nullptr);
    ~EcuOperationsSubaru();

private:
    int ecu_functions(FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type);
    void closeEvent(QCloseEvent *bar);

    #define STATUS_SUCCESS							0x00
    #define STATUS_ERROR							0x01

    #define KERNEL_MAXSIZE_SUB 8*1024U
    uint8_t FILE_SIZE_ERROR = 11;

    bool ssm_init_ok = true;
    bool request_denso_kernel_init = false;
    bool request_denso_kernel_id = false;
    bool test_write = false;
    bool kernel_alive = false;
    bool kill_process = false;

    int mcu_type_index;

    uint8_t bootloader_start_countdown = 3;

    uint8_t comm_try_timeout = 50;
    uint8_t comm_try_count = 4;
    uint16_t receive_timeout = 500;

    QString mcu_type_string;

    SerialPortActions *serial;

    QByteArray subaru_16bit_bootloader_init = { "\x4D\xFF\xB4" };

    QByteArray subaru_16bit_bootloader_init_ok = { "\x4D\x00\xB3" };

    unsigned long serial_read_extra_short_timeout = 50;
    unsigned long serial_read_short_timeout = 200;
    unsigned long serial_read_medium_timeout = 400;
    unsigned long serial_read_long_timeout = 800;
    unsigned long serial_read_extra_long_timeout = 1800;

    void check_mcu_type(QString mcu_type_string);

    int connect_bootloader_subaru_denso_kline_02_16bit();
    int connect_bootloader_subaru_denso_kline_04_16bit();
    int connect_bootloader_subaru_denso_kline_02_32bit();
    int connect_bootloader_subaru_denso_kline_04_32bit();
    int connect_bootloader_subaru_denso_can_32bit();

    int initialize_read_mode_subaru_hitachi_uj20_uj30_kline();
    int initialize_read_mode_subaru_hitachi_uj40_kline();
    int initialize_read_mode_subaru_hitachi_uj70_kline();
    int initialize_read_mode_subaru_hitachi_uj70_can();

    int uninitialize_read_mode_subaru_hitachi_uj20_uj30_kline();

    int initialize_flash_mode_subaru_hitachi_uj20_uj30_kline();
    int initialize_flash_mode_subaru_hitachi_uj40_kline();
    int initialize_flash_mode_subaru_hitachi_uj70_kline();
    int initialize_flash_mode_subaru_hitachi_uj70_can();

    int upload_kernel_subaru_denso_kline_02_16bit(QString kernel);
    int upload_kernel_subaru_denso_kline_04_16bit(QString kernel);
    int upload_kernel_subaru_denso_kline_02_32bit(QString kernel);
    int upload_kernel_subaru_denso_kline_04_32bit(QString kernel);
    int upload_kernel_subaru_denso_can_32bit(QString kernel);

    int read_rom_subaru_denso_kline_02_16bit(FileActions::EcuCalDefStructure *ecuCalDef);
    int read_rom_subaru_denso_kline_04_16bit(FileActions::EcuCalDefStructure *ecuCalDef);
    int read_rom_subaru_denso_kline_32bit(FileActions::EcuCalDefStructure *ecuCalDef);
    int read_rom_subaru_denso_can_32bit(FileActions::EcuCalDefStructure *ecuCalDef);
    int read_rom_subaru_hitachi_uj20_uj30_kline(FileActions::EcuCalDefStructure *ecuCalDef);
    int read_rom_subaru_hitachi_uj40_kline(FileActions::EcuCalDefStructure *ecuCalDef);
    int read_rom_subaru_hitachi_uj70_kline(FileActions::EcuCalDefStructure *ecuCalDef);
    int read_rom_subaru_hitachi_uj70_can(FileActions::EcuCalDefStructure *ecuCalDef);

    int write_rom_subaru_denso_kline_02_16bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write);
    int write_rom_subaru_denso_kline_04_16bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write);
    int write_rom_subaru_denso_kline_32bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write);
    int write_rom_subaru_denso_can_32bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write);
    int write_rom_subaru_hitachi_uj20_uj30_kline(FileActions::EcuCalDefStructure *ecuCalDef);
    int write_rom_subaru_hitachi_uj40_kline(FileActions::EcuCalDefStructure *ecuCalDef);
    int write_rom_subaru_hitachi_uj70_kline(FileActions::EcuCalDefStructure *ecuCalDef);
    int write_rom_subaru_hitachi_uj70_can(FileActions::EcuCalDefStructure *ecuCalDef);

    QByteArray send_subaru_sid_a8_read_mem();
    QByteArray send_subaru_sid_bf_ssm_init();
    QByteArray send_subaru_denso_sid_81_start_communication();
    QByteArray send_subaru_denso_sid_83_request_timings();
    QByteArray send_subaru_denso_sid_27_request_seed();
    QByteArray send_subaru_denso_sid_27_send_seed_key(QByteArray seed_key);
    QByteArray send_subaru_denso_sid_10_start_diagnostic();
    QByteArray send_subaru_denso_sid_34_request_upload(uint32_t dataaddr, uint32_t datalen);
    QByteArray send_subaru_denso_sid_36_transferdata(uint32_t dataaddr, QByteArray buf, uint32_t len);
    QByteArray send_subaru_denso_sid_53_transferdata(uint32_t dataaddr, QByteArray buf, uint32_t len);
    QByteArray send_subaru_denso_sid_31_start_routine();

    QByteArray send_subaru_hitachi_sid_b8_change_baudrate_4800();
    QByteArray send_subaru_hitachi_sid_b8_change_baudrate_38400();
    QByteArray send_subaru_hitachi_sid_af_enter_flash_mode(QByteArray ecu_id);
    QByteArray send_subaru_hitachi_sid_af_erase_memory_block(uint32_t address);
    QByteArray send_subaru_hitachi_sid_af_write_memory_block(uint32_t address, QByteArray payload);
    QByteArray send_subaru_hitachi_sid_af_write_last_memory_block(uint32_t address, QByteArray payload);
    QByteArray send_subaru_hitachi_sid_a0_read_memory_block(uint32_t block_address);
    QByteArray send_subaru_hitachi_sid_a8_read_memory_address(uint32_t address);

    enum RomInfoEnum {
        XmlId,
        InternalIdAddress,
        Make,
        Model,
        SubModel,
        Market,
        Transmission,
        Year,
        EcuId,
        InternalIdString,
        MemModel,
        ChecksumModule,
        RomBase,
        FlashMethod,
        FileSize,
    };

    QByteArray subaru_denso_transform_wrx02_kernel(unsigned char *data, int length, bool doencrypt);
    QByteArray subaru_denso_transform_wrx04_kernel(unsigned char *data, int length, bool doencrypt);
    QByteArray subaru_denso_transform_denso_02_32bit_kernel(QByteArray buf, uint32_t len);

    QByteArray subaru_denso_transform_denso_32bit_payload(QByteArray buf, uint32_t len);
    QByteArray subaru_denso_calculate_32bit_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation);
    QByteArray subaru_denso_generate_kline_seed_key(QByteArray seed);
    QByteArray subaru_denso_generate_can_seed_key(QByteArray requested_seed);
    QByteArray subaru_denso_generate_ecutek_kline_seed_key(QByteArray requested_seed);
    QByteArray subaru_denso_generate_ecutek_can_seed_key(QByteArray requested_seed);
    QByteArray subaru_denso_calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation);

    QByteArray subaru_hitachi_transform_32bit_payload(QByteArray buf, uint32_t len);
    QByteArray subaru_hitachi_generate_kline_seed_key(QByteArray seed);
    QByteArray subaru_hitachi_generate_can_seed_key(QByteArray requested_seed);
    QByteArray subaru_hitachi_calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation);

    QByteArray subaru_denso_encrypt_buf(QByteArray buf, uint32_t pl_len);
    QByteArray subaru_denso_encrypt(const uint8_t *datatoencrypt);


    QByteArray add_ssm_header(QByteArray output, bool dec_0x100);
    int check_received_message(QByteArray msg, QByteArray received);
    int connect_bootloader_start_countdown(int timeout);
    void barrel_shift_16_right(unsigned short *barrel);
    uint8_t calculate_checksum(QByteArray output, bool dec_0x100);
    QString parse_message_to_hex(QByteArray received);

    //uint8_t cks_add8(QByteArray chksum_data, unsigned len);
    //void init_crc16_tab(void);
    //uint16_t crc16(const uint8_t *data, uint32_t siz);

    EcuOperations *ecuOperations;
    Ui::EcuOperationsWindow *ui;

public slots:

private slots:
    int send_log_window_message(QString message, bool timestamp, bool linefeed);
    void delay(int n);

};

#endif // ECU_OPERATIONS_SUBARU_H
