#include "biu_ops_subaru_input1.h"
#include <ui_biu_ops_subaru_input1.h>
#include <QMessageBox>

BiuOpsSubaruInput1::BiuOpsSubaruInput1(QByteArray *biu_tt_result, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::BiuOpsSubaruInput1Window)
{
    ui->setupUi(this);

    this->biu_tt_result = *biu_tt_result;

    // Values the BIU reports outside a combo's range leave the combo
    // unselected (-1); sending is refused until the user picks a value, so
    // an unknown setting is never overwritten with a guess.
    ui->light_delay_combo->addItem("Normal");
    ui->light_delay_combo->addItem("Off");
    ui->light_delay_combo->addItem("Short");
    ui->light_delay_combo->addItem("Long");
    int value = this->biu_tt_result.length() > 0 ? (uint8_t)this->biu_tt_result.at(0) : -1;
    ui->light_delay_combo->setCurrentIndex(value < ui->light_delay_combo->count() ? value : -1);

    ui->autolock_combo->addItem("20");
    ui->autolock_combo->addItem("30");
    ui->autolock_combo->addItem("40");
    ui->autolock_combo->addItem("50");
    ui->autolock_combo->addItem("60");
    value = this->biu_tt_result.length() > 1 ? (uint8_t)this->biu_tt_result.at(1) - 2 : -1;
    ui->autolock_combo->setCurrentIndex(value >= 0 && value < ui->autolock_combo->count() ? value : -1);

    if (this->biu_tt_result.length() >= 3)
    {
        ui->outtemp_combo->addItem("0.0");
        ui->outtemp_combo->addItem("0.5");
        ui->outtemp_combo->addItem("1.0");
        ui->outtemp_combo->addItem("1.5");
        ui->outtemp_combo->addItem("-2.0");
        ui->outtemp_combo->addItem("-1.5");
        ui->outtemp_combo->addItem("-1.0");
        ui->outtemp_combo->addItem("-0.5");
        value = (uint8_t)this->biu_tt_result.at(2);
        ui->outtemp_combo->setCurrentIndex(value < ui->outtemp_combo->count() ? value : -1);
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
    const bool has_outtemp = biu_tt_result.length() >= 3;

    if (ui->light_delay_combo->currentIndex() < 0 || ui->autolock_combo->currentIndex() < 0 ||
        (has_outtemp && ui->outtemp_combo->currentIndex() < 0))
    {
        QMessageBox::warning(this, "BIU setting", "The BIU reported a value that is not in the list. Select a value for every setting before sending.");
        return;
    }

    output.append((char)ui->light_delay_combo->currentIndex());
    output.append((char)(ui->autolock_combo->currentIndex() + 2));
    if (has_outtemp) output.append((char)ui->outtemp_combo->currentIndex());

    emit send_biu_setting1(output);

}
