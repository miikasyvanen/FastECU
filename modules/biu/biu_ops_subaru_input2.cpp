#include "biu_ops_subaru_input2.h"
#include <ui_biu_ops_subaru_input2.h>

BiuOpsSubaruInput2::BiuOpsSubaruInput2(QStringList *biu_option_names, QByteArray *biu_option_result, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::BiuOpsSubaruInput2Window)
{
    ui->setupUi(this);

    this->biu_option_names = biu_option_names;
    this->biu_option_result = *biu_option_result;
    // 3 strings (name, set, clear) per bit, 8 bits per byte
    byte_count = qMin((int)this->biu_option_result.length(), (int)(biu_option_names->length() / 24));

    QLabel *label;
    QRadioButton *radio_button;
    QButtonGroup *button_group;
    QPushButton *send_setting;
    int bitmask, i, current_value;

    for (int byte_counter = 0; byte_counter < byte_count; byte_counter++)
    {
        bitmask = 1;

        for (int bit_counter = 0; bit_counter < 8; bit_counter++)
        {
            i = byte_counter * 8 + bit_counter;

            current_value = (uint8_t)this->biu_option_result.at(byte_counter) & bitmask;
            bitmask = bitmask << 1;

            label = new QLabel();
            label->setObjectName("Name" + QString::number(i));
            label->setText(biu_option_names->at(3 * i));
            ui->gridLayout->addWidget(label, i, 0);

            button_group = new QButtonGroup(this);
            button_group->setObjectName("Name GROUP" + QString::number(i));

            radio_button = new QRadioButton();
            radio_button->setObjectName("Name ON" + QString::number(i));
            radio_button->setText(biu_option_names->at(3 * i + 1));
            if (current_value != 0) radio_button->setChecked(true);
            button_group->addButton(radio_button, 1);
            ui->gridLayout->addWidget(radio_button, i, 1);

            radio_button = new QRadioButton();
            radio_button->setObjectName("Name OFF" + QString::number(i));
            radio_button->setText(biu_option_names->at(3 * i + 2));
            if (current_value == 0) radio_button->setChecked(true);
            button_group->addButton(radio_button, 0);
            ui->gridLayout->addWidget(radio_button, i, 2);
        }

    }

    send_setting = new QPushButton();
    send_setting->setObjectName("Name Send");
    send_setting->setText("Send to BIU");
    ui->gridLayout->addWidget(send_setting, byte_count * 8 + 1, 2);

    connect(send_setting, SIGNAL(clicked(bool)), this, SLOT(prepare_biu_setting2()));


}

BiuOpsSubaruInput2::~BiuOpsSubaruInput2()
{
    delete ui;
}

void BiuOpsSubaruInput2::prepare_biu_setting2()
{
    // Start from the bytes read from the BIU so any bit without a radio
    // button keeps its current value.
    QByteArray output = biu_option_result.left(byte_count);
    QRadioButton* current_button;
    int i, bitmask;

    for (int byte_counter = 0; byte_counter < byte_count; byte_counter++)
    {
        bitmask = 1;

        for (int bit_counter = 0; bit_counter < 8; bit_counter++)
        {
            i = byte_counter * 8 + bit_counter;
            current_button = ui->gridLayoutWidget->findChild< QRadioButton* >("Name ON" + QString::number(i));
            if (current_button == nullptr)
            {
                bitmask = bitmask << 1;
                continue;
            }
            if (current_button->isChecked()) output[byte_counter] = output[byte_counter] | bitmask;
            else output[byte_counter] = output[byte_counter] & ~bitmask;
            bitmask = bitmask << 1;
        }
    }

    emit send_biu_setting2(output);
}
