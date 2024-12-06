#ifndef CHECKSUM_TCU_SUBARU_HITACHI_M32R_CAN_H
#define CHECKSUM_TCU_SUBARU_HITACHI_M32R_CAN_H

#include <QDebug>
#include <QMessageBox>

class ChecksumTcuSubaruHitachiM32rCan
{
public:
    ChecksumTcuSubaruHitachiM32rCan();
    ~ChecksumTcuSubaruHitachiM32rCan();

    static QByteArray calculate_checksum(QByteArray romData);

private:

};

#endif // CHECKSUM_TCU_SUBARU_HITACHI_M32R_CAN_H
