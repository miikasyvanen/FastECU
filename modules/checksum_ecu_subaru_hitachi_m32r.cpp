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

    uint16_t checksum_1_value_calculated = 0;
    uint32_t checksum_1_balance_value_stored = 0;
    uint16_t checksum_2_value_calculated = 0;
    uint8_t checksum_2_hi_value_calculated = 0;
    uint8_t checksum_2_lo_value_calculated = 0;
    uint32_t checksum_2_value_stored = 0;

    for (int i = 0x4000; i < romData.length(); i += 4)
    {
        if (i < 0x10000 || i > 0x10003)
            checksum_1_value_calculated += (romData.at(i) << 24) + (romData.at(i + 1) << 16) + (romData.at(i + 2) << 8) + romData.at(i + 3);
    }
    for (int i = 0; i < romData.length(); i += 1)
    {
        if (i < 0x10000 || i > 0x10001)
        {
            checksum_2_hi_value_calculated += romData.at(i);
            checksum_2_lo_value_calculated ^= romData.at(i);
        }
    }
    checksum_2_value_calculated = (checksum_2_hi_value_calculated << 8) + checksum_2_lo_value_calculated;

    checksum_1_balance_value_stored = (romData.at(0x07fffa) << 8) + romData.at(0x07fffb);
    msg.clear();
    msg.append(QString("Checksum 1 balance value stored 0x0x7fffa: 0x%1").arg(checksum_1_balance_value_stored,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    msg.clear();
    msg.append(QString("Checksum 1 calculated: 0x%1").arg(checksum_1_value_calculated,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    checksum_2_value_stored = (romData.at(0x10000) << 8) + romData.at(0x10001);
    msg.clear();
    msg.append(QString("Checksum 2 stored 0x10000: 0x%1").arg(checksum_2_value_stored,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    msg.clear();
    msg.append(QString("Checksum 2 calculated 0x10000: 0x%1").arg(checksum_2_value_calculated,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    if (checksum_1_value_calculated != 0x5aa5)
    {
        qDebug() << "Checksum 1 mismatch!";

        QByteArray checksum_value_array;
        uint32_t checksum_value_address = 0x07fffa;

        msg.clear();
        msg.append(QString("Checksum 1 balance value before: 0x%1").arg(checksum_1_balance_value_stored,8,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        checksum_1_balance_value_stored += 0x5aa5 - checksum_1_value_calculated;

        checksum_value_array.append((uint8_t)((checksum_1_balance_value_stored >> 8) & 0xff));
        checksum_value_array.append((uint8_t)(checksum_1_balance_value_stored & 0xff));

        romData.replace(checksum_value_address, checksum_value_array.length(), checksum_value_array);

        msg.clear();
        msg.append(QString("Checksum 1 balance value after: 0x%1").arg(checksum_1_balance_value_stored,8,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;
    }

    if (checksum_2_value_calculated != checksum_2_value_stored)
    {
        qDebug() << "Checksum 2 mismatch!";

        QByteArray balance_value_array;
        uint32_t balance_value_array_start = 0x10000;

        msg.clear();
        msg.append(QString("Checksum 2 value before: 0x%1").arg(checksum_2_value_calculated,8,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        checksum_2_value_calculated = 0;
        for (int i = 0; i < romData.length(); i += 1)
        {
            if (i < 0x10000 || i > 0x10001)
            {
                checksum_2_hi_value_calculated += romData.at(i);
                checksum_2_lo_value_calculated ^= romData.at(i);
            }
        }
        checksum_2_value_calculated = (checksum_2_hi_value_calculated << 8) + checksum_2_lo_value_calculated;

        uint8_t checksum_2_value_calculated_bytes[2];
        checksum_2_value_calculated_bytes[1] = 0xff - ((checksum_2_value_calculated >> 8) & 0xff);
        checksum_2_value_calculated_bytes[0] = 0x100 - (checksum_2_value_calculated & 0xff);
        checksum_2_value_calculated = (checksum_2_value_calculated_bytes[1] << 8) + checksum_2_value_calculated_bytes[0];

        for (int i = 0; i < 2; i++)
        {
            balance_value_array.append((uint8_t)((checksum_2_value_calculated >> 8) & 0xff));
            balance_value_array.append((uint8_t)(checksum_2_value_calculated & 0xff));
        }
        romData.replace(balance_value_array_start, balance_value_array.length(), balance_value_array);

        msg.clear();
        msg.append(QString("Checksum 2 value after: 0x%1").arg(checksum_2_value_calculated,8,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;
    }

    return romData;
}
