#ifndef CHECKSUM_ECU_SUBARU_HITACHI_SH7058_H
#define CHECKSUM_ECU_SUBARU_HITACHI_SH7058_H

#include <QDebug>
#include <QMessageBox>
#include <QWidget>

class ChecksumEcuSubaruHitachiSH7058 : public QWidget
{
    Q_OBJECT

public:
    ChecksumEcuSubaruHitachiSH7058();
    ~ChecksumEcuSubaruHitachiSH7058();

    QByteArray calculate_checksum(QByteArray romData);

private:

};

#endif // CHECKSUM_ECU_SUBARU_HITACHI_SH7058_H
