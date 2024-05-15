#include "get_key_operations_subaru.h"
#include <ui_ecu_operations.h>
#include <QRandomGenerator>

GetKeyOperationsSubaru::GetKeyOperationsSubaru(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::EcuOperationsWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("Determine Encryption Keys from Unencrypted and Encrypted Files");
    this->show();

    int result = 0;

    ui->progressbar->setValue(0);

    result = load_and_apply_linear_approx();

    if (result == STATUS_SUCCESS)
    {
        QMessageBox::information(this, tr("Get Key Operation"), "Get Key operation completed succesfully, press OK to exit");
        this->accept();
        //this->close();
    }
    else
    {
        QMessageBox::warning(this, tr("Get Key Operation"), "Get Key operation failed, press OK to exit and try again");
    }
}

GetKeyOperationsSubaru::~GetKeyOperationsSubaru()
{

}

void GetKeyOperationsSubaru::closeEvent(QCloseEvent *bar)
{
    //kill_process = true;
    //ecuOperations->kill_process = true;
}

int GetKeyOperationsSubaru::load_and_apply_linear_approx()
{
    //QFileDialog openDialog;
    QString unencryptedFilename = QFileDialog::getOpenFileName(this, tr("Select unencrypted ROM"));

    if (unencryptedFilename.isEmpty())
    {
        QMessageBox::information(this, tr("Unencrypted file"), "No file selected");
    }

    QString encryptedFilename = QFileDialog::getOpenFileName(this, tr("Select encrypted ROM"));

    if (encryptedFilename.isEmpty())
    {
        QMessageBox::information(this, tr("Encrypted file"), "No file selected");
    }

    QFile unencryptedFile(unencryptedFilename);
    if (!unencryptedFile.open(QIODevice::ReadOnly ))
    {
        QMessageBox::warning(this, tr("File"), "Unable to open file for reading");
        return STATUS_ERROR;
    }
    QByteArray unencryptedFileData = unencryptedFile.readAll();
    unencryptedFile.close();

    QFile encryptedFile(encryptedFilename);
    if (!encryptedFile.open(QIODevice::ReadOnly ))
    {
        QMessageBox::warning(this, tr("File"), "Unable to open file for reading");
        return STATUS_ERROR;
    }
    QByteArray encryptedFileData = encryptedFile.readAll();
    encryptedFile.close();

    send_log_window_message("Files loaded successfully", true, true);

    //uint16_t keys[]={
    //    0x3b61, 0x8bef, 0x9e51, 0x1075
    //};

    int inBits[][2]={
        {20, 21}, {24, 25}, {28, 29}, {32, 17}
    };

    int outBits[][2]={
        {11, 8}, {15, 12}, {3, 16}, {7, 4}
    };

    uint32_t plainText, cipherText;
    uint16_t x2Text, x3Text, k4, k1, k2, k3, x1, x4, fOutcome;
    int counter[4][0x20], nybble, subkey, j, k, total;
    bool alreadyExists[0x20000/4];

    for (nybble = 0; nybble < 4; nybble++) for (subkey = 0; subkey < 0x20; subkey++) counter[nybble][subkey] = 0;
    total = 0;

    for(j = 0; j < 0x20000; j+=4)
    {
        alreadyExists[j/4] = false;
        for (k = 0; k < j; k+=4)
        {
            alreadyExists[j/4] = (unencryptedFileData[j] == unencryptedFileData[k]) && (unencryptedFileData[j+1] == unencryptedFileData[k+1]) && (unencryptedFileData[j+2] == unencryptedFileData[k+2]) && (unencryptedFileData[j+3] == unencryptedFileData[k+3]);
            if(alreadyExists[j/4]) k = j;
        }
    }

    send_log_window_message("Start Time", true, true);
    for (j = 0; j < 0x20000; j+=4)   // 0x20000
    {
        //send_log_window_message("plaintext value = " + parse_message_to_hex(inDWord) + " ciphertext value = " + parse_message_to_hex(outDWord), true, true);

        if (!alreadyExists[j/4])
        {
            total++;
            plainText = ((unencryptedFileData[j] << 24) & 0xFF000000) + ((unencryptedFileData[j+1] << 16) & 0xFF0000) + ((unencryptedFileData[j+2] << 8) & 0xFF00) + (unencryptedFileData[j+3] & 0xFF);
            cipherText = ((encryptedFileData[j] << 24) & 0xFF000000) + ((encryptedFileData[j+1] << 16) & 0xFF0000) + ((encryptedFileData[j+2] << 8) & 0xFF00) + (encryptedFileData[j+3] & 0xFF);

            /*
            plainText = QRandomGenerator::global()->generate();
            //send_log_window_message("Plaintext: " + QString::number(plainText, 16) + " Ciphertext " + QString::number(cipherText, 16), true, true);
            cipherText = manyRoundAndFlip(plainText, keys, 4);
            cipherText = flipLeftRight(cipherText);
            //send_log_window_message("Plaintext: " + QString::number(plainText, 16) + " Ciphertext " + QString::number(cipherText, 16), true, true);
            */

            x4 = cipherText & 0xFFFF;

            for(nybble = 0; nybble < 4; nybble++)
            {

                uint8_t p = get_bit(plainText, inBits[nybble][0]) ^ get_bit(plainText, inBits[nybble][1]) ^ get_bit(plainText, outBits[nybble][0]) ^ get_bit(plainText, outBits[nybble][1]);
                uint8_t c = get_bit(cipherText, outBits[nybble][0]+16) ^ get_bit(cipherText, outBits[nybble][1]+16);

                for(subkey = 0; subkey < 0x20; subkey++)
                {

                    k4 = subkey << (12 - (nybble * 4));
                    if ((nybble == 0) && (subkey > 0xf)) k4 += 0x01;
                    fOutcome = fFunction(x4, k4);
                    x3Text = ((cipherText >> 16) & 0xffff) ^ fOutcome;
                    //send_log_window_message("plaintext = " + QString::number(plainText, 16) + " ciphertext = " + QString::number(cipherText, 16) + " x3text = " + QString::number(x3Text, 16), true, true);

                    uint8_t x3_bits = get_bit(x3Text, inBits[nybble][0]) ^ get_bit(x3Text, inBits[nybble][1]);
                    uint8_t z = p ^ c ^ x3_bits;
                    //send_log_window_message("p3:" + QString::number(p3, 16) + " p15:" + QString::number(p15, 16) + " p16:" + QString::number(p16, 16) + " p28:" + QString::number(p28, 16) + " p29:" + QString::number(p29, 16) + " c19:" + QString::number(c19, 16) + " c31:" + QString::number(c31, 16) + " c32:" + QString::number(c32, 16) + " x3_28:" + QString::number(x3_28, 16) + " x3_29:" + QString::number(x3_29, 16), true, true);

                    if (z == 0) counter[nybble][subkey]++;
                }
            }
        }
        //if ((j % 3000) == 0) send_log_window_message("Unique plaintext / ciphertext pair tested: " + QString::number(total), true, true);
    }


    int order[4][0x20];
    for(nybble = 0; nybble < 4; nybble++)
    {
        for (subkey = 0; subkey < 0x20; subkey++)
        {
            counter[nybble][subkey] = abs(counter[nybble][subkey] - total/2);
            order[nybble][subkey] = subkey;
        }
    }

    k4 = 0;

    for (nybble = 0; nybble < 4; nybble++)
    {

        //uint32_t correctKey = ((keys[3] + ((keys[3] & 0x01) << 16)) >> (12 - (nybble * 4))) & 0x1f;
        //send_log_window_message("Nybble: " + QString::number(nybble) + "   Correct key: 0x" + QString::number(correctKey, 16) + " Count: " + QString::number(counter[nybble][correctKey]), true, true);

        int tempMax, tempOrder;
        for (subkey = 1; subkey < 0x20; subkey++)
        {
            tempMax = counter[nybble][subkey];
            tempOrder = order[nybble][subkey];
            j = subkey - 1;
            while (j >= 0 && counter[nybble][j] > tempMax)
            {
                counter[nybble][j+1] = counter[nybble][j];
                order[nybble][j+1] = order[nybble][j];
                j--;
            }
            counter[nybble][j+1] = tempMax;
            order[nybble][j+1] = tempOrder;
        }

        for (j = 0x1f; j > 0x1e; j--)
        {
            //send_log_window_message("Nybble: " + QString::number(nybble) + " Predicted key: 0x" + QString::number(order[nybble][j], 16) + " Count: " + QString::number(counter[nybble][j]), true, true);
            k4 += (order[nybble][0x1f] & 0xf) << (12 - (nybble * 4));
        }

    }
    send_log_window_message("Predicted k4: 0x" + QString::number(k4, 16), true, true);
    send_log_window_message("Moving on to k1", true, true);

    // now getting k1

    for (nybble = 0; nybble < 4; nybble++) for (subkey = 0; subkey < 0x20; subkey++) counter[nybble][subkey] = 0;
    total = 0;
    for (j = 0; j < 0x20000; j+=4)   // 0x20000
    {
        if (!alreadyExists[j/4])
        {
            total++;

            plainText = ((unencryptedFileData[j] << 24) & 0xFF000000) + ((unencryptedFileData[j+1] << 16) & 0xFF0000) + ((unencryptedFileData[j+2] << 8) & 0xFF00) + (unencryptedFileData[j+3] & 0xFF);
            cipherText = ((encryptedFileData[j] << 24) & 0xFF000000) + ((encryptedFileData[j+1] << 16) & 0xFF0000) + ((encryptedFileData[j+2] << 8) & 0xFF00) + (encryptedFileData[j+3] & 0xFF);
            x4 = cipherText & 0xFFFF;
            fOutcome = fFunction(x4, k4);
            x3Text = ((cipherText >> 16) & 0xffff) ^ fOutcome;

            x1 = plainText & 0xFFFF;

            for(nybble = 0; nybble < 4; nybble++)
            {

                uint8_t p = get_bit(plainText, outBits[nybble][0]+16) ^ get_bit(plainText, outBits[nybble][1]+16);
                uint8_t c = get_bit(x3Text, outBits[nybble][0]+16) ^ get_bit(x3Text, outBits[nybble][1]+16);

                for(subkey = 0; subkey < 0x20; subkey++)
                {

                    k1 = subkey << (12 - (nybble * 4));
                    if ((nybble == 0) && (subkey > 0xf)) k1 += 0x01;
                    fOutcome = fFunction(x1, k1);
                    x2Text = ((plainText >> 16) & 0xffff) ^ fOutcome;
                    //send_log_window_message("plaintext = " + QString::number(plainText, 16) + " ciphertext = " + QString::number(cipherText, 16) + " x3text = " + QString::number(x3Text, 16), true, true);

                    uint8_t x2_bits = get_bit(x2Text, inBits[nybble][0]) ^ get_bit(x2Text, inBits[nybble][1]);
                    uint8_t z = p ^ c ^ x2_bits;
                    //send_log_window_message("p3:" + QString::number(p3, 16) + " p15:" + QString::number(p15, 16) + " p16:" + QString::number(p16, 16) + " p28:" + QString::number(p28, 16) + " p29:" + QString::number(p29, 16) + " c19:" + QString::number(c19, 16) + " c31:" + QString::number(c31, 16) + " c32:" + QString::number(c32, 16) + " x3_28:" + QString::number(x3_28, 16) + " x3_29:" + QString::number(x3_29, 16), true, true);

                    if (z == 0) counter[nybble][subkey]++;
                }
            }
        }
        //if ((j % 3000) == 0) send_log_window_message("Unique plaintext / ciphertext pair tested: " + QString::number(total), true, true);
    }


    for(nybble = 0; nybble < 4; nybble++)
    {
        for (subkey = 0; subkey < 0x20; subkey++)
        {
            counter[nybble][subkey] = abs(counter[nybble][subkey] - total/2);
            order[nybble][subkey] = subkey;
        }
    }

    k1 = 0;

    for (nybble = 0; nybble < 4; nybble++)
    {

        //uint32_t correctKey = ((keys[0] + ((keys[0] & 0x01) << 16)) >> (12 - (nybble * 4))) & 0x1f;
        //send_log_window_message("Nybble: " + QString::number(nybble) + "   Correct key: 0x" + QString::number(correctKey, 16) + " Count: " + QString::number(counter[nybble][correctKey]), true, true);

        int tempMax, tempOrder;
        for (subkey = 1; subkey < 0x20; subkey++)
        {
            tempMax = counter[nybble][subkey];
            tempOrder = order[nybble][subkey];
            j = subkey - 1;
            while (j >= 0 && counter[nybble][j] > tempMax)
            {
                counter[nybble][j+1] = counter[nybble][j];
                order[nybble][j+1] = order[nybble][j];
                j--;
            }
            counter[nybble][j+1] = tempMax;
            order[nybble][j+1] = tempOrder;
        }

        for (j = 0x1f; j > 0x1e; j--)
        {
            //send_log_window_message("Nybble: " + QString::number(nybble) + " Predicted key: 0x" + QString::number(order[nybble][j], 16) + " Count: " + QString::number(counter[nybble][j]), true, true);
            k1 += (order[nybble][0x1f] & 0xf) << (12 - (nybble * 4));
        }

    }
    send_log_window_message("Predicted k1: 0x" + QString::number(k1, 16), true, true);
    send_log_window_message("Moving on to k2", true, true);

    // k2 using last plaintext, ciphertext pair

    x2Text = ((plainText >> 16) & 0xffff) ^ fFunction(plainText & 0xffff, k1);
    x3Text = ((cipherText >> 16) & 0xffff) ^ fFunction(cipherText & 0xffff, k4);
    //send_log_window_message("Plaintext: " + QString::number(plainText, 16) + " Ciphertext: " + QString::number(cipherText, 16), true, true);
    //send_log_window_message("x2Text: " + QString::number(x2Text, 16) + " x3 Text: " + QString::number(x3Text, 16), true, true);

    for (k2 = 0; k2 < 0x10000; k2++)
    {
        if ((fFunction(x2Text, k2) ^ (plainText & 0xffff)) == x3Text) break;
    }

    send_log_window_message("Predicted k2: 0x" + QString::number(k2, 16), true, true);
    send_log_window_message("Moving on to k3", true, true);

    // k3 using last plaintext, ciphertext pair

    for (k3 = 0; k3 < 0x10000; k3++)
    {
        if ((fFunction(x3Text, k3) ^ (cipherText & 0xffff)) == x2Text) break;
    }

    send_log_window_message("Predicted k3: 0x" + QString::number(k3, 16), true, true);
    send_log_window_message("End Time", true, true);

    return STATUS_SUCCESS;

}

uint8_t GetKeyOperationsSubaru::get_bit(uint32_t value, int bit_num)
{
    return (value >> (32 - bit_num)) & 0x01;
}

int GetKeyOperationsSubaru::linear_approx_test()
{

    uint16_t cIndexOfMax, dIndexOfMax;

    uint16_t **approxTable = new uint16_t*[0x20];
    for(int i = 0; i < 0x20; i++) approxTable[i] = new uint16_t[0x10];

    uint16_t c, d;

    for (c = 0; c < 0x10; c++)
        for (d = 0; d < 0x20; d++)
                approxTable[d][c] = 0;

    findApprox(approxTable);

    for (c = 0; c < 0x10; c++)
        for (d = 0; d < 0x20; d++)
            send_log_window_message("Results of Linear Approx: ," + QString::number(approxTable[d][c]) + "," + QString::number(d) + "," + QString::number(c), true, true);

    return STATUS_SUCCESS;
}

uint16_t GetKeyOperationsSubaru::applyMask(uint16_t value, uint16_t mask)
{
    uint16_t interValue = value & mask;
    uint16_t cum = 0;
    uint16_t bitValue;

    while (interValue > 0)
    {
        bitValue = interValue % 2;
        interValue /= 2;

        cum = cum ^ bitValue;
    }
    return cum;
}

uint32_t GetKeyOperationsSubaru::manyRoundAndFlip(uint32_t input, uint16_t *keys, int numRounds)
{
    int n;
    uint32_t temp = input;

    for(n = 0; n < numRounds; n++) temp = roundAndFlip(temp, keys[n]);

    return temp;
}

uint32_t GetKeyOperationsSubaru::roundAndFlip(uint32_t input, uint16_t keyInput)
{
    uint32_t roundOutcome;

    roundOutcome = roundFunction(input, keyInput);
    roundOutcome = flipLeftRight(roundOutcome);
    return roundOutcome;
}

uint32_t GetKeyOperationsSubaru::flipLeftRight(uint32_t flipInput)
{
    return (flipInput >> 16) + (flipInput << 16);
}

uint32_t GetKeyOperationsSubaru::roundFunction(uint32_t roundInput, uint16_t keyInput)
{
    uint16_t highWord, lowWord, fBoxOutcome, highRoundOutcome, lowRoundOutcome;

    highWord = roundInput >> 16;
    lowWord = roundInput & 0xFFFF;

    fBoxOutcome = fFunction(lowWord, keyInput);

    highRoundOutcome = highWord ^ fBoxOutcome;
    lowRoundOutcome = lowWord;

    return (highRoundOutcome << 16) + lowRoundOutcome;
}

uint16_t GetKeyOperationsSubaru::fFunction(uint16_t wordInput, uint16_t keyInput)
{
    int n;
    uint16_t fBoxOutcome = 0;
    uint32_t temp;

    temp = keyInput ^ wordInput;
    temp += temp << 16;

    for (n = 0; n < 4; n++) fBoxOutcome += sBox((temp >> (n * 4)) & 0x1F) << (n * 4);

    fBoxOutcome = (fBoxOutcome >> 3) + (fBoxOutcome << 13);

    return fBoxOutcome;
}


uint16_t GetKeyOperationsSubaru::sBox(uint16_t sBoxInput)
{
    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    return indextransformation[sBoxInput];
}

void GetKeyOperationsSubaru::findApprox(uint16_t **approxTable)
{

    uint16_t c, d, e;

    for (c = 0; c < 0x10; c++)
    {
        send_log_window_message("Got to: " + QString::number(c), true, true);
        for (d = 0; d < 0x20; d++)
        {
            for (e = 0; e < 0x20; e++)
            {
                if (applyMask(e, d) ^ applyMask(sBox(e), c) == 0)
                {
                    approxTable[d][c]++;
                    //send_log_window_message(QString::number(d) + "," + QString::number(c) + "," + QString::number(approxTable[d][c]), true, true);
                }
            }
        }
    }
}

int GetKeyOperationsSubaru::send_log_window_message(QString message, bool timestamp, bool linefeed)
{
    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz']  ");

    if (timestamp)
        message = dateTimeString + message;
    if (linefeed)
        message = message + "\n";

    QTextEdit* textedit = this->findChild<QTextEdit*>("text_edit");
    if (textedit)
    {
        ui->text_edit->insertPlainText(message);
        ui->text_edit->ensureCursorVisible();

        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
}



