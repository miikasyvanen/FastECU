#include "checksum_ecu_subaru_hitachi_m32r.h"

ChecksumEcuSubaruHitachiM32r::ChecksumEcuSubaruHitachiM32r()
{

}

ChecksumEcuSubaruHitachiM32r::~ChecksumEcuSubaruHitachiM32r()
{

}

QByteArray ChecksumEcuSubaruHitachiM32r::calculate_checksum(QByteArray romData)
{
    QString msg;

    uint16_t checksum_1 = 0;
    uint16_t checksum_2 = 0;
    uint8_t checksum_2_hi = 0;
    uint8_t checksum_2_lo = 0;
    for (int i = 0x4000; i < romData.length(); i += 4)
    {
        if (i < 0x10000 || i > 0x10003)
            checksum_1 += (romData.at(i) << 24) + (romData.at(i + 1) << 16) + (romData.at(i + 2) << 8) + romData.at(i + 3);
    }
    for (int i = 0; i < romData.length(); i += 1)
    {
        if (i < 0x10000 || i > 0x10001)
        {
            checksum_2_hi += romData.at(i);
            checksum_2_lo ^= romData.at(i);
        }
    }
    msg.clear();
    msg.append(QString("Checksum 1 before: 0x%1").arg(checksum_1,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    checksum_2 = (checksum_2_hi << 8) + checksum_2_lo;
    msg.clear();
    msg.append(QString("Checksum 2 before: 0x%1").arg(checksum_2,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;


    return romData;
}
