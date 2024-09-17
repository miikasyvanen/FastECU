#include "checksum_ecu_subaru_hitachi_sh72543r.h"

ChecksumEcuSubaruHitachiSh72543r::ChecksumEcuSubaruHitachiSh72543r()
{

}

ChecksumEcuSubaruHitachiSh72543r::~ChecksumEcuSubaruHitachiSh72543r()
{

}
static unsigned int crc_table[256] = {
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
    0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
    0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
    0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
    0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
    0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
    0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
    0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
    0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
    0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
    0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
    0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
    0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
    0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
    0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
    0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
    0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
    0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
    0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
    0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
    0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
    0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
    0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
    0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
    0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
    0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
    0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
    0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
    0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
    0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
    0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
    0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
    0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
    0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
    0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
    0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
    0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
    0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
    0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
    0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
    0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
    0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
    0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
    0x2d02ef8dL
};


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
     *
     *
     * 0x6c == 0x6010
     *
     *
     * *DAT_00096550 (0xFFF80A8E) == 0x1fffe4-0x1fffe5
     * *DAT_00096ccc = sVar6;
     *
     *
     */

    uint8_t uVar1 = 0;
    uint32_t uVar2 = 0;
    uint32_t uVar3 = 0;
    uint32_t uVar4 = 0;
    uint32_t pbVar5;

    QByteArray msg;

    // FUN_000620be, fff8065a = uVar4, DAT_000622cc = (short)uVar4;
    for (pbVar5 = 0x6014; pbVar5 < 0x1ffff2; pbVar5+=1)
    {
        if (pbVar5 < 0x6080 || (0x6080 + 0x22) <= pbVar5)
        {
            uVar2 = (uint8_t)romData.at(pbVar5)  << 8;
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
    uint32_t puVar2 = 0;
    uint32_t uVar5 = 0;
    uint32_t uVar6 = 0;

    puVar2 = uVar4;
    iVar4 = 0;
    uVar5 = uVar4 * 2;

    if (((uint16_t)uVar4 & 0x8000) != 0)
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

    // *DAT_000622ac = (short)uVar1;
    uint32_t param_1 = 0x10000;
    uint32_t param_2 = 0x51070;

    uint32_t chksum1 = 0;
    uVar11 = (param_1 >> 0xf) | (param_1 << 1);
    for (puVar12 = param_1+1; puVar12 < param_2; puVar12+=1) {
        //uVar11 = (((uint8_t)romData.at(puVar12) << 8) + (uint8_t)romData.at(puVar12 + 1)) + (uVar11 & 0xffff);
        uVar11 = (uint8_t)romData.at(puVar12) + (uVar11 & 0xffff);
        uVar11 = (uVar11 >> 0xf) | (uVar11 << 1);
        //chksum1 += (((uint8_t)romData.at(puVar12) << 8) + (uint8_t)romData.at(puVar12 + 1));
    }

    uint32_t chksum = uVar11 * 0x10000;

    msg.clear();
    msg.append(QString("uVar11: 0x%1").arg(uVar11,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    // *DAT_000622b4 = (ushort)uVar1;
    // *DAT_000622b8 = *DAT_000622b8 | 1;
    param_1 = 0x51070;
    param_2 = 0x1fffe0;

    uint32_t chksum2 = 0;
    uVar11 = (param_1 >> 0xf) | (param_1 << 1);
    for (puVar12 = param_1+1; puVar12 < param_2; puVar12+=1) {
        //uVar11 = (((uint8_t)romData.at(puVar12) << 8) + (uint8_t)romData.at(puVar12 + 1)) + (uVar11 & 0xffff);
        uVar11 = (uint8_t)romData.at(puVar12) + (uVar11 & 0xffff);
        uVar11 = (uVar11 >> 0xf) | (uVar11 << 1);
        //chksum2 += (((uint8_t)romData.at(puVar12) << 8) + (uint8_t)romData.at(puVar12 + 1));
    }


    // *DAT_0006229c = *DAT_0006229c | 0x80;
    // *DAT_000622bc = (uint)*DAT_000622b4 + *DAT_000622ac * 0x10000;

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

    uint16_t crc16_hi = 0xFFFF;
    uint16_t crc16_lo = 0xFFFF;
    uint32_t crc32_1 = 0;
    uint32_t crc32_2 = 0;
    bool crc_carry = false;
    bool crc32_match = false;
    int crc_table_match = 0;
    int loop = 0;

    for (int i = 0; i < 257; i++)
    {
        loop = 8;
        while(loop > 0)
        {
            crc_carry = false;
            if(crc16_lo & 0x0001)
                crc_carry = true;
            crc16_lo >>= 1;
            if(crc16_hi & 0x0001)
                crc16_lo |= 0x8000;
            crc16_hi >>= 1;

            if (crc_carry)
            {
                crc16_lo ^= 0x8320;
                crc16_hi ^= 0xEDB8;
            }
            loop--;
        }

        crc32_1 = (crc16_hi << 16) | crc16_lo;
        crc32_2 = ~crc32_1;

        for (int j = 0; j < 257; j++)
        {
        if (crc32_2 == crc_table[j])
            crc_table_match++;
        }
        msg.clear();
        msg.append(QString("0x%1 | 0x%2 | 0x%3").arg(crc32_1,8,16,QLatin1Char('0')).arg(crc32_2,8,16,QLatin1Char('0')).arg(crc_table_match,4,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;
    }
    if (crc32_match)
        qDebug() << "CRC32 MATCH!!!";
    else
        qDebug() << "NO CRC32 MATCH!!!";


    return romData;
}
