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
     * UndefinedFunction_00062064
     * FUN_000620be
     * FUN_00062126
     *
     * FUN_00078ca6
     * 0x6000 = 0x5555
     * 0x1ffff2 = 0xAAAA
     *
     * 0x10000 - 0x51070
     * 0x51070 - 0x1fffe0
     */

    uint8_t uVar1 = 0;
    uint32_t uVar2 = 0;
    uint32_t uVar3 = 0;
    uint32_t uVar4 = 0;
    uint32_t pbVar5;

    QByteArray msg;

    // FUN_000620be, fff8065a = uVar4
    for (pbVar5 = 0x6014; pbVar5 < 0x1ffff2; pbVar5+=1)
    {
        if (pbVar5 < 0x6080 || (0x6080 + 0x22) <= pbVar5)
        {
            uVar2 = ((uint8_t)romData.at(pbVar5) << 8);
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

    // FUN_00062126, fff80280 = iVar4, fff8027d |= 0x80
    bool bVar1 = false;
    uint8_t bVar3;
    uint32_t iVar4;
    uint32_t puVar2 = 0;
    uint32_t uVar5 = 0;
    uint32_t uVar6 = 0;

    puVar2 = uVar4;
    iVar4 = 0;
    uVar5 = uVar4 * 2;

    if ((uVar4 & 0x8000) != 0)
        uVar5++;
    uVar5 ^= uVar4;
    // DAT_000622d0 = uVar5
    msg.clear();
    msg.append(QString("uVar5: 0x%1").arg(uVar5,4,16,QLatin1Char('0')).toUtf8());
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
    uint32_t param_2 = 0x51070;

    uint32_t chksum1 = 0;
    uVar11 = (param_1 >> 0xf) | (param_1 << 1);
    for (puVar12 = param_1; puVar12 < param_2; puVar12+=2) {
        uVar11 = (((uint8_t)romData.at(puVar12) << 8) + (uint8_t)romData.at(puVar12 + 1)) + (uVar11 & 0xffff);
        uVar11 = (uVar11 >> 0xf) | (uVar11 << 1);
        chksum1 += (((uint8_t)romData.at(puVar12) << 8) + (uint8_t)romData.at(puVar12 + 1));
    }

    uint32_t chksum = uVar11 * 0x10000;

    msg.clear();
    msg.append(QString("uVar11: 0x%1").arg(uVar11,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    param_1 = 0x51070;
    param_2 = 0x1fffe0;

    uint32_t chksum2 = 0;
    uVar11 = (param_1 >> 0xf) | (param_1 << 1);
    for (puVar12 = param_1; puVar12 < param_2; puVar12+=2) {
        uVar11 = (((uint8_t)romData.at(puVar12) << 8) + (uint8_t)romData.at(puVar12 + 1)) + (uVar11 & 0xffff);
        uVar11 = (uVar11 >> 0xf) | (uVar11 << 1);
        chksum2 += (((uint8_t)romData.at(puVar12) << 8) + (uint8_t)romData.at(puVar12 + 1));
    }

    chksum += uVar11;

    msg.clear();
    msg.append(QString("uVar11: 0x%1").arg(uVar11,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    msg.clear();
    msg.append(QString("chksum: 0x%1").arg(chksum,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("chksum1: 0x%1").arg(chksum1,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("chksum1: 0x%1").arg(chksum2,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;


    msg.clear();
    msg.append(QString("uVar2: 0x%1").arg(uVar2,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("uVar3: 0x%1").arg(uVar3,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("uVar4: 0x%1").arg(uVar4,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("iVar4: 0x%1").arg(iVar4,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("uVar5: 0x%1").arg(uVar5,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    msg.clear();
    msg.append(QString("uVar6: 0x%1").arg(uVar6,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    return romData;
}
