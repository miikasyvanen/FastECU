#include "biu_ops_subaru_dtcs.h"
#include <ui_biu_ops_subaru_dtcs.h>

BiuOpsSubaruDtcs::BiuOpsSubaruDtcs(QStringList *dtc_result, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::BiuOpsSubaruDtcsWindow)
{
    ui->setupUi(this);

    this->dtc_result = dtc_result;

    QLabel *label;

    for (int i = 0; i < dtc_result->length(); i++)
    {
        label = new QLabel;
        label->setText(dtc_result->at(i));
        ui->gridLayout->addWidget(label, i, 0);
    }


}

BiuOpsSubaruDtcs::~BiuOpsSubaruDtcs()
{
    delete ui;
}

void BiuOpsSubaruDtcs::closeEvent(QCloseEvent *event)
{
    qDebug() << "Closing BIU DTC window";
}

