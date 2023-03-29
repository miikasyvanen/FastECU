#include "biu_ops_subaru_data.h"
#include <ui_biu_ops_subaru_data.h>

BiuOpsSubaruData::BiuOpsSubaruData(QStringList *data_result, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::BiuOpsSubaruDataWindow)
{
    ui->setupUi(this);

    this->data_result = data_result;

    QFont custom_font("Courier New", 10);

    QLabel *label;

    for (int i = 0; i < data_result->length(); i++)
    {
        label = new QLabel;
        label->setObjectName("Name" + QString::number(i));
        label->setFont(custom_font);
        label->setText(data_result->at(i));
        ui->gridLayout->addWidget(label, i, 0);
    }


}

BiuOpsSubaruData::~BiuOpsSubaruData()
{
    delete ui;
}

void BiuOpsSubaruData::update_data_results(QStringList *data_result)
{

    QLabel* current_label;

    for (int i = 0; i < data_result->length(); i++) {

        current_label = ui->gridLayoutWidget->findChild< QLabel* >("Name" + QString::number(i));
        if (current_label) current_label->setText(data_result->at(i));

    }
}
