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

    int ecu_functions(FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type);
private:
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

    int connect_bootloader_subaru_kline_16bit();
    int connect_bootloader_subaru_kline_04_16bit();
    //int connect_bootloader_subaru_can_05_32bit();
    int connect_bootloader_subaru_kline_02_32bit();
    int connect_bootloader_subaru_kline_32bit();
    int connect_bootloader_subaru_can_32bit();

    int upload_kernel_subaru_kline_02_16bit(QString kernel);
    int upload_kernel_subaru_kline_04_16bit(QString kernel);
    int upload_kernel_subaru_kline_02_32bit(QString kernel);
    int upload_kernel_subaru_can_05_32bit(QString kernel);
    int upload_kernel_subaru_kline_32bit(QString kernel);
    int upload_kernel_subaru_can_32bit(QString kernel);

    int read_rom_subaru_kline_02_16bit(FileActions::EcuCalDefStructure *ecuCalDef);
    int read_rom_subaru_kline_04_16bit(FileActions::EcuCalDefStructure *ecuCalDef);
    int read_rom_subaru_kline_02_32bit(FileActions::EcuCalDefStructure *ecuCalDef);
    int read_rom_subaru_kline_32bit(FileActions::EcuCalDefStructure *ecuCalDef);
    int read_rom_subaru_can_05_32bit(FileActions::EcuCalDefStructure *ecuCalDef);

    int write_rom_subaru_kline_02_16bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write);
    int write_rom_subaru_kline_04_16bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write);
    int write_rom_subaru_kline_02_32bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write);
    int write_rom_subaru_kline_32bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write);
    int write_rom_subaru_can_05_32bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write);

    QByteArray sub_sid_a8_read_mem();
    QByteArray sub_sid_bf_ssm_init();
    QByteArray sub_sid_81_start_communication();
    QByteArray sub_sid_83_request_timings();
    QByteArray sub_sid_27_request_seed();
    QByteArray sub_sid_27_send_seed_key(QByteArray seed_key);
    QByteArray sub_sid_10_start_diagnostic();
    QByteArray sub_sid_34_request_upload(uint32_t dataaddr, uint32_t datalen);
    QByteArray sub_sid_36_transferdata(uint32_t dataaddr, QByteArray buf, uint32_t len);
    QByteArray sub_sid_31_start_routine();

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

    QByteArray add_ssm_header(QByteArray output, bool dec_0x100);
    QString parse_message_to_hex(QByteArray received);
    int check_received_message(QByteArray msg, QByteArray received);
    void connect_bootloader_start_countdown(int timeout);
    QByteArray sub_generate_seed_key(QByteArray seed);
    uint8_t cks_add8(QByteArray chksum_data, unsigned len);
    void init_crc16_tab(void);
    uint16_t crc16(const uint8_t *data, uint32_t siz);
    uint8_t calculate_checksum(QByteArray output, bool dec_0x100);
    QByteArray sub_encrypt_buf(QByteArray buf, uint32_t pl_len);
    QByteArray sub_encrypt(const uint8_t *datatoencrypt);

    EcuOperations *ecuOperations;
    Ui::EcuOperationsWindow *ui;

public slots:

private slots:
    void send_log_window_message(QString message, bool timestamp, bool linefeed);
    void delay(int n);

};

#endif // ECU_OPERATIONS_SUBARU_H
