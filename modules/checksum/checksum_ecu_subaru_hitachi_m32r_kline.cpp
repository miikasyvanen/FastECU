#include "checksum_ecu_subaru_hitachi_m32r_kline.h"

ChecksumEcuSubaruHitachiM32rKline::ChecksumEcuSubaruHitachiM32rKline()
{

}

ChecksumEcuSubaruHitachiM32rKline::~ChecksumEcuSubaruHitachiM32rKline()
{

}

QByteArray ChecksumEcuSubaruHitachiM32rKline::calculate_checksum(QByteArray romData)
{
    /*******************
     *
     *  Checksum 1 calculated between 0x0000 - 0x7ffff excluding 0x8100 - 0x8101, 8bit SUM, result at 0x8100
     *  Checksum 2 calculated between 0x0000 - 0x7ffff excluding 0x8100 - 0x8101, 8bit XOR, result at 0x8101
     *  Checksum 3 calculated between 0x4000 - 0x7ffff excluding 0x8100 - 0x8103, 32bit sum, 16bit result must mach 0x5aa5, balancing address 0x7fffa
     *
     * ****************/
    QString msg;

    uint8_t checksum_1_value_stored = 0;
    uint8_t checksum_1_value_calculated = 0;
    uint32_t checksum_1_value_address = 0x8100;
    uint8_t checksum_2_value_stored = 0;
    uint8_t checksum_2_value_calculated = 0;
    uint32_t checksum_2_value_address = 0x8101;

    uint16_t checksum_3_value_calculated = 0;
    uint16_t checksum_3_balance_value_stored = 0;
    uint32_t checksum_3_balance_value_address = 0x7fffa;

    bool checksum_ok = true;

    /****************************************
     *
     * Calculate and fix checksums 1 and 2
     *
     * *************************************/
    for (int i = 0x0000; i < romData.length(); i += 1)
    {
        if (i < 0x8100 || i > 0x8101)
        {
            checksum_1_value_calculated += (uint8_t)romData.at(i);
            checksum_2_value_calculated ^= (uint8_t)romData.at(i);
        }
    }
    checksum_1_value_stored = (uint8_t)romData.at(checksum_1_value_address);
    checksum_2_value_stored = (uint8_t)romData.at(checksum_2_value_address);
    if (checksum_1_value_calculated != checksum_1_value_stored)
    {
        qDebug() << "Checksum 1 value mismatch!";
        checksum_ok = false;

        QByteArray checksum;
        checksum.append((uint8_t)checksum_1_value_calculated);
        romData.replace(checksum_1_value_address, checksum.length(), checksum);

        qDebug() << "Subaru Hitachi M32R K-Line/CAN ECU checksum 1 corrected";
    }
    else
    {
        qDebug() << "Subaru Hitachi M32R K-Line/CAN ECU checksum 1 OK";
    }
    if (checksum_2_value_calculated != checksum_2_value_stored)
    {
        qDebug() << "Checksum 2 value mismatch!";
        checksum_ok = false;

        QByteArray checksum;
        checksum.append((uint8_t)checksum_2_value_calculated);
        romData.replace(checksum_2_value_address, checksum.length(), checksum);

        qDebug() << "Subaru Hitachi M32R K-Line/CAN ECU checksum 2 corrected";
    }
    else
    {
        qDebug() << "Subaru Hitachi M32R K-Line/CAN ECU checksum 2 OK";
    }

    /****************************************
     *
     * Calculate and fix checksum 5
     *
     * *************************************/
    for (int i = 0x4000; i < romData.length(); i += 4)
    {
        if (i < 0x8100 || i > 0x8103)
            checksum_3_value_calculated += ((uint8_t)romData.at(i) << 24) + ((uint8_t)romData.at(i + 1) << 16) + ((uint8_t)romData.at(i + 2) << 8) + (uint8_t)romData.at(i + 3);
    }
    checksum_3_balance_value_stored = ((uint8_t)romData.at(checksum_3_balance_value_address) << 8) + (uint8_t)romData.at(checksum_3_balance_value_address+1);
    if (checksum_3_value_calculated != 0x5aa5)
    {
        qDebug() << "Checksum 3 balance value mismatch!";
        checksum_ok = false;

        QByteArray balance_value_array;
        uint16_t balance_value = (uint16_t)(romData.at(checksum_3_balance_value_address) << 8) + (uint16_t)(romData.at(checksum_3_balance_value_address + 1));

        msg.clear();
        msg.append(QString("Balance value before: 0x%1").arg(balance_value,4,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        balance_value += 0x5aa5 - checksum_3_value_calculated;

        msg.clear();
        msg.append(QString("Balance value after: 0x%1").arg(balance_value,4,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        balance_value_array.append((uint8_t)((balance_value >> 8) & 0xff));
        balance_value_array.append((uint8_t)(balance_value & 0xff));
        romData.replace(checksum_3_balance_value_address, balance_value_array.length(), balance_value_array);

        qDebug() << "Subaru Hitachi M32R K-Line/CAN ECU checksum 3 corrected";
    }
    else
    {
        qDebug() << "Subaru Hitachi M32R K-Line/CAN ECU checksum 3 OK";
    }

    msg.clear();
    msg.append(QString("Checksum 1 value calculated 0x8100: 0x%1").arg(checksum_1_value_calculated,2,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("Checksum 1 value stored 0x8100: 0x%1").arg(checksum_1_value_stored,2,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    msg.clear();
    msg.append(QString("Checksum 2 value calculated 0x8101: 0x%1").arg(checksum_2_value_calculated,2,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("Checksum 2 value stored 0x8101: 0x%1").arg(checksum_2_value_stored,2,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    msg.clear();
    msg.append(QString("Checksum 3 value calculated: 0x%1").arg(checksum_3_value_calculated,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("Checksum 3 balance value stored 0x7fffa: 0x%1").arg(checksum_3_balance_value_stored,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    if (!checksum_ok)
    {
        QMessageBox::information(this, tr("Subaru Hitachi M32R K-Line ECU Checksum"), "Checksums corrected");
    }

    return romData;
}
