#include "ecu_operations_manual.h"
#include <ui_ecu_manual_operations.h>

EcuManualOperations::EcuManualOperations(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::EcuManualOperationsWindow)
{
    ui->setupUi(this);
    this->setParent(parent);
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowTitle(ecuCalDef->FileName);
    this->show();
    this->serial = serial;

    mcu_type_string = ecuCalDef->RomInfo.at(MemModel);

    add_mcu_type_combobox();
    add_kernels_combobox();
    add_flash_methods_combobox();

}

EcuManualOperations::~EcuManualOperations()
{

}

void EcuManualOperations::add_mcu_type_combobox()
{
    int index = 0;
    int mcu_type_index = 0;

    while (flashdevices[index].name != 0)
    {
        ui->mcu_type->addItem(flashdevices[index].name);
        index++;
    }
    while (flashdevices[mcu_type_index].name != 0)
    {
        qDebug() << flashdevices[mcu_type_index].name << mcu_type_string;
        if (flashdevices[mcu_type_index].name == mcu_type_string)
            break;
        mcu_type_index++;
    }

    ui->mcu_type->setCurrentIndex(mcu_type_index);
}

void EcuManualOperations::add_kernels_combobox()
{
    int index = 0;

    while (index < kernels.length())
    {
        ui->kernels->addItem(kernels.at(index));
        index++;
    }
}

void EcuManualOperations::add_flash_methods_combobox()
{
    int index = 0;

    while (index < flash_methods.length())
    {
        ui->flash_methods->addItem(flash_methods.at(index));
        index++;
    }
}

void EcuManualOperations::delay(int n)
{
    QTime dieTime = QTime::currentTime().addMSecs(n);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}
