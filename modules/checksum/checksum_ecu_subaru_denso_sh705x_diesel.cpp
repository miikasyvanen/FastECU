#include "checksum_ecu_subaru_denso_sh705x_diesel.h"

ChecksumEcuSubaruDensoSH705xDiesel::ChecksumEcuSubaruDensoSH705xDiesel()
{

}

ChecksumEcuSubaruDensoSH705xDiesel::~ChecksumEcuSubaruDensoSH705xDiesel()
{

}

QByteArray ChecksumEcuSubaruDensoSH705xDiesel::calculate_checksum(QByteArray romData, uint32_t checksum_area_start, uint32_t checksum_area_length)
{
    QByteArray checksum_array;

    uint32_t checksum_area_end = checksum_area_start + checksum_area_length;
    uint32_t checksum_dword_addr_lo = 0;
    uint32_t checksum_dword_addr_hi = 0;
    uint32_t checksum = 0;
    uint32_t checksum_temp = 0;
    uint32_t checksum_diff = 0;
    uint32_t checksum_check = 0;
    uint8_t checksum_block = 0;

    bool checksum_ok = true;
    bool sh72543_checksum_ok = true;


    for (uint32_t i = checksum_area_start; i < checksum_area_end; i+=12)
    {
        checksum = 0;
        checksum_temp = 0;
        checksum_dword_addr_lo = 0;
        checksum_dword_addr_hi = 0;
        checksum_diff = 0;

        for (int j = 0; j < 4; j++)
        {
            checksum_dword_addr_lo = (checksum_dword_addr_lo << 8) + (uint8_t)romData.at(i + j);
            checksum_dword_addr_hi = (checksum_dword_addr_hi << 8) + (uint8_t)romData.at(i + 4 + j);
            checksum_diff = (checksum_diff << 8) + (uint8_t)romData.at(i + 8 + j);
        }
        if (i == checksum_area_start && checksum_dword_addr_lo == 0 && checksum_dword_addr_hi == 0 && checksum_diff == 0x5aa5a55a)
        {
            qDebug() << "ROM has all checksums disabled";
            QMessageBox::information(nullptr, QObject::tr("32-bit checksum"), "ROM has all checksums disabled");
            return 0;
        }

        if (checksum_dword_addr_lo == 0 && checksum_dword_addr_hi == 0 && checksum_diff == 0x5aa5a55a)
        {
            //QMessageBox::information(nullptr, QObject::tr("32-bit checksum"), "Checksums disabled");
        }
        if (checksum_dword_addr_lo != 0 && checksum_dword_addr_hi != 0 && checksum_diff != 0x5aa5a55a)
        {
            for (uint32_t j = checksum_dword_addr_lo; j < checksum_dword_addr_hi; j+=4)
            {
                for (int k = 0; k < 4; k++)
                {
                    checksum_temp = (checksum_temp << 8) + (uint8_t)(romData.at(j + k));
                }
                if (checksum_area_start == 0x0FFB80 && j == 0x0FFAFC)
                    checksum += 0xFFFFFFFF;
                else if (checksum_area_start == 0x17FB80 && j == 0x17FAFC)
                    checksum += 0xFFFFFFFF;
                else
                    checksum += checksum_temp;
            }
        }
        checksum_check = 0x5aa5a55a - checksum;

        if (checksum_diff == checksum_check)
        {
            qDebug() << "Checksum block " + QString::number(checksum_block) + " OK";
        }
        else
        {
            qDebug() << "Checksum block " + QString::number(checksum_block) + " NOK";
            checksum_ok = false;
        }

        checksum_array.append(checksum_dword_addr_lo >> 24);
        checksum_array.append(checksum_dword_addr_lo >> 16);
        checksum_array.append(checksum_dword_addr_lo >> 8);
        checksum_array.append(checksum_dword_addr_lo);
        checksum_array.append(checksum_dword_addr_hi >> 24);
        checksum_array.append(checksum_dword_addr_hi >> 16);
        checksum_array.append(checksum_dword_addr_hi >> 8);
        checksum_array.append(checksum_dword_addr_hi);
        checksum_array.append(checksum_check >> 24);
        checksum_array.append(checksum_check >> 16);
        checksum_array.append(checksum_check >> 8);
        checksum_array.append(checksum_check);

        checksum_block++;
    }

    if (!checksum_ok)
    {
        romData.replace(checksum_area_start, checksum_area_length, checksum_array);
        qDebug() << "Checksums corrected";
    }

    // Check SH72543 EURO6 Diesel additional checksums
    if (checksum_area_start == 0x1FF800)
    {
        checksum_array.clear();
        checksum_area_start = 0x001FF8E8;
        checksum_area_end = checksum_area_start + (2 * 12);

        for (uint32_t i = checksum_area_start; i < checksum_area_end; i+=12)
        {
            checksum = 0;
            checksum_temp = 0;
            checksum_dword_addr_lo = 0;
            checksum_dword_addr_hi = 0;
            checksum_diff = 0;

            for (int j = 0; j < 4; j++)
            {
                checksum_dword_addr_lo = (checksum_dword_addr_lo << 8) + (uint8_t)romData.at(i + j);
                checksum_dword_addr_hi = (checksum_dword_addr_hi << 8) + (uint8_t)romData.at(i + 4 + j);
                checksum_diff = (checksum_diff << 8) + (uint8_t)romData.at(i + 8 + j);
            }

            if (checksum_dword_addr_lo != 0 && checksum_dword_addr_hi != 0 && checksum_diff != 0x5aa5a55a)
            {
                for (uint32_t j = checksum_dword_addr_lo; j < checksum_dword_addr_hi; j+=4)
                {
                    for (int k = 0; k < 4; k++)
                    {
                        checksum_temp = (checksum_temp << 8) + (uint8_t)(romData.at(j + k));
                    }
                    checksum += checksum_temp;
                }
            }
            checksum_check = 0x5aa5a55a - checksum;

            if (checksum_diff == checksum_check)
            {
                qDebug() << "Checksum block " + QString::number(checksum_block) + " OK";
            }
            else
            {
                qDebug() << "Checksum block " + QString::number(checksum_block) + " NOK";
                sh72543_checksum_ok = false;
            }

            checksum_array.append(checksum_dword_addr_lo >> 24);
            checksum_array.append(checksum_dword_addr_lo >> 16);
            checksum_array.append(checksum_dword_addr_lo >> 8);
            checksum_array.append(checksum_dword_addr_lo);
            checksum_array.append(checksum_dword_addr_hi >> 24);
            checksum_array.append(checksum_dword_addr_hi >> 16);
            checksum_array.append(checksum_dword_addr_hi >> 8);
            checksum_array.append(checksum_dword_addr_hi);
            checksum_array.append(checksum_check >> 24);
            checksum_array.append(checksum_check >> 16);
            checksum_array.append(checksum_check >> 8);
            checksum_array.append(checksum_check);

            checksum_block++;
        }

        if (!sh72543_checksum_ok)
        {
            romData.replace(checksum_area_start, checksum_area_length, checksum_array);
            qDebug() << "SH72543 EURO6 additional checksums corrected";
            QMessageBox::information(nullptr, QObject::tr("Subaru Denso SH705x Checksum"), "Checksums corrected");
        }
    }
    if (!checksum_ok || !sh72543_checksum_ok)
    {
        QMessageBox::information(nullptr, QObject::tr("Subaru Denso SH705x Checksum"), "Checksums corrected");
    }
    return romData;
}

