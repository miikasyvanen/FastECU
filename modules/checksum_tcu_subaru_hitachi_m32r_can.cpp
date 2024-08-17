#include "checksum_tcu_subaru_hitachi_m32r_can.h"

ChecksumTcuSubaruHitachiM32rCan::ChecksumTcuSubaruHitachiM32rCan()
{

}

ChecksumTcuSubaruHitachiM32rCan::~ChecksumTcuSubaruHitachiM32rCan()
{

}

QByteArray ChecksumTcuSubaruHitachiM32rCan::calculate_checksum(QByteArray romData)
{
    QByteArray msg;
    uint32_t checksum = 0;
    uint32_t checksum1 = 0;
    uint32_t checksum2 = 0;
    for (int i = 0x0; i < romData.length(); i += 4)
    {
        if (i >= 0x8020)
            checksum1 += (romData.at(i) << 24) + (romData.at(i + 1) << 16) + (romData.at(i + 2) << 8) + romData.at(i + 3);
        if (i < 0x8000 || i > 0x8007)
            checksum2 += (romData.at(i) << 24) + (romData.at(i + 1) << 16) + (romData.at(i + 2) << 8) + romData.at(i + 3);
    }

    checksum = (romData.at(0x8020) << 24) + (romData.at(0x8021) << 16) + (romData.at(0x8022) << 8) + romData.at(0x8023);
    msg.clear();
    msg.append(QString("Checksum 1 balance value ROM 0x8020: 0x%1").arg(checksum,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    checksum = checksum1;
    msg.clear();
    msg.append(QString("Checksum 1 calculated 0x8020: 0x%1").arg(checksum,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    checksum = (romData.at(0x8000) << 24) + (romData.at(0x8001) << 16) + (romData.at(0x8002) << 8) + romData.at(0x8003);
    msg.clear();
    msg.append(QString("Checksum 2 ROM 0x8000/0x8004: 0x%1").arg(checksum,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    uint8_t checksum_2[4];
    checksum_2[3] = 0xff - ((checksum2 >> 24) & 0xff);
    checksum_2[2] = 0xff - ((checksum2 >> 16) & 0xff);
    checksum_2[1] = 0xff - ((checksum2 >> 8) & 0xff);
    checksum_2[0] = 0x100 - (checksum2 & 0xff);
    checksum = (checksum_2[3] << 24) + (checksum_2[2] << 16) + (checksum_2[1] << 8) + checksum_2[0];
    msg.clear();
    msg.append(QString("Checksum 2 calculated 0x8000/0x8004: 0x%1").arg(checksum,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    return romData;
}
