#ifndef CHECKSUM_ECU_SUBARU_DENSO_SH7XXX_H
#define CHECKSUM_ECU_SUBARU_DENSO_SH7XXX_H

#include <QDebug>
#include <QMessageBox>
#include <QWidget>

class ChecksumEcuSubaruDensoSH7xxx : public QWidget
{
    Q_OBJECT

public:
    ChecksumEcuSubaruDensoSH7xxx();
    ~ChecksumEcuSubaruDensoSH7xxx();

    //Note that offset is added to all addresses
    QByteArray calculate_checksum(QByteArray romData, uint32_t checksum_area_start, uint32_t checksum_area_length, int32_t offset = 0);

private:

};

#endif // CHECKSUM_ECU_SUBARU_DENSO_SH7XXX_H
