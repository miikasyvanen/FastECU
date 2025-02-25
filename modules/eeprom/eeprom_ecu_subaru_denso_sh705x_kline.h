#ifndef EEPROM_ECU_SUBARU_DENSO_SH705X_KLINE_H
#define EEPROM_ECU_SUBARU_DENSO_SH705X_KLINE_H

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

class EepromEcuSubaruDensoSH705xKline : public QDialog
{
    Q_OBJECT

public:
    EepromEcuSubaruDensoSH705xKline(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent = nullptr);
    ~EepromEcuSubaruDensoSH705xKline();

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
    bool kernel_alive = false;
    bool test_write = false;
    bool request_denso_kernel_init = false;
    bool request_denso_kernel_id = false;

    int result;
    int mcu_type_index;
    int bootloader_start_countdown = 3;

    int EEPROM_MODE = 0;

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

    void closeEvent(QCloseEvent *bar);

    int connect_bootloader();
    int upload_kernel(QString kernel, uint32_t kernel_start_addr);
    int read_mem(uint32_t start_addr, uint32_t length);


    uint8_t cks_add8(QByteArray chksum_data, unsigned len);
    void init_crc16_tab(void);
    uint16_t crc16(const uint8_t *data, uint32_t siz);

    QByteArray send_sid_bf_ssm_init();
    QByteArray send_sid_81_start_communication();
    QByteArray send_sid_83_request_timings();
    QByteArray send_sid_27_request_seed();
    QByteArray send_sid_27_send_seed_key(QByteArray seed_key);
    QByteArray send_sid_10_start_diagnostic();
    QByteArray send_sid_34_request_upload(uint32_t dataaddr, uint32_t datalen);
    QByteArray send_sid_36_transferdata(uint32_t dataaddr, QByteArray buf, uint32_t len);
    QByteArray send_sid_31_start_routine();

    QByteArray generate_seed_key(QByteArray seed);
    QByteArray generate_ecutek_seed_key(QByteArray requested_seed);
    QByteArray calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation);

    QByteArray request_kernel_id();

    QByteArray encrypt_payload(QByteArray buf, uint32_t len);
    QByteArray decrypt_payload(QByteArray buf, uint32_t len);
    QByteArray calculate_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation);

    QByteArray add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100);
    uint8_t calculate_checksum(QByteArray output, bool dec_0x100);

    QString parse_message_to_hex(QByteArray received);
    void set_progressbar_value(int value);
    void delay(int timeout);

    SerialPortActions *serial;
    Ui::EcuOperationsWindow *ui;

};


#endif // EEPROM_ECU_SUBARU_DENSO_SH705X_KLINE_H
