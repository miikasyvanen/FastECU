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
    QFont custom_font("Courier New", 10);
    QLabel* current_label;
    int i;

    for (i = 0; i < data_result->length(); i++) {

        current_label = ui->gridLayoutWidget->findChild< QLabel* >("Name" + QString::number(i));
        if (!current_label)
        {
            // more rows than last time (e.g. more DTCs)
            current_label = new QLabel;
            current_label->setObjectName("Name" + QString::number(i));
            current_label->setFont(custom_font);
            ui->gridLayout->addWidget(current_label, i, 0);
        }
        current_label->setText(data_result->at(i));
        current_label->show();
    }

    // fewer rows than last time: hide the old ones instead of showing stale data
    while ((current_label = ui->gridLayoutWidget->findChild< QLabel* >("Name" + QString::number(i))) != nullptr)
    {
        current_label->clear();
        current_label->hide();
        i++;
    }
}
