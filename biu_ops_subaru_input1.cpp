#include "biu_ops_subaru_input1.h"
#include <ui_biu_ops_subaru_input1.h>

BiuOpsSubaruInput1::BiuOpsSubaruInput1(QByteArray *biu_tt_result, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::BiuOpsSubaruInput1Window)
{
    ui->setupUi(this);

    this->biu_tt_result = biu_tt_result;

    ui->light_delay_combo->addItem("Normal");
    ui->light_delay_combo->addItem("Off");
    ui->light_delay_combo->addItem("Short");
    ui->light_delay_combo->addItem("Long");
    ui->light_delay_combo->setCurrentIndex(biu_tt_result->at(0));

    ui->autolock_combo->addItem("20");
    ui->autolock_combo->addItem("30");
    ui->autolock_combo->addItem("40");
    ui->autolock_combo->addItem("50");
    ui->autolock_combo->addItem("60");
    ui->autolock_combo->setCurrentIndex(biu_tt_result->at(1) - 2);

    if (biu_tt_result->length() == 3)
    {
        ui->outtemp_combo->addItem("0.0");
        ui->outtemp_combo->addItem("0.5");
        ui->outtemp_combo->addItem("1.0");
        ui->outtemp_combo->addItem("1.5");
        ui->outtemp_combo->addItem("-2.0");
        ui->outtemp_combo->addItem("-1.5");
        ui->outtemp_combo->addItem("-1.0");
        ui->outtemp_combo->addItem("-0.5");
        ui->outtemp_combo->setCurrentIndex(biu_tt_result->at(2));
    }
    else
    {
        ui->outtemp_combo->hide();
        ui->label_3->hide();
    }


    connect(ui->send_setting, SIGNAL(clicked(bool)), this, SLOT(prepare_biu_setting1()));

}

BiuOpsSubaruInput1::~BiuOpsSubaruInput1()
{
    delete ui;
}

void BiuOpsSubaruInput1::prepare_biu_setting1()
{
    QByteArray output;

    output.append(ui->light_delay_combo->currentIndex());
    output.append(ui->autolock_combo->currentIndex() + 2);
    if(biu_tt_result->length() == 3) output.append(ui->outtemp_combo->currentIndex());

    emit send_biu_setting1(output);

}
