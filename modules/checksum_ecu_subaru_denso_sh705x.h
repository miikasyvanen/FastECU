#ifndef CHECKSUM_ECU_SUBARU_DENSO_SH705X_H
#define CHECKSUM_ECU_SUBARU_DENSO_SH705X_H

#include <QDebug>
#include <QMessageBox>
#include <QWidget>

class ChecksumEcuSubaruDensoSH705x : public QWidget
{
    Q_OBJECT

public:
    ChecksumEcuSubaruDensoSH705x();
    ~ChecksumEcuSubaruDensoSH705x();

    QByteArray calculate_checksum(QByteArray romData, uint32_t checksum_area_start, uint32_t checksum_area_length);

private:

};

#endif // CHECKSUM_ECU_SUBARU_DENSO_SH705X_H
