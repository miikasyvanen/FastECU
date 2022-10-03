#include "logvalues.h"
#include "ui_logvalues.h"

LogValues::LogValues(FileActions::LogValuesStructure *logValues, int tabIndex, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::LogValues)
{
    ui->setupUi(this);

    ui->tabWidget->setCurrentIndex(tabIndex);

    QVBoxLayout *gaugeValueLayout = new QVBoxLayout();
    QVBoxLayout *digitalValueLayout = new QVBoxLayout();
    QVBoxLayout *switchValueLayout = new QVBoxLayout();
    ui->logGaugesTab->setLayout(gaugeValueLayout);
    ui->logBoxesTab->setLayout(digitalValueLayout);
    ui->logSwitchesTab->setLayout(switchValueLayout);

    QComboBox *logValueComboBox;

    //for (int i = 0; i < logValues->logValueName.count(); i++)
        //qDebug() << logValues->logValueId.at(i) << logValues->logValueName.at(i);

    for (int i = 0; i < logValues->dashboard_log_value_id.count(); i++)
    {
        int j = 0;
        while (logValues->log_value_id.at(j) != logValues->dashboard_log_value_id.at(i))
            j++;
        QString logValueName = logValues->log_value_name.at(j);

        logValueComboBox = new QComboBox();
        //qDebug() << "Gauge index " << logValueName << "found";
        logValueComboBox->addItem(logValueName);
        gaugeValueLayout->addWidget(logValueComboBox);
    }
    for (int i = 0; i < logValues->lower_panel_log_value_id.count(); i++)
    {
        int j = 0;
        while (logValues->log_value_id.at(j) != logValues->lower_panel_log_value_id.at(i))
            j++;
        QString logValueName = logValues->log_value_name.at(j);

        logValueComboBox = new QComboBox();
        //qDebug() << "Panel index " << logValueName << "found";
        logValueComboBox->addItem(logValueName);
        digitalValueLayout->addWidget(logValueComboBox);
    }
    for (int i = 0; i < logValues->lower_panel_switch_id.count(); i++)
    {
        int j = 0;
        while (logValues->log_switch_id.at(j) != logValues->lower_panel_switch_id.at(i))
            j++;
        QString logSwitchName = logValues->log_switch_name.at(j);

        logValueComboBox = new QComboBox();
        //qDebug() << "Switch index " << logSwitchName << "found";
        logValueComboBox->addItem(logSwitchName);
        switchValueLayout->addWidget(logValueComboBox);
    }

}

LogValues::~LogValues()
{
    delete ui;
}
