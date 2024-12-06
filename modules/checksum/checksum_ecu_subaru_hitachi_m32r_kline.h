#ifndef CHECKSUM_ECU_SUBARU_HITACHI_M32R_KLINE_H
#define CHECKSUM_ECU_SUBARU_HITACHI_M32R_KLINE_H

#include <QDebug>
#include <QMessageBox>

class ChecksumEcuSubaruHitachiM32rKline
{
public:
    ChecksumEcuSubaruHitachiM32rKline();
    ~ChecksumEcuSubaruHitachiM32rKline();

    static QByteArray calculate_checksum(QByteArray romData);

private:

};

#endif // CHECKSUM_ECU_SUBARU_HITACHI_M32R_KLINE_H
