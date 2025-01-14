#include "checksum_ecu_subaru_hitachi_sh7058.h"

ChecksumEcuSubaruHitachiSH7058::ChecksumEcuSubaruHitachiSH7058()
{

}

ChecksumEcuSubaruHitachiSH7058::~ChecksumEcuSubaruHitachiSH7058()
{

}

QByteArray ChecksumEcuSubaruHitachiSH7058::calculate_checksum(QByteArray romData)
{
    /*******************
     *
     *  Checksum 1 calculated between 0x18000 - 0x1dfff, 32bit sum, result at 0x7ffe8
     *  Checksum 2 calculated between 0x18000 - 0x1dfff, 32 bit XOR, result at 0x7ffec
     *  Checksum 5 calculated between 0x4000 - 0xfffff excluding 0xffff0 - 0xffff7, 32-bit sum. Balance value is
     *  calculated so that this matches with 0x5aa5a55a.
     *  Checksum 3 calculated between 0x0000 - 0xfffff excluding 0xffff0 - 0xffff7, 32-bit sum, result at 0x7fff0
     *  Checksum 4 calculated between 0x0000 - 0xfffff excluding 0xffff0 - 0xffff7, 32-bit XOR, result at 0x7fff4
     *
     * ****************/
    QString msg;

    uint32_t checksum_1_value_stored = 0;
    uint32_t checksum_1_value_calculated = 0;
    uint32_t checksum_1_value_address = 0xfffe8;
    uint32_t checksum_2_value_stored = 0;
    uint32_t checksum_2_value_calculated = 0;
    uint32_t checksum_2_value_address = 0xfffec;
    uint32_t checksum_3_value_stored = 0;
    uint32_t checksum_3_value_calculated = 0;
    uint32_t checksum_3_value_address = 0xffff0;
    uint32_t checksum_4_value_stored = 0;
    uint32_t checksum_4_value_calculated = 0;
    uint32_t checksum_4_value_address = 0xffff4;

    uint32_t checksum_5_value_calculated = 0;
    uint32_t checksum_5_balance_value_address = 0xffff8;

    bool checksum_ok = true;

    /****************************************
     *
     * Calculate and fix checksums 1 and 2
     *
     * *************************************/
    for (int i = 0x18400; i < 0x1e000; i += 4)
    {
        checksum_1_value_calculated += ((uint8_t)romData.at(i) << 24) + ((uint8_t)romData.at(i + 1) << 16) + ((uint8_t)romData.at(i + 2) << 8) + (uint8_t)romData.at(i + 3);
        checksum_2_value_calculated ^= ((uint8_t)romData.at(i) << 24) + ((uint8_t)romData.at(i + 1) << 16) + ((uint8_t)romData.at(i + 2) << 8) + (uint8_t)romData.at(i + 3);
    }
    checksum_1_value_stored = ((uint8_t)romData.at(checksum_1_value_address) << 24) + ((uint8_t)romData.at(checksum_1_value_address+1) << 16) + ((uint8_t)romData.at(checksum_1_value_address+2) << 8) + (uint8_t)romData.at(checksum_1_value_address+3);
    checksum_2_value_stored = ((uint8_t)romData.at(checksum_2_value_address) << 24) + ((uint8_t)romData.at(checksum_2_value_address+1) << 16) + ((uint8_t)romData.at(checksum_2_value_address+2) << 8) + (uint8_t)romData.at(checksum_2_value_address+3);
    if (checksum_1_value_calculated != checksum_1_value_stored)
    {
        qDebug() << "Checksum 1 value mismatch!";
        checksum_ok = false;

        QByteArray checksum;
        checksum.append((uint8_t)(checksum_1_value_calculated >> 24));
        checksum.append((uint8_t)(checksum_1_value_calculated >> 16));
        checksum.append((uint8_t)(checksum_1_value_calculated >> 8));
        checksum.append((uint8_t)checksum_1_value_calculated);
        romData.replace(checksum_1_value_address, checksum.length(), checksum);

        qDebug() << "Subaru Hitachi SH7058 CAN ECU checksum 1 corrected";
    }
    else
    {
        qDebug() << "Subaru Hitachi SH7058 CAN ECU checksum 1 OK";
    }
    if (checksum_2_value_calculated != checksum_2_value_stored)
    {
        qDebug() << "Checksum 2 value mismatch!";
        checksum_ok = false;

        QByteArray checksum;
        checksum.append((uint8_t)(checksum_2_value_calculated >> 24));
        checksum.append((uint8_t)(checksum_2_value_calculated >> 16));
        checksum.append((uint8_t)(checksum_2_value_calculated >> 8));
        checksum.append((uint8_t)checksum_2_value_calculated);
        romData.replace(checksum_2_value_address, checksum.length(), checksum);

        qDebug() << "Subaru Hitachi SH7058 CAN ECU checksum 2 corrected";
    }
    else
    {
        qDebug() << "Subaru Hitachi SH7058 CAN ECU checksum 2 OK";
    }
    /****************************************
     *
     * Calculate and fix checksum 5
     *
     * *************************************/
    for (uint32_t i = 0x4000; i < (uint32_t)romData.length(); i += 4)
    {
        if (i != checksum_3_value_address && i != checksum_4_value_address)
            checksum_5_value_calculated += ((uint8_t)romData.at(i) << 24) + ((uint8_t)romData.at(i + 1) << 16) + ((uint8_t)romData.at(i + 2) << 8) + (uint8_t)romData.at(i + 3);
    }
    if (checksum_5_value_calculated != 0x5aa5a55a)
    {
        QByteArray balance_value_array;
        uint32_t balance_value = ((uint8_t)romData.at(checksum_5_balance_value_address) << 24) + ((uint8_t)romData.at(checksum_5_balance_value_address + 1) << 16) + ((uint8_t)romData.at(checksum_5_balance_value_address + 2) << 8) + ((uint8_t)romData.at(checksum_5_balance_value_address + 3));

        msg.clear();
        msg.append(QString("Balance value before: 0x%1").arg(balance_value,8,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        balance_value += 0x5aa5a55a - checksum_5_value_calculated;

        msg.clear();
        msg.append(QString("Balance value after: 0x%1").arg(balance_value,8,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        balance_value_array.append((uint8_t)((balance_value >> 24) & 0xff));
        balance_value_array.append((uint8_t)((balance_value >> 16) & 0xff));
        balance_value_array.append((uint8_t)((balance_value >> 8) & 0xff));
        balance_value_array.append((uint8_t)(balance_value & 0xff));
        romData.replace(checksum_5_balance_value_address, balance_value_array.length(), balance_value_array);

    }
    /****************************************
     *
     * Calculate and fix checksums 3 and 4
     *
     * *************************************/
    for (uint32_t i = 0x0000; i < 0x100000; i += 4)
    {
        if (i != checksum_3_value_address && i != checksum_4_value_address)
        {
            checksum_3_value_calculated += ((uint8_t)romData.at(i) << 24) + ((uint8_t)romData.at(i + 1) << 16) + ((uint8_t)romData.at(i + 2) << 8) + (uint8_t)romData.at(i + 3);
            checksum_4_value_calculated ^= ((uint8_t)romData.at(i) << 24) + ((uint8_t)romData.at(i + 1) << 16) + ((uint8_t)romData.at(i + 2) << 8) + (uint8_t)romData.at(i + 3);
        }
    }
    checksum_3_value_stored = ((uint8_t)romData.at(checksum_3_value_address) << 24) + ((uint8_t)romData.at(checksum_3_value_address+1) << 16) + ((uint8_t)romData.at(checksum_3_value_address+2) << 8) + (uint8_t)romData.at(checksum_3_value_address+3);
    checksum_4_value_stored = ((uint8_t)romData.at(checksum_4_value_address) << 24) + ((uint8_t)romData.at(checksum_4_value_address+1) << 16) + ((uint8_t)romData.at(checksum_4_value_address+2) << 8) + (uint8_t)romData.at(checksum_4_value_address+3);
    if (checksum_3_value_calculated != checksum_3_value_stored)
    {
        qDebug() << "Checksum 3 value mismatch!";
        checksum_ok = false;

        QByteArray checksum;
        checksum.append((uint8_t)(checksum_3_value_calculated >> 24));
        checksum.append((uint8_t)(checksum_3_value_calculated >> 16));
        checksum.append((uint8_t)(checksum_3_value_calculated >> 8));
        checksum.append((uint8_t)checksum_3_value_calculated);
        romData.replace(checksum_3_value_address, checksum.length(), checksum);

/*
        QByteArray balance_value_array;
        uint32_t balance_value = ((uint8_t)romData.at(checksum_3_balance_value_address) << 24) + ((uint8_t)romData.at(checksum_3_balance_value_address + 1) << 16) + ((uint8_t)romData.at(checksum_3_balance_value_address + 2) << 8) + ((uint8_t)romData.at(checksum_3_balance_value_address + 3));

        msg.clear();
        msg.append(QString("Balance value before: 0x%1").arg(balance_value,8,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        romData.replace(checksum_3_value_address, 4, "\xC1\x4F\xB6\xC1");
        checksum_3_value_stored = 0xC14FB6C1;
        balance_value += checksum_3_value_stored - checksum_3_value_calculated;

        msg.clear();
        msg.append(QString("Balance value after: 0x%1").arg(balance_value,8,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        balance_value_array.append((uint8_t)((balance_value >> 24) & 0xff));
        balance_value_array.append((uint8_t)((balance_value >> 16) & 0xff));
        balance_value_array.append((uint8_t)((balance_value >> 8) & 0xff));
        balance_value_array.append((uint8_t)(balance_value & 0xff));
        romData.replace(checksum_3_balance_value_address, balance_value_array.length(), balance_value_array);
*/
        qDebug() << "Subaru Hitachi SH7058 CAN ECU checksum 3 corrected";
    }
    else
    {
        qDebug() << "Subaru Hitachi SH7058 CAN ECU checksum 3 OK";
    }
    if (checksum_4_value_calculated != checksum_4_value_stored)
    {
        qDebug() << "Checksum 4 value mismatch!";
        checksum_ok = false;

        msg.clear();
        msg.append(QString("CHKSUM: 0x%1").arg(checksum_4_value_calculated,8,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        checksum_4_value_calculated = 0;
        for (uint32_t i = 0x0000; i < 0x100000; i += 4)
        {
            if (i != checksum_3_value_address && i != checksum_4_value_address)
            {
                checksum_4_value_calculated ^= ((uint8_t)romData.at(i) << 24) + ((uint8_t)romData.at(i + 1) << 16) + ((uint8_t)romData.at(i + 2) << 8) + (uint8_t)romData.at(i + 3);
            }
        }

        QByteArray checksum;
        checksum.append((uint8_t)(checksum_4_value_calculated >> 24));
        checksum.append((uint8_t)(checksum_4_value_calculated >> 16));
        checksum.append((uint8_t)(checksum_4_value_calculated >> 8));
        checksum.append((uint8_t)checksum_4_value_calculated);
        romData.replace(checksum_4_value_address, checksum.length(), checksum);

        qDebug() << "Subaru Hitachi SH7058 CAN ECU checksum 4 corrected";
    }
    else
    {
        qDebug() << "Subaru Hitachi SH7058 CAN ECU checksum 4 OK";
    }

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

    if (!checksum_ok)
    {
        QMessageBox::information(nullptr, QObject::tr("Subaru Hitachi SH7058 CAN ECU Checksum"), "Checksums corrected");
    }


    return romData;
}
