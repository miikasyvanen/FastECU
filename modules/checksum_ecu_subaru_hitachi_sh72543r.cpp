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
     *
     * 102d9447 entry
     ******************/

    QByteArray msg;
    uint16_t chksum_6010 = 0;
    uint16_t chksum_1ffffe = 0;

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

    uint8_t uVar1 = 0;
    uint32_t uVar2 = 0;
    uint32_t uVar3 = 0;
    uint32_t uVar4 = 0;
    uint32_t pbVar5;

    for (pbVar5 = 0x6014; pbVar5 < 0x1ffff2; pbVar5+=1)
    {
        if (pbVar5 < 0x6080 || (0x6080 + 0x22) <= pbVar5)
        {
            //uVar2 = (((uint8_t)romData.at(pbVar5) << 24) | ((uint8_t)romData.at(pbVar5 + 1) << 16) | ((uint8_t)romData.at(pbVar5 + 2) << 8) | ((uint8_t)romData.at(pbVar5 + 3) << 0)) << 8;
            //uVar2 = (((uint8_t)romData.at(pbVar5) << 8) | ((uint8_t)romData.at(pbVar5 + 1) << 0)) << 8;
            uVar2 = (uint8_t)romData.at(pbVar5) << 8;
            for (uVar1 = 0; uVar1 < 8; uVar1++) {
                uVar3 = uVar4 & 0xffff;
                uVar4 = uVar3 << 1;
                if ((((uVar2 & 0xffff) ^ uVar3) & 0x8000) != 0) {
                    uVar4 = uVar4 ^ 0x1021;
                }
                uVar2 = (uVar2 & 0xffff) << 1;
            }
        }
    }

    // FUN_00062126, fff80280 = iVar4, *DAT_000622d4 = iVar4;, fff8027d |= 0x80
    bool bVar1 = false;
    uint8_t bVar3 = 0;
    int iVar4 = 0;
    uint16_t puVar2 = 0;
    uint uVar5 = 0;
    uint uVar6 = 0;

    puVar2 = uVar4;
    iVar4 = 0;
    uVar5 = uVar4 * 2;

    if (((int)(short)uVar4 & 0x8000U) != 0)
        uVar5++;
    uVar5 ^= (int)(short)uVar4;

    // DAT_000622d0 = uVar5
    qDebug() << "-----------------------";
    msg.clear();
    msg.append(QString("uVar2: 0x%1").arg(uVar2,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("uVar3: 0x%1").arg(uVar3,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("uVar4: 0x%1").arg(uVar4,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("uVar5: 0x%1").arg(uVar5,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    uVar6 = (uint)(short)puVar2;
    for (bVar3 = 0; bVar3 < 0x20; bVar3 = bVar3 + 1) {
        iVar4 = iVar4 * 2;
        if (bVar1) {
            if ((uVar5 & 0x8000) != 0) {
                iVar4 = iVar4 + 1;
            }
            uVar5 = (uVar5 & 0xffff) << 1;
            bVar1 = false;
        }
        else {
            if ((uVar6 & 0x8000) != 0) {
                iVar4 = iVar4 + 1;
            }
            uVar6 = uVar6 << 1;
            bVar1 = true;
        }
    }

    qDebug() << "-----------------------";
    msg.clear();
    msg.append(QString("puVar2: 0x%1").arg(puVar2,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("iVar4: 0x%1").arg(iVar4,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("uVar5: 0x%1").arg(uVar5,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("uVar6: 0x%1").arg(uVar6,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    qDebug() << "-----------------------";

    return romData;
}
