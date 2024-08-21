#include "checksum_ecu_subaru_hitachi_m32r.h"

ChecksumEcuSubaruHitachiM32r::ChecksumEcuSubaruHitachiM32r()
{

}

ChecksumEcuSubaruHitachiM32r::~ChecksumEcuSubaruHitachiM32r()
{

}

QByteArray ChecksumEcuSubaruHitachiM32r::calculate_checksum(QByteArray romData)
{
    /*******************
     *
     *  Checksum 1 calculated between 0x6000 - 0x8000 excluding 0x10000 - 0x10003, 32bit sum, result at 0x7ffe8
     *  Checksum 2 calculated between 0x6000 - 0x8000 excluding 0x10000 - 0x10003, 32 bit XOR, result at 0x7ffec
     *  Checksum 3 calculated between 0x0000 - 0x7ffef excluding 0x10000 - 0x10003, 32bit sum, result at 0x7fff0
     *  Checksum 4 calculated between 0x0000 - 0x7ffef excluding 0x10000 - 0x10003, 32 bit XOR, result at 0x7fff4
     *  Checksum 5 calculated between 0x4000 - 0x7ffff excluding 0x10000 - 0x10003, 16bit sum, must mach 0x5aa5, balancing address 0x7fffa
     *  Checksum 6 calculated between 0x0000 - 0x7ffff excluding 0x10000 - 0x10003, 8bit sum & XOR, result 16bit, sum hi byte, XOR low byte
     *
     * ****************/
    QString msg;

    uint32_t checksum_1_value_stored = 0;
    uint32_t checksum_1_value_calculated = 0;
    uint32_t checksum_1_value_address = 0x7ffe8;
    uint32_t checksum_2_value_stored = 0;
    uint32_t checksum_2_value_calculated = 0;
    uint32_t checksum_2_value_address = 0x7ffec;
    uint32_t checksum_3_value_stored = 0;
    uint32_t checksum_3_value_calculated = 0;
    uint32_t checksum_3_value_address = 0x7fff0;
    uint32_t checksum_4_value_stored = 0;
    uint32_t checksum_4_value_calculated = 0;
    uint32_t checksum_4_value_address = 0x7fff4;

    uint16_t checksum_5_value_calculated = 0;
    uint16_t checksum_5_balance_value_address = 0x7fffa;

    uint16_t checksum_6_balance_value_stored = 0;
    uint16_t checksum_6_balance_value_calculated = 0;
    uint32_t checksum_6_balance_value_address = 0x10000;
    uint8_t checksum_6_balance_value_hi_calculated = 0;
    uint8_t checksum_6_balance_value_lo_calculated = 0;

    for (int i = 0x6000; i < 0x8000; i += 4)
    {
        checksum_1_value_calculated += (romData.at(i) << 24) + (romData.at(i + 1) << 16) + (romData.at(i + 2) << 8) + romData.at(i + 3);
        checksum_2_value_calculated ^= (romData.at(i) << 24) + (romData.at(i + 1) << 16) + (romData.at(i + 2) << 8) + romData.at(i + 3);
    }
    for (int i = 0; i < romData.length(); i += 4)
    {
        if (i < 0x10000 || (i > 0x10003 && i < 0x7fff0))
        {
            checksum_3_value_calculated += (romData.at(i) << 24) + (romData.at(i + 1) << 16) + (romData.at(i + 2) << 8) + romData.at(i + 3);
            checksum_4_value_calculated ^= (romData.at(i) << 24) + (romData.at(i + 1) << 16) + (romData.at(i + 2) << 8) + romData.at(i + 3);
        }
    }
    for (int i = 0x4000; i < romData.length(); i += 4)
    {
        if (i < 0x10000 || i > 0x10003)
            checksum_5_value_calculated += (romData.at(i) << 24) + (romData.at(i + 1) << 16) + (romData.at(i + 2) << 8) + romData.at(i + 3);
    }
    for (int i = 0; i < romData.length(); i += 1)
    {
        if (i < 0x10000 || i > 0x10003)
        {
            checksum_6_balance_value_hi_calculated += romData.at(i);
            checksum_6_balance_value_lo_calculated ^= romData.at(i);
        }
    }
    checksum_6_balance_value_calculated = (checksum_6_balance_value_hi_calculated << 8) + checksum_6_balance_value_lo_calculated;

    checksum_1_value_stored = (romData.at(0x07ffe8) << 24) + (romData.at(0x07ffe9) << 16) + (romData.at(0x07ffea) << 8) + romData.at(0x07ffeb);
    checksum_2_value_stored = (romData.at(0x07ffec) << 24) + (romData.at(0x07ffed) << 16) + (romData.at(0x07ffee) << 8) + romData.at(0x07ffef);
    checksum_3_value_stored = (romData.at(0x07fff0) << 24) + (romData.at(0x07fff1) << 16) + (romData.at(0x07fff2) << 8) + romData.at(0x07fff3);
    checksum_4_value_stored = (romData.at(0x07fff4) << 24) + (romData.at(0x07fff5) << 16) + (romData.at(0x07fff6) << 8) + romData.at(0x07fff7);

    checksum_6_balance_value_stored = (romData.at(checksum_6_balance_value_address) << 8) + romData.at(checksum_6_balance_value_address + 1);

    msg.clear();
    msg.append(QString("Checksum 1 value calculated 0x7ffe8: 0x%1").arg(checksum_1_value_calculated,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("Checksum 1 value stored 0x7ffe8: 0x%1").arg(checksum_1_value_stored,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    msg.clear();
    msg.append(QString("Checksum 2 value calculated 0x7ffec: 0x%1").arg(checksum_2_value_calculated,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("Checksum 2 value stored 0x7ffec: 0x%1").arg(checksum_2_value_stored,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    msg.clear();
    msg.append(QString("Checksum 3 value calculated 0x7fff0: 0x%1").arg(checksum_3_value_calculated,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("Checksum 3 value stored 0x7fff0: 0x%1").arg(checksum_3_value_stored,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    msg.clear();
    msg.append(QString("Checksum 4 value calculated 0x7fff4: 0x%1").arg(checksum_4_value_calculated,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("Checksum 4 value stored 0x7fff4: 0x%1").arg(checksum_4_value_stored,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    msg.clear();
    msg.append(QString("Checksum 5 calculated: 0x%1").arg(checksum_5_value_calculated,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    msg.clear();
    msg.append(QString("Checksum 6 value calculated 0x10000: 0x%1").arg(checksum_6_balance_value_calculated,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("Checksum 6 value stored 0x10000: 0x%1").arg(checksum_6_balance_value_stored,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    if (checksum_1_value_calculated != checksum_1_value_stored)
    {
        qDebug() << "Checksum 1 value mismatch!";
        QByteArray checksum;
        checksum.append((uint8_t)(checksum_1_value_calculated >> 24));
        checksum.append((uint8_t)(checksum_1_value_calculated >> 16));
        checksum.append((uint8_t)(checksum_1_value_calculated >> 8));
        checksum.append((uint8_t)checksum_1_value_calculated);
        romData.replace(checksum_1_value_address, checksum.length(), checksum);

        qDebug() << "Subaru Hitachi M32R K-Line/CAN ECU checksum 1 corrected";
        //QMessageBox::information(this, tr("Subaru Hitachi M32R K-Line/CAN ECU Checksum"), "Checksum 1 corrected");
    }
    else
    {
        qDebug() << "Subaru Hitachi M32R K-Line/CAN ECU checksum 1 OK";
        //QMessageBox::information(this, tr("Subaru Hitachi M32R K-Line/CAN ECU Checksum"), "Checksum 1 OK");
    }
    if (checksum_2_value_calculated != checksum_2_value_stored)
    {
        qDebug() << "Checksum 2 value mismatch!";
        QByteArray checksum;
        checksum.append((uint8_t)(checksum_2_value_calculated >> 24));
        checksum.append((uint8_t)(checksum_2_value_calculated >> 16));
        checksum.append((uint8_t)(checksum_2_value_calculated >> 8));
        checksum.append((uint8_t)checksum_2_value_calculated);
        romData.replace(checksum_2_value_address, checksum.length(), checksum);
        qDebug() << "Subaru Hitachi M32R K-Line/CAN ECU checksum 2 corrected";
        //QMessageBox::information(this, tr("Subaru Hitachi M32R K-Line/CAN ECU Checksum"), "Checksum 2 corrected");
    }
    else
    {
        qDebug() << "Subaru Hitachi M32R K-Line/CAN ECU checksum 2 OK";
        //QMessageBox::information(this, tr("Subaru Hitachi M32R K-Line/CAN ECU Checksum"), "Checksum 2 OK");
    }
    if (checksum_3_value_calculated != checksum_3_value_stored)
    {
        qDebug() << "Checksum 3 value mismatch!";
        QByteArray checksum;
        checksum.append((uint8_t)(checksum_3_value_calculated >> 24));
        checksum.append((uint8_t)(checksum_3_value_calculated >> 16));
        checksum.append((uint8_t)(checksum_3_value_calculated >> 8));
        checksum.append((uint8_t)checksum_3_value_calculated);
        romData.replace(checksum_3_value_address, checksum.length(), checksum);
        qDebug() << "Subaru Hitachi M32R K-Line/CAN ECU checksum 3 corrected";
        //QMessageBox::information(this, tr("Subaru Hitachi M32R K-Line/CAN ECU Checksum"), "Checksum 3 corrected");
    }
    else
    {
        qDebug() << "Subaru Hitachi M32R K-Line/CAN ECU checksum 3 OK";
        //QMessageBox::information(this, tr("Subaru Hitachi M32R K-Line/CAN ECU Checksum"), "Checksum 3 OK");
    }
    if (checksum_4_value_calculated != checksum_4_value_stored)
    {
        qDebug() << "Checksum 4 value mismatch!";
        QByteArray checksum;
        checksum.append((uint8_t)(checksum_4_value_calculated >> 24));
        checksum.append((uint8_t)(checksum_4_value_calculated >> 16));
        checksum.append((uint8_t)(checksum_4_value_calculated >> 8));
        checksum.append((uint8_t)checksum_4_value_calculated);
        romData.replace(checksum_4_value_address, checksum.length(), checksum);
        qDebug() << "Subaru Hitachi M32R K-Line/CAN ECU checksum 4 corrected";
        //QMessageBox::information(this, tr("Subaru Hitachi M32R K-Line/CAN ECU Checksum"), "Checksum 4 corrected");
    }
    else
    {
        qDebug() << "Subaru Hitachi M32R K-Line/CAN ECU checksum 4 OK";
        //QMessageBox::information(this, tr("Subaru Hitachi M32R K-Line/CAN ECU Checksum"), "Checksum 4 OK");
    }
    if (checksum_5_value_calculated != 0x5aa5)
    {
        qDebug() << "Checksum 5 balance value mismatch!";
        QByteArray balance_value_array;
        uint16_t balance_value = (uint16_t)(romData.at(checksum_5_balance_value_address) << 8) + (uint16_t)(romData.at(checksum_5_balance_value_address + 1));

        msg.clear();
        msg.append(QString("Balance value before: 0x%1").arg(balance_value,4,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        balance_value += 0x5aa5 - checksum_4_value_calculated;

        msg.clear();
        msg.append(QString("Balance value after: 0x%1").arg(balance_value,4,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        balance_value_array.append((uint8_t)((balance_value >> 8) & 0xff));
        balance_value_array.append((uint8_t)(balance_value & 0xff));
        romData.replace(checksum_5_balance_value_address, balance_value_array.length(), balance_value_array);

        qDebug() << "Subaru Hitachi M32R K-Line/CAN ECU checksum 5 corrected";
        //QMessageBox::information(this, tr("Subaru Hitachi M32R K-Line/CAN ECU Checksum"), "Checksum 5 corrected");
    }
    else
    {
        qDebug() << "Subaru Hitachi M32R K-Line/CAN ECU checksum 5 OK";
        //QMessageBox::information(this, tr("Subaru Hitachi M32R K-Line/CAN ECU Checksum"), "Checksum 5 OK");
    }

    if (checksum_6_balance_value_calculated != checksum_6_balance_value_stored)
    {
        qDebug() << "Checksum 6 balance value mismatch!";

        //qDebug() << "Subaru Hitachi M32R K-Line/CAN ECU checksum 5 corrected";
        //QMessageBox::information(this, tr("Subaru Hitachi M32R K-Line/CAN ECU Checksum"), "Checksum 5 corrected");
    }
    else
    {
        qDebug() << "Subaru Hitachi M32R K-Line/CAN ECU checksum 6 balance value OK";
        //QMessageBox::information(this, tr("Subaru Hitachi M32R K-Line/CAN ECU Checksum"), "Checksum 6 balance value OK");
    }


    return romData;
}
