#ifndef CHECKSUM_TCU_SUBARU_HITACHI_M32R_KLINE_H
#define CHECKSUM_TCU_SUBARU_HITACHI_M32R_KLINE_H

#include <QByteArray>
#include <QDebug>
#include <QMessageBox>
#include <QString>

/*
 * Checksums of the 2005-06 Hitachi/JECS M32R 5EAT TCU generation whose
 * code/calibration region ends at 0x5FFFF (e.g. 91FE216300). Mirrors the
 * TCU firmware's own checks:
 *   1) flash kernel verify (FUN_000050AC): low 16 bits of the u32 sum over
 *      0x8020..0x5FFFC (0x8000-0x801F excluded) must be 0x5AA5; balanced by
 *      the 16-bit word at 0x8022
 *   2) startup check (FUN_00020100): u32 sum over 0x0..0x5FFFC (stored words
 *      included) must equal the u32 words at 0x8000 and 0x8004
 *   3) reset gate (FUN_00000178): 55 AA CC 33 at 0xFFFC + n * 0x10000, n = 0..5
 * The full-512 KB generation (32-bit whole-file balance) is NOT handled here,
 * see checksum_tcu_subaru_hitachi_m32r_can.
 */
class ChecksumTcuSubaruHitachiM32rKline
{
public:
    ChecksumTcuSubaruHitachiM32rKline();
    ~ChecksumTcuSubaruHitachiM32rKline();

    static QByteArray calculate_checksum(QByteArray romData);
    static bool detect_layout(const QByteArray &romData, QString *reason = nullptr);
    static bool verify_checksum(const QByteArray &romData, QString *report = nullptr);

    static const int region_end = 0x60000;

private:
    static uint32_t read_u32(const QByteArray &data, int addr);
    static void write_u32(QByteArray &data, int addr, uint32_t value);
    static uint32_t kernel_balance_sum(const QByteArray &romData);
    static uint32_t additive_sum(const QByteArray &romData, bool skip_stored);

};

#endif // CHECKSUM_TCU_SUBARU_HITACHI_M32R_KLINE_H
