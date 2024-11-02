#ifndef CHECKSUM_ECU_SUBARU_HITACHI_SH72543R_H
#define CHECKSUM_ECU_SUBARU_HITACHI_SH72543R_H

#include <QDebug>
#include <QMessageBox>
#include <QWidget>

class ChecksumEcuSubaruHitachiSh72543r : public QWidget
{
    Q_OBJECT

public:
    ChecksumEcuSubaruHitachiSh72543r();
    ~ChecksumEcuSubaruHitachiSh72543r();

    QByteArray calculate_checksum(QByteArray romData);

private:

};

#endif // CHECKSUM_ECU_SUBARU_HITACHI_SH72543R_H
