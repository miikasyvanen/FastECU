#ifndef CHECKSUM_ECU_SUBARU_HITACHI_M32R_H
#define CHECKSUM_ECU_SUBARU_HITACHI_M32R_H

#include <QDebug>
#include <QMessageBox>
#include <QWidget>

class ChecksumEcuSubaruHitachiM32r : public QWidget
{
    Q_OBJECT

public:
    ChecksumEcuSubaruHitachiM32r();
    ~ChecksumEcuSubaruHitachiM32r();

    QByteArray calculate_checksum(QByteArray romData);

private:

};

#endif // CHECKSUM_ECU_SUBARU_HITACHI_M32R_H
