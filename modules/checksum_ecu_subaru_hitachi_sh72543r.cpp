#include "checksum_ecu_subaru_hitachi_sh72543r.h"

ChecksumEcuSubaruHitachiSh72543r::ChecksumEcuSubaruHitachiSh72543r()
{

}

ChecksumEcuSubaruHitachiSh72543r::~ChecksumEcuSubaruHitachiSh72543r()
{

}

QByteArray ChecksumEcuSubaruHitachiSh72543r::calculate_checksum(QByteArray romData)
{
    /*************
     *
     * LF79002P
     *
     * UndefinedFunction_00077508
     * FUN_00077562
     * FUN_000775ca
     * FUN_0007763e
     *
     * sVar6 != *DAT_000b2644
     * *DAT_000b2ddc = sVar6;
     *
     * 0x10000 - 0x63930
     * 0x63930 - 0x1fffe0
     *
     *
     * 000b2c7e d6 57           mov.l      @(DAT_000b2ddc,pc),r6                            = FFF80AF2h
     * 000b2ddc - FFF80AF2h
     *
     * 000b25b2 d6 24           mov.l      @(DAT_000b2644,pc),r6                            = FFF80AF2h
     * 000b2644 - FFF80AF2h
     *
     * DAT_fff806d0
     * DAT_fff806d2
     *
     *
     *
     */

    QByteArray msg;

    uint32_t checksum_1_value_calculated = 0;
    uint32_t checksum_2_value_calculated = 0;

    for (int i = 0x10000; i < 0x1ffffc; i+=4)
    {
        checksum_1_value_calculated += ((uint8_t)romData.at(i) << 24) + ((uint8_t)romData.at(i + 1) << 16) + ((uint8_t)romData.at(i + 2) << 8) + (uint8_t)romData.at(i + 3);
        checksum_2_value_calculated ^= ((uint8_t)romData.at(i) << 24) + ((uint8_t)romData.at(i + 1) << 16) + ((uint8_t)romData.at(i + 2) << 8) + (uint8_t)romData.at(i + 3);
    }

    msg.clear();
    msg.append(QString("1: 0x%1 | 2: 0x%2").arg(checksum_1_value_calculated,8,16,QLatin1Char('0')).arg(checksum_2_value_calculated,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    uint8_t uVar1 = 0;
    uint32_t uVar2 = 0;
    uint32_t uVar3 = 0;
    uint32_t uVar4 = 0;
    uint32_t pbVar5;

    uint32_t value = (uint8_t)romData.at(0x6000) << 8;
    msg.clear();
    msg.append(QString("Value in 0x6000 << 8: 0x%1").arg(value,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    // FUN_000620be, fff8065a = uVar4, DAT_000622cc = (short)uVar4;
    for (pbVar5 = 0x6014; pbVar5 < 0x1ffff2; pbVar5+=1)
    {
        if (pbVar5 < 0x6080 || (0x6080 + 0x22) <= pbVar5)
        {
            //uVar2 = (((uint8_t)romData.at(pbVar5) << 24) | ((uint8_t)romData.at(pbVar5 + 1) << 16) | ((uint8_t)romData.at(pbVar5 + 2) << 8) | ((uint8_t)romData.at(pbVar5 + 3) << 0)) << 8;
            //uVar2 = (((uint8_t)romData.at(pbVar5 + 2) << 8) | ((uint8_t)romData.at(pbVar5 + 3) << 0)) << 8;
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
    uint8_t bVar3;
    uint32_t iVar4;
    uint16_t puVar2 = 0;
    uint32_t uVar5 = 0;
    uint32_t uVar6 = 0;

    puVar2 = uVar4;
    iVar4 = 0;
    uVar5 = uVar4 * 2;

    if (((int)(short)uVar4 & 0x8000U) != 0)
        uVar5++;
    uVar5 ^= uVar4;
    // DAT_000622d0 = uVar5
    msg.clear();
    msg.append(QString("uVar5: 0x%1").arg(uVar5,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    uVar6 = puVar2;
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

    uint16_t uVar11;
    uint32_t puVar12;

    uint32_t param_1 = 0x10000;
    uint32_t param_2 = 0x63930;

    uint16_t romdata = ((uint8_t)romData.at(param_1) << 8) + (uint8_t)romData.at(param_1 + 1);
    uVar11 = (romdata >> 0xf) | (romdata << 1);
    for (puVar12 = param_1+2; puVar12 < param_2; puVar12+=2) {
        romdata = ((uint8_t)romData.at(puVar12) << 8) + (uint8_t)romData.at(puVar12 + 1);
        //uVar11 = (((uint8_t)romData.at(puVar12) << 8) + (uint8_t)romData.at(puVar12 + 1)) + (uVar11 & 0xffff);
        //uVar11 = (uint8_t)romData.at(puVar12) + (uVar11 & 0xffff);
        uVar11 = romdata + (uVar11 & 0xffff);
        uVar11 = (uVar11 >> 0xf) | (uVar11 << 1);
    }

    uint32_t chksum = uVar11 & 0xffff;
/*
    param_1 = 0x78c0;
    param_2 = 0x10000;

    romdata = ((uint8_t)romData.at(param_1) << 8) + (uint8_t)romData.at(param_1 + 1);
    uVar11 = (romdata >> 0xf) | (romdata << 1);
    for (puVar12 = param_1+2; puVar12 < param_2; puVar12+=2) {
        romdata = ((uint8_t)romData.at(puVar12) << 8) + (uint8_t)romData.at(puVar12 + 1);
        //uVar11 = (((uint8_t)romData.at(puVar12) << 8) + (uint8_t)romData.at(puVar12 + 1)) + (uVar11 & 0xffff);
        //uVar11 = (uint8_t)romData.at(puVar12) + (uVar11 & 0xffff);
        uVar11 = romdata + (uVar11 & 0xffff);
        uVar11 = (uVar11 >> 0xf) | (uVar11 << 1);
    }
*/
    uint32_t chksum2 = 0;//uVar11 & 0xFFFF;

    msg.clear();
    msg.append(QString("uVar11: 0x%1").arg(uVar11,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    param_1 = 0x63930;
    param_2 = 0x1fffe0;

    romdata = ((uint8_t)romData.at(param_1) << 8) + (uint8_t)romData.at(param_1 + 1);
    uVar11 = (romdata >> 0xf) | (romdata << 1);
    for (puVar12 = param_1+2; puVar12 < param_2; puVar12+=2) {
        romdata = ((uint8_t)romData.at(puVar12) << 8) + (uint8_t)romData.at(puVar12 + 1);
        //uVar11 = (((uint8_t)romData.at(puVar12) << 8) + (uint8_t)romData.at(puVar12 + 1)) + (uVar11 & 0xffff);
        //uVar11 = (uint8_t)romData.at(puVar12) + (uVar11 & 0xffff);
        uVar11 = romdata + (uVar11 & 0xffff);
        uVar11 = (uVar11 >> 0xf) | (uVar11 << 1);
    }

    chksum2 += uVar11 & 0xFFFF;
    chksum = chksum * 0x10000 + chksum2;

    // *DAT_0006229c = *DAT_0006229c | 0x80;
    // *DAT_000622bc = (uint)*DAT_000622b4 + *DAT_000622ac * 0x10000;

    msg.clear();
    msg.append(QString("uVar11: 0x%1").arg(uVar11,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    msg.clear();
    msg.append(QString("chksum: 0x%1").arg(chksum,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    msg.clear();
    msg.append(QString("uVar2: 0x%1").arg(uVar2,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("uVar3: 0x%1").arg(uVar3,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("uVar5: 0x%1").arg(uVar5,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("uVar6: 0x%1").arg(uVar6,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    msg.clear();
    msg.append(QString("uVar4: 0x%1").arg(uVar4,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("iVar4: 0x%1").arg(iVar4,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    return romData;
}
