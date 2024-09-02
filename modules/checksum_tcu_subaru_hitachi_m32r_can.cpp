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
    uint32_t checksum_1_value_calculated = 0;
    uint32_t checksum_1_balance_value_stored = 0;
    uint32_t checksum_2_value_calculated = 0;
    uint32_t checksum_2_value_stored = 0;

    bool checksum_ok = true;

    for (int i = 0x0; i < romData.length(); i += 4)
    {
        if (i >= 0x8020)
            checksum_1_value_calculated += (romData.at(i) << 24) + (romData.at(i + 1) << 16) + (romData.at(i + 2) << 8) + romData.at(i + 3);
        if (i < 0x8000 || i > 0x8007)
            checksum_2_value_calculated += (romData.at(i) << 24) + (romData.at(i + 1) << 16) + (romData.at(i + 2) << 8) + romData.at(i + 3);
    }

    uint8_t checksum_2_value_calculated_bytes[4];
    checksum_2_value_calculated_bytes[3] = 0xff - ((checksum_2_value_calculated >> 24) & 0xff);
    checksum_2_value_calculated_bytes[2] = 0xff - ((checksum_2_value_calculated >> 16) & 0xff);
    checksum_2_value_calculated_bytes[1] = 0xff - ((checksum_2_value_calculated >> 8) & 0xff);
    checksum_2_value_calculated_bytes[0] = 0x100 - (checksum_2_value_calculated & 0xff);
    checksum_2_value_calculated = (checksum_2_value_calculated_bytes[3] << 24) + (checksum_2_value_calculated_bytes[2] << 16) + (checksum_2_value_calculated_bytes[1] << 8) + checksum_2_value_calculated_bytes[0];

    checksum_1_balance_value_stored = (romData.at(0x8020) << 24) + (romData.at(0x8021) << 16) + (romData.at(0x8022) << 8) + romData.at(0x8023);
    msg.clear();
    msg.append(QString("Checksum 1 balance value stored 0x8020: 0x%1").arg(checksum_1_balance_value_stored,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    msg.clear();
    msg.append(QString("Checksum 1 calculated 0x8020: 0x%1").arg(checksum_1_value_calculated,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    checksum_2_value_stored = (romData.at(0x8000) << 24) + (romData.at(0x8001) << 16) + (romData.at(0x8002) << 8) + romData.at(0x8003);
    msg.clear();
    msg.append(QString("Checksum 2 stored 0x8000/0x8004: 0x%1").arg(checksum_2_value_stored,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    msg.clear();
    msg.append(QString("Checksum 2 calculated 0x8000/0x8004: 0x%1").arg(checksum_2_value_calculated,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    if (checksum_1_value_calculated != 0x5aa5a55a)
    {
        qDebug() << "Checksum 1 mismatch!";
        checksum_ok = false;

        QByteArray checksum_value_array;
        uint32_t checksum_value_address = 0x8020;

        msg.clear();
        msg.append(QString("Checksum 1 balance value before: 0x%1").arg(checksum_1_balance_value_stored,8,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        checksum_1_balance_value_stored += 0x5aa5a55a - checksum_1_value_calculated;

        checksum_value_array.append((uint8_t)((checksum_1_balance_value_stored >> 24) & 0xff));
        checksum_value_array.append((uint8_t)((checksum_1_balance_value_stored >> 16) & 0xff));
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
        checksum_ok = false;

        QByteArray balance_value_array;
        uint32_t balance_value_array_start = 0x8000;

        msg.clear();
        msg.append(QString("Checksum 2 value before: 0x%1").arg(checksum_2_value_calculated,8,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        checksum_2_value_calculated = 0;
        for (int i = 0x0; i < romData.length(); i += 4)
        {
            if (i < 0x8000 || i > 0x8007)
                checksum_2_value_calculated += (romData.at(i) << 24) + (romData.at(i + 1) << 16) + (romData.at(i + 2) << 8) + romData.at(i + 3);
        }
        uint8_t checksum_2_value_calculated_bytes[4];
        checksum_2_value_calculated_bytes[3] = 0xff - ((checksum_2_value_calculated >> 24) & 0xff);
        checksum_2_value_calculated_bytes[2] = 0xff - ((checksum_2_value_calculated >> 16) & 0xff);
        checksum_2_value_calculated_bytes[1] = 0xff - ((checksum_2_value_calculated >> 8) & 0xff);
        checksum_2_value_calculated_bytes[0] = 0x100 - (checksum_2_value_calculated & 0xff);
        checksum_2_value_calculated = (checksum_2_value_calculated_bytes[3] << 24) + (checksum_2_value_calculated_bytes[2] << 16) + (checksum_2_value_calculated_bytes[1] << 8) + checksum_2_value_calculated_bytes[0];

        for (int i = 0; i < 2; i++)
        {
            balance_value_array.append((uint8_t)((checksum_2_value_calculated >> 24) & 0xff));
            balance_value_array.append((uint8_t)((checksum_2_value_calculated >> 16) & 0xff));
            balance_value_array.append((uint8_t)((checksum_2_value_calculated >> 8) & 0xff));
            balance_value_array.append((uint8_t)(checksum_2_value_calculated & 0xff));
        }
        romData.replace(balance_value_array_start, balance_value_array.length(), balance_value_array);

        msg.clear();
        msg.append(QString("Checksum 2 value after: 0x%1").arg(checksum_2_value_calculated,8,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;
    }
    if (!checksum_ok)
    {
        QMessageBox::information(this, tr("Subaru Hitachi M32R K-Line/CAN ECU Checksum"), "Checksums corrected");
    }

    return romData;
}
