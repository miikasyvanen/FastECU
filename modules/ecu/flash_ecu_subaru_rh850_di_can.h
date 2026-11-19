#ifndef FLASH_ECU_SUBARU_RH850_DI_CAN_H
#define FLASH_ECU_SUBARU_RH850_DI_CAN_H

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
#include <ui_ecu_operations.h>

//Forward declaration
class SerialPortActions;

QT_BEGIN_NAMESPACE
namespace Ui { class EcuOperationsWindow; }
QT_END_NAMESPACE

/*
 * Subaru DI (RH850, 2022+) over CAN, UDS-native (no kernel).
 *
 * Reads/writes the application flash of the 2022+ Subaru DIT ECUs (RH850 part,
 * "LHB*" calibrations: WRX VB, Ascent, Legacy/Outback 2.4T). Unlike the SH705x /
 * SH7254x Subaru modules this does NOT upload a kernel; it drives the ECU's own
 * UDS services, the way the factory/Atlas flow does.
 *
 * Protocol facts are all confirmed from stock images + a live flash session
 * (see DIT_Project_FA20_FA24 rh850/docs, 2026-11-18):
 *   - Flash region 0x00010000..0x00400000 (0x3F0000), little-endian.
 *   - Central gateway must be disarmed first: AES SecurityAccess L7 + RoutineControl 0x0200.
 *   - Engine security: AES-128 SecurityAccess (answer = AES128(seed16, key16)).
 *   - Read  = UDS 0x23 ReadMemoryByAddress (ALFID 0x14, <=0x32 bytes/req), plaintext.
 *   - Write = erase RoutineControl 0xFF00, RequestDownload 0x34 (dataFormat 0x04 =
 *             Feistel transport), TransferData 0x36 (<=0xFFE), TransferExit 0x37,
 *             checksum verify RoutineControl 0xFF01.
 */
class FlashEcuSubaruRH850DICan : public QDialog
{
    Q_OBJECT

public:
    explicit FlashEcuSubaruRH850DICan(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent = nullptr);
    ~FlashEcuSubaruRH850DICan();

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

    #define STATUS_SUCCESS  0x00
    #define STATUS_ERROR    0x01

    // --- bus / component arbitration IDs (confirmed on the wire) ---
    const uint32_t CAN_ENGINE_TX   = 0x7A2;  // secure engine (security, read, flash)
    const uint32_t CAN_ENGINE_RX   = 0x7AA;
    const uint32_t CAN_GATEWAY_TX  = 0x763;  // central gateway (disarm)
    const uint32_t CAN_GATEWAY_RX  = 0x76B;
    const uint32_t CAN_OBD_TX      = 0x7E0;  // identify (OBD/DID)
    const uint32_t CAN_OBD_RX      = 0x7E8;

    // --- flash map ---
    const uint32_t FLASH_START = 0x00010000;
    const uint32_t FLASH_END   = 0x00400000;
    const uint32_t READ_CHUNK  = 0x32;       // max bytes per 0x23 request
    const uint32_t WRITE_BLOCK = 0xFF0;      // <= 0xFFE max block from RequestDownload

    // --- security levels ---
    const uint8_t SEC_GATEWAY  = 0x07;       // gateway seed (key = +1)
    const uint8_t SEC_READ     = 0x05;       // engine READ_MEMORY seed
    const uint8_t SEC_WRITE    = 0x01;       // engine FLASH seed (FLASH_ROM)

    /* AES-128 SecurityAccess keys (16 bytes each).
     * NOTE: the working keys for a given car are NOT the ones in the RayTuned
     * seed_keys.txt (verified against a known seed/answer pair: no match). Supply
     * the real per-ECU keys here (from the key set, or recovered via seed-test). */
    uint8_t aes_gateway_key[16] = {0};       // gateway (L7)
    uint8_t aes_read_key[16]    = {0};       // engine read (L5)
    uint8_t aes_write_key[16]   = {0};       // engine flash (L1)

    /* Feistel flash transport key (validated 2026-11-18 on a 2024 WRX image). */
    uint16_t feistel_enc_key[4] = {0xB740, 0x42DA, 0xA7CA, 0x5FB1};
    uint16_t feistel_dec_key[4] = {0x5FB1, 0xA7CA, 0x42DA, 0xB740};

    bool kill_process = false;
    bool test_write = false;
    int  result;
    int  mcu_type_index;
    QString mcu_type_string;
    QString flash_method;

    uint16_t serial_read_timeout = 2000;
    uint16_t serial_read_short_timeout = 200;
    uint16_t serial_read_extra_long_timeout = 8000;   // erase can take several seconds

    uint32_t flashbytescount = 0;
    uint32_t flashbytesindex = 0;

    void closeEvent(QCloseEvent *event);

    int  connect_ecu();
    int  identify();
    int  gateway_disarm();
    int  engine_security(uint8_t seed_level, const uint8_t *aes_key);
    int  read_mem(uint32_t start_addr, uint32_t length);
    int  write_mem(bool test_write);
    int  erase_mem(uint32_t start_addr, uint32_t length);

    // UDS helper: send payload on tx_id, expect reply on rx_id; returns reply payload (without 4-byte id), empty on fail.
    QByteArray uds_request(uint32_t tx_id, uint32_t rx_id, QByteArray payload, uint16_t timeout, bool expect_reply = true);
    bool positive(QByteArray reply, uint8_t req_sid);

    // crypto (validated standalone against FIPS-197 and the captured 2024 flash)
    QByteArray aes128_answer(const uint8_t key[16], QByteArray seed16);
    QByteArray feistel_encrypt_block(QByteArray plain);   // 4-byte-aligned
    QByteArray feistel_decrypt_block(QByteArray cipher);  // 4-byte-aligned

    QString parse_message_to_hex(QByteArray received);
    void set_progressbar_value(int value);
    void delay(int timeout);

    SerialPortActions *serial;
    Ui::EcuOperationsWindow *ui;
};

#endif // FLASH_ECU_SUBARU_RH850_DI_CAN_H
