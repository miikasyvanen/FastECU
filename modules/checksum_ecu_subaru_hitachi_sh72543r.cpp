#include "checksum_ecu_subaru_hitachi_sh72543r.h"

ChecksumEcuSubaruHitachiSh72543r::ChecksumEcuSubaruHitachiSh72543r()
{

}

ChecksumEcuSubaruHitachiSh72543r::~ChecksumEcuSubaruHitachiSh72543r()
{

}

QByteArray ChecksumEcuSubaruHitachiSh72543r::calculate_checksum(QByteArray romData)
{
    /*******************
     *
     * Checksum_6010 is calculated between 0x6000 - 0x1fffff, 16bit summation, balance value at 0x6c and 0x6010
     * PTR_DAT_000b446c
     *
     ******************/

    QByteArray msg;
    uint16_t chksum_6010 = 0;

    for (int i = 0x6000; i < 0x200000; i+=2)
    {
        chksum_6010 += ((uint8_t)romData.at(i) << 8) + (uint8_t)romData.at(i + 1);
    }
    msg.clear();
    msg.append(QString("CHKSUM: 0x%1").arg(chksum_6010,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    if (chksum_6010 != 0x5aa5)
    {
        qDebug() << "Checksum mismatch!";

        QByteArray balance_value_array;
        uint32_t balance_value_1_array_start = 0x6010;
        uint32_t balance_value_2_array_start = 0x6C;
        uint16_t balance_value = ((uint8_t)romData.at(0x6010) << 8) + ((uint8_t)romData.at(0x6011));

        msg.clear();
        msg.append(QString("Balance value before: 0x%1").arg(balance_value,4,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        balance_value += 0x5aa5 - chksum_6010;

        msg.clear();
        msg.append(QString("Balance value after: 0x%1").arg(balance_value,4,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        balance_value_array.append((uint8_t)((balance_value >> 8) & 0xff));
        balance_value_array.append((uint8_t)(balance_value & 0xff));
        romData.replace(balance_value_1_array_start, balance_value_array.length(), balance_value_array);
        romData.replace(balance_value_2_array_start, balance_value_array.length(), balance_value_array);

        qDebug() << "Checksums corrected";
        QMessageBox::information(this, tr("Subaru Hitachi SH72543r ECU Checksum"), "Checksums corrected");
    }

    return romData;
}
