#ifndef CHECKSUM_ECU_SUBARU_HITACHI_M32R_KLINE_H
#define CHECKSUM_ECU_SUBARU_HITACHI_M32R_KLINE_H

#include <QDebug>
#include <QMessageBox>
#include <QWidget>

class ChecksumEcuSubaruHitachiM32rKline : public QWidget
{
    Q_OBJECT

public:
    ChecksumEcuSubaruHitachiM32rKline();
    ~ChecksumEcuSubaruHitachiM32rKline();

    QByteArray calculate_checksum(QByteArray romData);

private:

};

#endif // CHECKSUM_ECU_SUBARU_HITACHI_M32R_KLINE_H
