#include "checksum_ecu_subaru_hitachi_m32r.h"

ChecksumEcuSubaruHitachiM32r::ChecksumEcuSubaruHitachiM32r()
{

}

ChecksumEcuSubaruHitachiM32r::~ChecksumEcuSubaruHitachiM32r()
{

}

QByteArray ChecksumEcuSubaruHitachiM32r::calculate_checksum(QByteArray romData)
{
    uint32_t chksum = 0;
    uint32_t chksum1 = 0;
    uint32_t chksum2 = 0;
    uint8_t chksum_2[4];
    for (int i = 0x4000; i < romData.length(); i += 4)
    {
        if (i < 0x10000 || i > 0x10003)
            chksum += (romData.at(i) << 24) + (romData.at(i + 1) << 16) + (romData.at(i + 2) << 8) + romData.at(i + 3);
    }
    for (int i = 0; i < romData.length(); i += 1)
    {
        if (i < 0x10000 || i > 0x10007)
        {
            chksum1 += romData.at(i);
            chksum2 ^= i;
        }
    }
    chksum_2[3] = ((chksum >> 24) & 0xff);
    chksum_2[2] = ((chksum >> 16) & 0xff);
    chksum_2[1] = ((chksum >> 8) & 0xff);
    chksum_2[0] = (chksum & 0xff);

    QString msg;
    msg.append(QString("0x%1").arg(chksum_2[3],2,16,QLatin1Char('0')));
    msg.append(QString("%1").arg(chksum_2[2],2,16,QLatin1Char('0')));
    msg.append(QString("%1").arg(chksum_2[1],2,16,QLatin1Char('0')));
    msg.append(QString("%1").arg(chksum_2[0],2,16,QLatin1Char('0')));

    qDebug() << "ChkSum = " + QString::number(chksum) + " (" + msg + ")";

    chksum_2[3] = ((chksum1 >> 24) & 0xff);
    chksum_2[2] = ((chksum1 >> 16) & 0xff);
    chksum_2[1] = ((chksum1 >> 8) & 0xff);
    chksum_2[0] = (chksum1 & 0xff);
    msg.clear();
    msg.append(QString("0x%1").arg(chksum_2[3],2,16,QLatin1Char('0')));
    msg.append(QString("%1").arg(chksum_2[2],2,16,QLatin1Char('0')));
    msg.append(QString("%1").arg(chksum_2[1],2,16,QLatin1Char('0')));
    msg.append(QString("%1").arg(chksum_2[0],2,16,QLatin1Char('0')));
    qDebug() << "ChkSum1 = " + QString::number(chksum) + " (" + msg + ")";

    chksum1 = (chksum1 * 256) + chksum2;
    chksum_2[3] = ((chksum1 >> 24) & 0xff);
    chksum_2[2] = ((chksum1 >> 16) & 0xff);
    chksum_2[1] = ((chksum1 >> 8) & 0xff);
    chksum_2[0] = (chksum1 & 0xff);
    msg.clear();
    msg.append(QString("0x%1").arg(chksum_2[3],2,16,QLatin1Char('0')));
    msg.append(QString("%1").arg(chksum_2[2],2,16,QLatin1Char('0')));
    msg.append(QString("%1").arg(chksum_2[1],2,16,QLatin1Char('0')));
    msg.append(QString("%1").arg(chksum_2[0],2,16,QLatin1Char('0')));
    qDebug() << "ChkSum2 = " + QString::number(chksum) + " (" + msg + ")";


    return romData;
}
