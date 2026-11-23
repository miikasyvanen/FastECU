#include "checksum_ecu_subaru_rh850_di.h"

ChecksumEcuSubaruRh850Di::ChecksumEcuSubaruRh850Di()
{

}

ChecksumEcuSubaruRh850Di::~ChecksumEcuSubaruRh850Di()
{

}

QByteArray ChecksumEcuSubaruRh850Di::calculate_checksum(QByteArray romData)
{
    /*******************
     *
     * Subaru DI (RH850, 2022+ "LHB*" calibrations, e.g. 2022/2023/2024 WRX VB MT).
     *
     * The image is the flash region only: file offset 0 == CPU address 0x10000,
     * length 0x3F0000 (CPU 0x10000 - 0x3FFFFF). The little-endian 16-bit word sum
     * over the whole image must equal 0x5AA5 (all stock images seen: LHBHB10B,
     * LHBKC40M, LHBP400b).
     *
     * Balance word: the 16-bit LE value at file offset 0x3EFF7E (CPU 0x3FFF7E),
     * inside the constant image trailer
     *
     *   CPU 0x3FFF70: 00 00 AA AA FF FF FF FF FF FF FF FF 00 00 [xx xx] 01 FF FF FF F9 55 48 4E
     *
     * It is the only word of that trailer that differs between stock builds
     * (C2B4 / 8C00 / B3CD in 2022 / 2023 / 2024), which is where the factory
     * balances the sum. Its role is inferred from the images; no flash test yet.
     * (File offset 0x3DFF00 also restores the sum arithmetically, but it is erased
     * fill (FFFF) in every stock image, so the factory does not use it.)
     *
     ******************/

    const int image_size = 0x3F0000;
    const uint32_t balance_value_array_start = 0x3EFF7E;
    const uint16_t target = 0x5AA5;

    QByteArray msg;

    if (romData.length() != image_size)
    {
        qDebug() << "RH850 DI checksum: image size" << romData.length() << "is not" << image_size;
        QMessageBox::warning(nullptr, QObject::tr("Subaru RH850 DI ECU Checksum"),
                             QString("Image size is 0x%1 bytes, expected 0x%2 (CPU 0x10000-0x3FFFFF). Checksum not changed.")
                             .arg(romData.length(), 0, 16).arg(image_size, 0, 16));
        return romData;
    }

    uint16_t chksum = 0;
    for (int i = 0; i + 1 < romData.length(); i += 2)
    {
        chksum += (uint16_t)((uint8_t)romData.at(i) | ((uint8_t)romData.at(i + 1) << 8));
    }
    msg.clear();
    msg.append(QString("CHKSUM: 0x%1").arg(chksum, 4, 16, QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    if (chksum != target)
    {
        qDebug() << "Checksum mismatch!";

        QByteArray balance_value_array;
        uint16_t balance_value = (uint16_t)((uint8_t)romData.at(balance_value_array_start)
                                 | ((uint8_t)romData.at(balance_value_array_start + 1) << 8));

        msg.clear();
        msg.append(QString("Balance value before: 0x%1").arg(balance_value, 4, 16, QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        balance_value += target - chksum;

        msg.clear();
        msg.append(QString("Balance value after: 0x%1").arg(balance_value, 4, 16, QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        // Little-endian: low byte first
        balance_value_array.append((uint8_t)(balance_value & 0xff));
        balance_value_array.append((uint8_t)((balance_value >> 8) & 0xff));
        romData.replace(balance_value_array_start, balance_value_array.length(), balance_value_array);

        qDebug() << "Checksums corrected";
        QMessageBox::information(nullptr, QObject::tr("Subaru RH850 DI ECU Checksum"), "Checksums corrected");
    }

    return romData;
}
