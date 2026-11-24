#include "checksum_tcu_subaru_hitachi_m32r_kline.h"

ChecksumTcuSubaruHitachiM32rKline::ChecksumTcuSubaruHitachiM32rKline()
{

}

ChecksumTcuSubaruHitachiM32rKline::~ChecksumTcuSubaruHitachiM32rKline()
{

}

uint32_t ChecksumTcuSubaruHitachiM32rKline::read_u32(const QByteArray &data, int addr)
{
    return ((uint32_t)(uint8_t)data.at(addr) << 24) | ((uint32_t)(uint8_t)data.at(addr + 1) << 16) |
           ((uint32_t)(uint8_t)data.at(addr + 2) << 8) | (uint32_t)(uint8_t)data.at(addr + 3);
}

void ChecksumTcuSubaruHitachiM32rKline::write_u32(QByteArray &data, int addr, uint32_t value)
{
    data[addr] = (char)((value >> 24) & 0xFF);
    data[addr + 1] = (char)((value >> 16) & 0xFF);
    data[addr + 2] = (char)((value >> 8) & 0xFF);
    data[addr + 3] = (char)(value & 0xFF);
}

/*
 * u32 sum 0x8020..0x5FFFC (flash kernel verify, 0x8000-0x801F not included)
 */
uint32_t ChecksumTcuSubaruHitachiM32rKline::kernel_balance_sum(const QByteArray &romData)
{
    uint32_t sum = 0;

    for (int i = 0x8020; i < region_end; i += 4)
        sum += read_u32(romData, i);

    return sum;
}

/*
 * u32 sum 0x0..0x5FFFC, optionally without the two stored words at 0x8000/0x8004
 */
uint32_t ChecksumTcuSubaruHitachiM32rKline::additive_sum(const QByteArray &romData, bool skip_stored)
{
    uint32_t sum = 0;

    for (int i = 0; i < region_end; i += 4)
    {
        if (skip_stored && (i == 0x8000 || i == 0x8004))
            continue;
        sum += read_u32(romData, i);
    }

    return sum;
}

/*
 * Check that the image is the 0x5FFFF generation this module knows
 *
 * @return true if layout matches
 */
bool ChecksumTcuSubaruHitachiM32rKline::detect_layout(const QByteArray &romData, QString *reason)
{
    QString why;

    if (romData.length() < region_end)
        why = QString("ROM size 0x%1 is smaller than 0x60000").arg(romData.length(), 0, 16);

    for (int i = region_end; why.isEmpty() && i < romData.length(); i++)
    {
        if ((uint8_t)romData.at(i) != 0xFF)
            why = QString("Byte at 0x%1 is not 0xFF: this is not a 0x5FFFF-generation TCU image").arg(i, 5, 16, QLatin1Char('0'));
    }

    for (int n = 0; why.isEmpty() && n < 6; n++)
    {
        int addr = 0xFFFC + n * 0x10000;
        if (read_u32(romData, addr) != 0x55AACC33)
            why = QString("Block marker 55 AA CC 33 missing at 0x%1").arg(addr, 5, 16, QLatin1Char('0'));
    }

    if (reason)
        *reason = why;

    return why.isEmpty();
}

/*
 * Verify both firmware checksums (no changes made)
 *
 * @return true if both checks pass
 */
bool ChecksumTcuSubaruHitachiM32rKline::verify_checksum(const QByteArray &romData, QString *report)
{
    QString why;

    if (!detect_layout(romData, &why))
    {
        if (report)
            *report = why;
        return false;
    }

    uint32_t balance_sum = kernel_balance_sum(romData);
    uint32_t total_sum = additive_sum(romData, false);
    uint32_t stored_1 = read_u32(romData, 0x8000);
    uint32_t stored_2 = read_u32(romData, 0x8004);

    bool balance_ok = (balance_sum & 0xFFFF) == 0x5AA5;
    bool additive_ok = (stored_1 == total_sum && stored_2 == total_sum);

    if (report)
        *report = QString("Kernel balance low16 0x%1 (%2), additive 0x%3 stored 0x%4 / 0x%5 (%6)")
                      .arg(balance_sum & 0xFFFF, 4, 16, QLatin1Char('0'))
                      .arg(balance_ok ? "OK" : "FAIL")
                      .arg(total_sum, 8, 16, QLatin1Char('0'))
                      .arg(stored_1, 8, 16, QLatin1Char('0'))
                      .arg(stored_2, 8, 16, QLatin1Char('0'))
                      .arg(additive_ok ? "OK" : "FAIL");

    return balance_ok && additive_ok;
}

/*
 * Correct both checksums of a 0x5FFFF-generation TCU image. Unknown layouts are
 * returned unchanged.
 *
 * Order matters: the balance word at 0x8022 is part of the additive region, so
 * it is fixed first; the stored additive words at 0x8000/0x8004 are outside the
 * balance region.
 *
 * @return corrected ROM data
 */
QByteArray ChecksumTcuSubaruHitachiM32rKline::calculate_checksum(QByteArray romData)
{
    QString msg;

    if (!detect_layout(romData, &msg))
    {
        qDebug() << "Subaru Hitachi M32R K-Line TCU checksum NOT corrected:" << msg;
        QMessageBox::warning(nullptr, "TCU checksum", "Checksums NOT corrected!\n\n" + msg +
                             "\n\nThis checksum module is for 0x5FFFF-generation Hitachi K-Line TCUs only.");
        return romData;
    }

    QByteArray original = romData;

    // 1) kernel balance word (low half of the u32 at 0x8020)
    romData[0x8022] = 0;
    romData[0x8023] = 0;
    uint16_t balance = (uint16_t)(0x5AA5 - (kernel_balance_sum(romData) & 0xFFFF));
    romData[0x8022] = (char)((balance >> 8) & 0xFF);
    romData[0x8023] = (char)(balance & 0xFF);

    // 2) additive words: sum(all) must equal the stored value C, stored twice
    //    => C = -(sum of all other words)
    uint32_t stored = (uint32_t)(0 - additive_sum(romData, true));
    write_u32(romData, 0x8000, stored);
    write_u32(romData, 0x8004, stored);

    if (!verify_checksum(romData, &msg))
    {
        qDebug() << "Subaru Hitachi M32R K-Line TCU checksum correction failed:" << msg;
        QMessageBox::warning(nullptr, "TCU checksum", "Checksum correction failed, ROM left unchanged!\n\n" + msg);
        return original;
    }

    if (romData != original)
        qDebug() << "Subaru Hitachi M32R K-Line TCU checksums corrected:" << msg;
    else
        qDebug() << "Subaru Hitachi M32R K-Line TCU checksums OK:" << msg;

    return romData;
}
