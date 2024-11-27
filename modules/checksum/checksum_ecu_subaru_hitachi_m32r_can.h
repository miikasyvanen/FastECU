#ifndef CHECKSUM_ECU_SUBARU_HITACHI_M32R_CAN_H
#define CHECKSUM_ECU_SUBARU_HITACHI_M32R_CAN_H

#include <QDebug>
#include <QMessageBox>
#include <QWidget>

class ChecksumEcuSubaruHitachiM32rCan : public QWidget
{
    Q_OBJECT

public:
    ChecksumEcuSubaruHitachiM32rCan();
    ~ChecksumEcuSubaruHitachiM32rCan();

    QByteArray calculate_checksum(QByteArray romData);

private:

};

#endif // CHECKSUM_ECU_SUBARU_HITACHI_M32R_CAN_H
