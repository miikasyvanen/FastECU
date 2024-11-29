#ifndef CHECKSUM_ECU_SUBARU_HITACHI_M32R_CAN_H
#define CHECKSUM_ECU_SUBARU_HITACHI_M32R_CAN_H

#include <QDebug>
#include <QMessageBox>

class ChecksumEcuSubaruHitachiM32rCan
{
public:
    ChecksumEcuSubaruHitachiM32rCan();
    ~ChecksumEcuSubaruHitachiM32rCan();

    static QByteArray calculate_checksum(QByteArray romData);

private:

};

#endif // CHECKSUM_ECU_SUBARU_HITACHI_M32R_CAN_H
