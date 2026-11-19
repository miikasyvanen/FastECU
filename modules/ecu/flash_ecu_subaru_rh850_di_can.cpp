#include "flash_ecu_subaru_rh850_di_can.h"
#include "serial_port_actions.h"
#include <QCloseEvent>
#include <QFile>
#include <QDir>
#include <QCoreApplication>

/*
 * Subaru DI (RH850, 2022+) UDS-native read/write over CAN.
 * Crypto (AES-128, Feistel transport) validated standalone against FIPS-197 and
 * the captured 2024 WRX flash before being embedded here.
 * NOT yet compiled against the Qt tree — build on the target before use.
 */

// ---- AES-128 (SecurityAccess answer) ----
static const uint8_t AES_SBOX[256] = {
0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16 };

static inline uint8_t aes_xtime(uint8_t a){ return (uint8_t)((a<<1) ^ ((a>>7)*0x1b)); }

static void aes128_encrypt_block(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]) {
    uint8_t rk[176]; memcpy(rk, key, 16); uint8_t rcon = 1;
    for (int i = 16; i < 176; i += 4) {
        uint8_t t[4]; memcpy(t, rk + i - 4, 4);
        if (i % 16 == 0) {
            uint8_t tmp = t[0]; t[0]=AES_SBOX[t[1]]^rcon; t[1]=AES_SBOX[t[2]]; t[2]=AES_SBOX[t[3]]; t[3]=AES_SBOX[tmp];
            rcon = aes_xtime(rcon);
        }
        for (int j = 0; j < 4; j++) rk[i+j] = rk[i-16+j] ^ t[j];
    }
    uint8_t s[16]; memcpy(s, in, 16);
    for (int j=0;j<16;j++) s[j]^=rk[j];
    for (int round=1; round<=10; round++) {
        uint8_t a[16]; for (int j=0;j<16;j++) a[j]=AES_SBOX[s[j]];
        uint8_t t[16];
        t[0]=a[0];  t[4]=a[4];  t[8]=a[8];   t[12]=a[12];
        t[1]=a[5];  t[5]=a[9];  t[9]=a[13];  t[13]=a[1];
        t[2]=a[10]; t[6]=a[14]; t[10]=a[2];  t[14]=a[6];
        t[3]=a[15]; t[7]=a[3];  t[11]=a[7];  t[15]=a[11];
        if (round != 10)
            for (int c=0;c<4;c++){
                uint8_t *col=t+4*c; uint8_t s0=col[0],s1=col[1],s2=col[2],s3=col[3];
                col[0]=aes_xtime(s0)^(aes_xtime(s1)^s1)^s2^s3;
                col[1]=s0^aes_xtime(s1)^(aes_xtime(s2)^s2)^s3;
                col[2]=s0^s1^aes_xtime(s2)^(aes_xtime(s3)^s3);
                col[3]=(aes_xtime(s0)^s0)^s1^s2^aes_xtime(s3);
            }
        for (int j=0;j<16;j++) s[j]=t[j]^rk[16*round+j];
    }
    memcpy(out, s, 16);
}

// ---- Subaru DI Feistel transport (32-bit block, 4 rounds) ----
static const uint8_t FEISTEL_LUT[32] = {
0x05,0x06,0x07,0x01,0x09,0x0c,0x0d,0x08,0x0a,0x0d,0x02,0x0b,0x0f,0x04,0x00,0x03,
0x0b,0x04,0x06,0x00,0x0f,0x02,0x0d,0x09,0x05,0x0c,0x01,0x0a,0x03,0x0d,0x0e,0x08 };

static uint32_t feistel_dec_sym(uint32_t sym, const uint16_t keys[4]) {
    uint32_t local_c = sym; int uVar2 = 4, ki = 0;
    for (int r=0;r<4;r++){
        int uVar6=uVar2,uVar4=0; uint16_t uVar1=keys[ki++];
        uVar2=(uVar1^local_c)&0xFFFF; int uVar3=uVar2&1; uint8_t ab8[4]={0,0,0,0};
        while(true){ ab8[uVar4]=uVar2&0x1f; uVar2=(uint32_t)uVar2>>4; uVar4=(uVar4+1)&0xff;
            if(uVar4>3)break; if(uVar4==3&&uVar3==1)uVar2|=0x10; }
        uint8_t ab4[4]; for(int i=0;i<4;i++)ab4[i]=FEISTEL_LUT[ab8[i]];
        uVar2=(ab4[0]+ab4[1]*0x10+ab4[2]*0x100+ab4[3]*0x1000)&0xFFFF;
        for(int i=0;i<3;i++){int b=uVar2&1;uVar2>>=1;if(b)uVar2|=0x8000;}
        local_c=((uVar2^(local_c>>16))&0xFFFF)|(local_c<<16); uVar2=uVar6-1;
    }
    return local_c;
}
static uint32_t feistel_enc_sym(uint32_t cleartext, const uint16_t keys[4]) {
    uint16_t low=cleartext&0xFFFF, high=(cleartext>>16)&0xFFFF;
    uint32_t local_c=((uint32_t)low<<16)|high; int uVar2=4,ki=0;
    for (int r=0;r<4;r++){
        int uVar6=uVar2,uVar4=0; uint16_t uVar1=keys[ki++];
        uVar2=(uVar1^(local_c>>16))&0xFFFF; int uVar3=uVar2&1; uint8_t ab8[4]={0,0,0,0};
        while(true){ ab8[uVar4]=uVar2&0x1f; uVar2=(uint32_t)uVar2>>4; uVar4=(uVar4+1)&0xff;
            if(uVar4>3)break; if(uVar4==3&&uVar3==1)uVar2|=0x10; }
        uint8_t ab4[4]; for(int i=0;i<4;i++)ab4[i]=FEISTEL_LUT[ab8[i]];
        uVar2=(ab4[0]+ab4[1]*0x10+ab4[2]*0x100+ab4[3]*0x1000)&0xFFFF;
        for(int i=0;i<3;i++){int b=uVar2&1;uVar2>>=1;if(b)uVar2|=0x8000;}
        local_c=(((uVar2^local_c)&0xFFFF)<<16)|((local_c>>16)&0xFFFF); uVar2=uVar6-1;
    }
    return local_c;
}

FlashEcuSubaruRH850DICan::FlashEcuSubaruRH850DICan(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
    : QDialog(parent), ecuCalDef(ecuCalDef), cmd_type(cmd_type), serial(serial)
{
    if (cmd_type == "test_write")      this->setWindowTitle("Test write ROM " + ecuCalDef->FileName + " to ECU");
    else if (cmd_type == "write")      this->setWindowTitle("Write ROM " + ecuCalDef->FileName + " to ECU");
    else if (cmd_type == "read")       this->setWindowTitle("Read ROM from ECU");
}

FlashEcuSubaruRH850DICan::~FlashEcuSubaruRH850DICan() { }

void FlashEcuSubaruRH850DICan::run()
{
    result = STATUS_ERROR;
    flash_method = ecuCalDef->FlashMethod;
    test_write = (cmd_type == "test_write");

    // ISO-TP over 500k CAN, 11-bit IDs.
    serial->set_is_iso14230_connection(false);
    serial->set_add_iso14230_header(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(true);
    serial->set_is_29_bit_id(false);
    serial->set_can_speed("500000");
    serial->set_iso15765_source_address(CAN_OBD_TX);
    serial->set_iso15765_destination_address(CAN_OBD_RX);
    serial->open_serial_port();

    if (!serial->is_serial_port_open()) {
        emit LOG_E("ERROR: Serial port is not open.", true, true);
        return;
    }

    load_keys();

    if (connect_ecu() != STATUS_SUCCESS) { emit external_logger("Finished"); return; }

    if (cmd_type == "read") {
        emit LOG_I("Reading RH850 DI ROM via UDS 0x23", true, true);
        result = read_mem(FLASH_START, FLASH_END - FLASH_START);
    } else if (cmd_type == "write" || cmd_type == "test_write") {
        emit LOG_I(QString("Writing RH850 DI ROM via UDS (test=%1)").arg(test_write), true, true);
        result = write_mem(test_write);
    } else {
        emit LOG_D("Unknown operation selected!", true, true);
    }
    emit external_logger("Finished");
}

// Open session, identify, disarm gateway. Engine security is taken per-operation.
int FlashEcuSubaruRH850DICan::connect_ecu()
{
    if (identify() != STATUS_SUCCESS) return STATUS_ERROR;
    if (gateway_disarm() != STATUS_SUCCESS) return STATUS_ERROR;
    return STATUS_SUCCESS;
}

int FlashEcuSubaruRH850DICan::identify()
{
    emit LOG_I("Requesting CAL ID...", true, true);
    // ReadDataByIdentifier of the CAL ID DID on the OBD channel.
    QByteArray req; req.append((uint8_t)0x22); req.append((uint8_t)0xF1); req.append((uint8_t)0x94);
    QByteArray rep = uds_request(CAN_OBD_TX, CAN_OBD_RX, req, serial_read_timeout);
    if (positive(rep, 0x22) && rep.length() > 3) {
        QString cal = QString(rep.mid(3));
        emit LOG_I("CAL ID: " + cal, true, true);
        if (cmd_type == "read") ecuCalDef->RomId = cal;
    } else {
        emit LOG_W("No CAL ID via DID 0xF194 (continuing)", true, true);
    }
    return STATUS_SUCCESS;
}

int FlashEcuSubaruRH850DICan::gateway_disarm()
{
    if (is_zero_key(aes_gateway_key)) {
        emit LOG_W("No gateway key set - skipping gateway disarm and trying the engine directly.", true, true);
        emit LOG_W("If engine security then fails, the central gateway requires its L7 key.", true, true);
        return STATUS_SUCCESS;
    }
    emit LOG_I("Disarming central gateway (0x763)...", true, true);
    // Extended session
    QByteArray s; s.append((uint8_t)0x10); s.append((uint8_t)0x03);
    if (!positive(uds_request(CAN_GATEWAY_TX, CAN_GATEWAY_RX, s, serial_read_timeout), 0x10)) {
        emit LOG_E("Gateway session control failed", true, true); return STATUS_ERROR;
    }
    if (engine_security(SEC_GATEWAY, aes_gateway_key) != STATUS_SUCCESS) {
        // engine_security() is generic; here it targets the gateway component via SEC_GATEWAY.
        emit LOG_E("Gateway security failed", true, true); return STATUS_ERROR;
    }
    // RoutineControl START 0x0200 -> allow comms to the ECU
    QByteArray r; r.append((uint8_t)0x31); r.append((uint8_t)0x01); r.append((uint8_t)0x02); r.append((uint8_t)0x00);
    if (!positive(uds_request(CAN_GATEWAY_TX, CAN_GATEWAY_RX, r, serial_read_timeout), 0x31)) {
        emit LOG_E("Gateway RoutineControl 0x0200 failed", true, true); return STATUS_ERROR;
    }
    emit LOG_I("Gateway disarmed.", true, true);
    return STATUS_SUCCESS;
}

// Generic AES SecurityAccess. seed_level = 0x05/0x01 (engine) or 0x07 (gateway).
// Targets the gateway component for SEC_GATEWAY, otherwise the engine component.
int FlashEcuSubaruRH850DICan::engine_security(uint8_t seed_level, const uint8_t *aes_key)
{
    uint32_t tx = (seed_level == SEC_GATEWAY) ? CAN_GATEWAY_TX : CAN_ENGINE_TX;
    uint32_t rx = (seed_level == SEC_GATEWAY) ? CAN_GATEWAY_RX : CAN_ENGINE_RX;

    if (is_zero_key(aes_key)) {
        emit LOG_E("No AES key set for level " + QString::number(seed_level) +
                   " - add it to rh850_di_keys.txt (gateway_l7 / engine_read_l5 / engine_flash_l1).", true, true);
        return STATUS_ERROR;
    }

    if (seed_level != SEC_GATEWAY) {
        QByteArray s; s.append((uint8_t)0x10); s.append((uint8_t)0x03);
        if (!positive(uds_request(tx, rx, s, serial_read_timeout), 0x10)) {
            emit LOG_E("Engine session control failed", true, true); return STATUS_ERROR;
        }
    }
    // request seed
    QByteArray sreq; sreq.append((uint8_t)0x27); sreq.append(seed_level);
    QByteArray srep = uds_request(tx, rx, sreq, serial_read_timeout);
    if (!positive(srep, 0x27) || srep.length() != 2 + 16) {
        emit LOG_E("Seed request (level " + QString::number(seed_level) + ") failed", true, true);
        return STATUS_ERROR;
    }
    QByteArray seed = srep.mid(2, 16);
    QByteArray answer = aes128_answer(aes_key, seed);
    // send key
    QByteArray kreq; kreq.append((uint8_t)0x27); kreq.append((uint8_t)(seed_level + 1)); kreq.append(answer);
    if (!positive(uds_request(tx, rx, kreq, serial_read_timeout), 0x27)) {
        emit LOG_E("Key rejected (level " + QString::number(seed_level + 1) + ") - wrong AES key?", true, true);
        return STATUS_ERROR;
    }
    emit LOG_I("Security level " + QString::number(seed_level) + " granted.", true, true);
    return STATUS_SUCCESS;
}

int FlashEcuSubaruRH850DICan::read_mem(uint32_t start_addr, uint32_t length)
{
    if (engine_security(SEC_READ, aes_read_key) != STATUS_SUCCESS) return STATUS_ERROR;

    QByteArray image;
    image.reserve(length);
    uint32_t addr = start_addr, end = start_addr + length;
    flashbytescount = length; flashbytesindex = 0;
    emit LOG_I(QString("Reading 0x%1..0x%2 via UDS 0x23").arg(start_addr,0,16).arg(end,0,16), true, true);

    while (addr < end) {
        if (kill_process) return STATUS_ERROR;
        uint32_t n = qMin(READ_CHUNK, end - addr);
        // 0x23, ALFID 0x14 (4-byte addr, 1-byte size)
        QByteArray req;
        req.append((uint8_t)0x23); req.append((uint8_t)0x14);
        req.append((uint8_t)((addr>>24)&0xFF)); req.append((uint8_t)((addr>>16)&0xFF));
        req.append((uint8_t)((addr>>8)&0xFF));   req.append((uint8_t)(addr&0xFF));
        req.append((uint8_t)(n & 0xFF));
        QByteArray rep = uds_request(CAN_ENGINE_TX, CAN_ENGINE_RX, req, serial_read_timeout);
        if (!positive(rep, 0x23) || (uint32_t)rep.length() < 1 + n) {
            emit LOG_E(QString("0x23 read failed at 0x%1").arg(addr,0,16), true, true);
            return STATUS_ERROR;
        }
        image.append(rep.mid(1, n));   // plaintext; no decrypt needed
        addr += n; flashbytesindex += n;
        if ((flashbytesindex & 0x3FFF) == 0) set_progressbar_value(100 * flashbytesindex / flashbytescount);
    }
    ecuCalDef->FullRomData = image;
    emit LOG_I("Read complete.", true, true);
    return STATUS_SUCCESS;
}

int FlashEcuSubaruRH850DICan::erase_mem(uint32_t start_addr, uint32_t length)
{
    // RoutineControl START 0xFF00, ALFID 0x44, addr(4), size(4)
    QByteArray r;
    r.append((uint8_t)0x31); r.append((uint8_t)0x01); r.append((uint8_t)0xFF); r.append((uint8_t)0x00);
    r.append((uint8_t)0x44);
    r.append((uint8_t)((start_addr>>24)&0xFF)); r.append((uint8_t)((start_addr>>16)&0xFF));
    r.append((uint8_t)((start_addr>>8)&0xFF));   r.append((uint8_t)(start_addr&0xFF));
    r.append((uint8_t)((length>>24)&0xFF)); r.append((uint8_t)((length>>16)&0xFF));
    r.append((uint8_t)((length>>8)&0xFF));   r.append((uint8_t)(length&0xFF));
    emit LOG_I("Erasing flash (RoutineControl 0xFF00)...", true, true);
    if (!positive(uds_request(CAN_ENGINE_TX, CAN_ENGINE_RX, r, serial_read_extra_long_timeout), 0x31)) {
        emit LOG_E("Erase failed", true, true); return STATUS_ERROR;
    }
    return STATUS_SUCCESS;
}

int FlashEcuSubaruRH850DICan::write_mem(bool test_write)
{
    if (ecuCalDef->FullRomData.length() != (int)(FLASH_END - FLASH_START)) {
        emit LOG_E("ROM size is not 0x3F0000; refusing to write.", true, true);
        return STATUS_ERROR;
    }
    if (engine_security(SEC_WRITE, aes_write_key) != STATUS_SUCCESS) return STATUS_ERROR;

    if (test_write) emit LOG_I("TEST WRITE: no erase / no transfer will be performed.", true, true);
    else if (erase_mem(FLASH_START, FLASH_END - FLASH_START) != STATUS_SUCCESS) return STATUS_ERROR;

    // RequestDownload: 0x34, dataFormat 0x04 (Feistel), ALFID 0x44, addr(4), size(4)
    uint32_t size = FLASH_END - FLASH_START;
    QByteArray d;
    d.append((uint8_t)0x34); d.append((uint8_t)0x04); d.append((uint8_t)0x44);
    d.append((uint8_t)((FLASH_START>>24)&0xFF)); d.append((uint8_t)((FLASH_START>>16)&0xFF));
    d.append((uint8_t)((FLASH_START>>8)&0xFF));   d.append((uint8_t)(FLASH_START&0xFF));
    d.append((uint8_t)((size>>24)&0xFF)); d.append((uint8_t)((size>>16)&0xFF));
    d.append((uint8_t)((size>>8)&0xFF));   d.append((uint8_t)(size&0xFF));
    QByteArray drep = uds_request(CAN_ENGINE_TX, CAN_ENGINE_RX, d, serial_read_timeout);
    if (!positive(drep, 0x34)) { emit LOG_E("RequestDownload failed", true, true); return STATUS_ERROR; }

    // TransferData 0x36 in <=WRITE_BLOCK plaintext chunks, Feistel-encrypted on the wire.
    const QByteArray &rom = ecuCalDef->FullRomData;
    uint32_t off = 0; uint8_t bsc = 1;
    flashbytescount = size; flashbytesindex = 0;
    while (off < size) {
        if (kill_process) return STATUS_ERROR;
        uint32_t n = qMin(WRITE_BLOCK, size - off);
        QByteArray plain = rom.mid(off, n);
        QByteArray enc = feistel_encrypt_block(plain);
        QByteArray t; t.append((uint8_t)0x36); t.append((uint8_t)bsc); t.append(enc);
        if (!test_write) {
            if (!positive(uds_request(CAN_ENGINE_TX, CAN_ENGINE_RX, t, serial_read_timeout), 0x36)) {
                emit LOG_E(QString("TransferData failed at 0x%1").arg(FLASH_START+off,0,16), true, true);
                return STATUS_ERROR;
            }
        }
        off += n; bsc++; flashbytesindex += n;
        if ((flashbytesindex & 0x3FFF) == 0) set_progressbar_value(100 * flashbytesindex / flashbytescount);
    }
    // TransferExit 0x37
    QByteArray ex; ex.append((uint8_t)0x37);
    if (!test_write && !positive(uds_request(CAN_ENGINE_TX, CAN_ENGINE_RX, ex, serial_read_timeout), 0x37)) {
        emit LOG_E("TransferExit failed", true, true); return STATUS_ERROR;
    }
    // Checksum verify RoutineControl 0xFF01 (same ALFID/addr/size)
    QByteArray c;
    c.append((uint8_t)0x31); c.append((uint8_t)0x01); c.append((uint8_t)0xFF); c.append((uint8_t)0x01);
    c.append((uint8_t)0x44);
    c.append((uint8_t)((FLASH_START>>24)&0xFF)); c.append((uint8_t)((FLASH_START>>16)&0xFF));
    c.append((uint8_t)((FLASH_START>>8)&0xFF));   c.append((uint8_t)(FLASH_START&0xFF));
    c.append((uint8_t)((size>>24)&0xFF)); c.append((uint8_t)((size>>16)&0xFF));
    c.append((uint8_t)((size>>8)&0xFF));   c.append((uint8_t)(size&0xFF));
    if (!test_write && !positive(uds_request(CAN_ENGINE_TX, CAN_ENGINE_RX, c, serial_read_timeout), 0x31)) {
        emit LOG_E("Post-write checksum verify failed", true, true); return STATUS_ERROR;
    }
    emit LOG_I(test_write ? "Test write complete (nothing written)." : "Write complete.", true, true);
    return STATUS_SUCCESS;
}

// ---- helpers ----

bool FlashEcuSubaruRH850DICan::is_zero_key(const uint8_t *k)
{
    for (int i = 0; i < 16; i++) if (k[i]) return false;
    return true;
}

bool FlashEcuSubaruRH850DICan::load_keys()
{
    auto hex_to = [](const QString &h, uint8_t *dst, int n) -> bool {
        QByteArray b = QByteArray::fromHex(h.trimmed().toLatin1());
        if (b.length() != n) return false;
        for (int i = 0; i < n; i++) dst[i] = (uint8_t)b.at(i);
        return true;
    };
    QStringList paths = { QCoreApplication::applicationDirPath() + "/rh850_di_keys.txt",
                          QDir::currentPath() + "/rh850_di_keys.txt" };
    QString found;
    for (const QString &p : paths) if (QFile::exists(p)) { found = p; break; }
    if (found.isEmpty()) {
        emit LOG_W("rh850_di_keys.txt not found - AES keys unset. Read/write will stop at security.", true, true);
        return false;
    }
    QFile f(found);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    int loaded = 0;
    while (!f.atEnd()) {
        QString line = QString::fromLatin1(f.readLine()).trimmed();
        if (line.startsWith('#') || !line.contains(':')) continue;
        QString tag = line.section(':', 0, 0).trimmed().toLower();
        QString val = line.section(':', 1).trimmed();
        if      (tag == "gateway_l7"      && hex_to(val, aes_gateway_key, 16)) loaded++;
        else if (tag == "engine_read_l5"  && hex_to(val, aes_read_key, 16))    loaded++;
        else if (tag == "engine_flash_l1" && hex_to(val, aes_write_key, 16))   loaded++;
        else if (tag == "feistel") {
            uint8_t fk[8];
            if (hex_to(val, fk, 8)) {
                for (int i = 0; i < 4; i++) {
                    feistel_enc_key[i]     = (fk[2*i] << 8) | fk[2*i+1];
                    feistel_dec_key[3 - i] = feistel_enc_key[i];
                }
                loaded++;
            }
        }
    }
    emit LOG_I(QString("Loaded %1 key(s) from %2").arg(loaded).arg(found), true, true);
    return loaded > 0;
}

QByteArray FlashEcuSubaruRH850DICan::uds_request(uint32_t tx_id, uint32_t rx_id, QByteArray payload, uint16_t timeout, bool expect_reply)
{
    serial->set_iso15765_source_address(tx_id);
    serial->set_iso15765_destination_address(rx_id);
    QByteArray output;
    output.append((uint8_t)((tx_id>>24)&0xFF)); output.append((uint8_t)((tx_id>>16)&0xFF));
    output.append((uint8_t)((tx_id>>8)&0xFF));   output.append((uint8_t)(tx_id&0xFF));
    output.append(payload);
    serial->write_serial_data_echo_check(output);
    if (!expect_reply) return QByteArray();
    delay(1);
    QByteArray received = serial->read_serial_data(timeout);
    if (received.length() > 4) return received.mid(4);   // strip the 4-byte arbitration id
    return QByteArray();
}

bool FlashEcuSubaruRH850DICan::positive(QByteArray reply, uint8_t req_sid)
{
    if (reply.length() < 1) return false;
    if ((uint8_t)reply.at(0) == (uint8_t)(req_sid + 0x40)) return true;
    if ((uint8_t)reply.at(0) == 0x7F)
        emit LOG_E("Negative response: " + FileActions::parse_nrc_message(reply), true, true);
    return false;
}

QByteArray FlashEcuSubaruRH850DICan::aes128_answer(const uint8_t key[16], QByteArray seed16)
{
    uint8_t in[16], out[16];
    for (int i=0;i<16;i++) in[i] = (i < seed16.length()) ? (uint8_t)seed16.at(i) : 0;
    aes128_encrypt_block(key, in, out);
    QByteArray a; for (int i=0;i<16;i++) a.append((char)out[i]); return a;
}

QByteArray FlashEcuSubaruRH850DICan::feistel_encrypt_block(QByteArray plain)
{
    QByteArray out; out.reserve(plain.length());
    for (int i=0;i+4<=plain.length(); i+=4) {
        uint32_t sym = ((uint8_t)plain.at(i)<<24)|((uint8_t)plain.at(i+1)<<16)|((uint8_t)plain.at(i+2)<<8)|(uint8_t)plain.at(i+3);
        uint32_t c = feistel_enc_sym(sym, feistel_enc_key);
        out.append((char)((c>>24)&0xFF)); out.append((char)((c>>16)&0xFF));
        out.append((char)((c>>8)&0xFF));   out.append((char)(c&0xFF));
    }
    return out;
}

QByteArray FlashEcuSubaruRH850DICan::feistel_decrypt_block(QByteArray cipher)
{
    QByteArray out; out.reserve(cipher.length());
    for (int i=0;i+4<=cipher.length(); i+=4) {
        uint32_t sym = ((uint8_t)cipher.at(i)<<24)|((uint8_t)cipher.at(i+1)<<16)|((uint8_t)cipher.at(i+2)<<8)|(uint8_t)cipher.at(i+3);
        uint32_t c = feistel_dec_sym(sym, feistel_dec_key);
        out.append((char)((c>>8)&0xFF)); out.append((char)(c&0xFF));
        out.append((char)((c>>24)&0xFF)); out.append((char)((c>>16)&0xFF));
    }
    return out;
}

QString FlashEcuSubaruRH850DICan::parse_message_to_hex(QByteArray received)
{
    QString msg;
    for (int i=0;i<received.length();i++) msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')));
    return msg;
}

void FlashEcuSubaruRH850DICan::set_progressbar_value(int value) { emit external_logger(value); }

void FlashEcuSubaruRH850DICan::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void FlashEcuSubaruRH850DICan::closeEvent(QCloseEvent *event) { kill_process = true; event->accept(); }
