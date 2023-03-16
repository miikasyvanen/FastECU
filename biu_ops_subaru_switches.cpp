#include "biu_ops_subaru_switches.h"
#include <ui_biu_ops_subaru_switches.h>

BiuOpsSubaruSwitches::BiuOpsSubaruSwitches(QStringList *switch_result, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::BiuOpsSubaruSwitchesWindow)

{
    ui->setupUi(this);
    this->setParent(parent);
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->switch_result = switch_result;

    QFont custom_font("Courier New", 7);

    QLabel *label;

    int row_num, col_num;

    col_num = 0;
    row_num = 0;

    for (int i = 0; i < 128; i++) {
        if ((i == 40) || (i == 72))
        {
            col_num += 2;
            row_num = 0;
        }
        label = new QLabel();
        label->setObjectName("Name" + QString::number(i));
        label->setFont(custom_font);
        label->setText(switch_result->at(2 * i));
        ui->gridLayout->addWidget(label, row_num, col_num);

        label = new QLabel();
        label->setObjectName("Result" + QString::number(i));
        label->setFont(custom_font);
        label->setText(switch_result->at(2 * i + 1));
        label->setAlignment(Qt::AlignCenter);
        if (switch_result->at(2 * i + 1) == " ON") label->setStyleSheet("QLabel { background-color : green; color : white;}");
        else label->setStyleSheet("QLabel { background-color : red; color : white;}");
        ui->gridLayout->addWidget(label, row_num, col_num + 1);

        row_num++;
    }

}

BiuOpsSubaruSwitches::~BiuOpsSubaruSwitches()
{
    this->close();
    delete ui;
}

void BiuOpsSubaruSwitches::update_switch_results(QStringList *switch_result)
{

    QLabel* current_label;

    for (int i = 0; i < 128; i++) {

        current_label = ui->gridLayoutWidget->findChild< QLabel* >("Result" + QString::number(i));

        if (current_label)
        {
            current_label->setText(switch_result->at(2 * i + 1));
            if (switch_result->at(2 * i + 1) == " ON") current_label->setStyleSheet("QLabel { background-color : green; color : white;}");
            else current_label->setStyleSheet("QLabel { background-color : red; color : white;}");
            //current_label->repaint();
        }
    }
}

