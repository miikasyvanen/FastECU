#include "protocol_select.h"
#include "ui_protocol_select.h"

ProtocolSelect::ProtocolSelect(FileActions::ConfigValuesStructure *configValues, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ProtocolSelect)
{
    ui->setupUi(this);
}

ProtocolSelect::~ProtocolSelect()
{
    delete ui;
}
