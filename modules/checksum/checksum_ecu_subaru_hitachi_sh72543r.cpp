#include "checksum_ecu_subaru_hitachi_sh72543r.h"

ChecksumEcuSubaruHitachiSh72543r::ChecksumEcuSubaruHitachiSh72543r()
{

}

ChecksumEcuSubaruHitachiSh72543r::~ChecksumEcuSubaruHitachiSh72543r()
{

}

QByteArray ChecksumEcuSubaruHitachiSh72543r::calculate_checksum(QByteArray romData)
{
    /*******************
     *
     * Big-endian 16-bit word sum from 0x6000 to the end of flash must equal
     * 0x5AA5; the correction (balance) word is the last word of flash. The boot
     * block (0x0000-0x5FFF) is excluded. No code reads the balance word
     * directly - it only balances the sum (boot block PTR_DAT_000b446c).
     *
     * One routine covers both SH725xx image sizes - the end of flash is taken
     * from the image size:
     *   SH72543R  2   MB  end 0x200000  balance 0x1FFFFE
     *   SH72544R  2.5 MB  end 0x280000  balance 0x27FFFE
     * Only those two sizes are accepted; anything else is left unchanged so a
     * mis-selected flash method cannot scribble on a partial image.
     *
     * Markers the boot block validates (word 0x6000 = 0x5555, word end-0xE =
     * 0xAAAA, word 0x006C = word 0x6010) and the application-side reference
     * words at end-0x20 are NOT touched - only the balance word is rewritten.
     *
     ******************/

    QByteArray msg;
    const int flash_end = romData.length();
    if (flash_end != 0x200000 && flash_end != 0x280000)
        return romData;   // not a 2 MB / 2.5 MB SH725xx image - leave untouched

    uint16_t chksum = 0;

    for (int i = 0x6000; i < flash_end; i+=2)
    {
        chksum += ((uint8_t)romData.at(i) << 8) + (uint8_t)romData.at(i + 1);
    }
    msg.clear();
    msg.append(QString("CHKSUM: 0x%1").arg(chksum,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    if (chksum != 0x5aa5)
    {
        qDebug() << "Checksum mismatch!";

        QByteArray balance_value_array;
        uint32_t balance_value_array_start = flash_end - 2;
        uint16_t balance_value = ((uint8_t)romData.at(balance_value_array_start) << 8) + ((uint8_t)romData.at(balance_value_array_start + 1));

        msg.clear();
        msg.append(QString("Balance value before: 0x%1").arg(balance_value,4,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        balance_value += 0x5aa5 - chksum;

        msg.clear();
        msg.append(QString("Balance value after: 0x%1").arg(balance_value,4,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        balance_value_array.append((uint8_t)((balance_value >> 8) & 0xff));
        balance_value_array.append((uint8_t)(balance_value & 0xff));
        romData.replace(balance_value_array_start, balance_value_array.length(), balance_value_array);

        qDebug() << "Checksums corrected";
        QMessageBox::information(nullptr, QObject::tr("Subaru Hitachi SH725xx ECU Checksum"), "Checksums corrected");
    }

    return romData;
}
