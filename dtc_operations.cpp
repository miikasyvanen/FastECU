#include "dtc_operations.h"

DtcOperations::DtcOperations(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DtcOperationsWindow)
    , ecuCalDef(ecuCalDef)
    , cmd_type(cmd_type)
{
    ui->setupUi(this);

    this->serial = serial;
}

DtcOperations::~DtcOperations()
{

}
