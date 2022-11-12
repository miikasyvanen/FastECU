#include "mainwindow.h"
#include "ui_mainwindow.h"

void MainWindow::change_log_values(int tab_index, QString protocol)
{
    QDialog *change_log_values_dialog = new QDialog;
    QVBoxLayout *main_layout = new QVBoxLayout();
    QTabWidget *tab_widget = new QTabWidget();
    QSpacerItem *button_spacer = new QSpacerItem(1,1, QSizePolicy::Expanding, QSizePolicy::Fixed);

    QHBoxLayout *button_layout = new QHBoxLayout();
    QPushButton *button_ok = new QPushButton("Ok");
    QPushButton *button_cancel = new QPushButton("Cancel");
    button_layout->addSpacerItem(button_spacer);
    button_layout->addWidget(button_ok);
    button_layout->addWidget(button_cancel);

    connect(button_ok, SIGNAL(clicked()), change_log_values_dialog, SLOT(close()));
    connect(button_cancel, SIGNAL(clicked()), change_log_values_dialog, SLOT(close()));

    change_log_values_dialog->setLayout(main_layout);
    main_layout->addWidget(tab_widget);
    main_layout->addLayout(button_layout);

    QWidget *gauge_widget = new QWidget();
    QWidget *digital_widget = new QWidget();
    QWidget *switch_widget = new QWidget();
    QGridLayout *gauge_value_layout = new QGridLayout();
    QGridLayout *digital_value_layout = new QGridLayout();
    QGridLayout *switch_value_layout = new QGridLayout();
    gauge_widget->setLayout(gauge_value_layout);
    digital_widget->setLayout(digital_value_layout);
    switch_widget->setLayout(switch_value_layout);

    tab_widget->addTab(gauge_widget, "Gauges");
    tab_widget->addTab(digital_widget, "Digital");
    tab_widget->addTab(switch_widget, "Switches");

    QLabel *title_label;
    QLabel *log_value_label;
    QComboBox *log_value_combobox;
    QComboBox *log_units_combobox;
    QLineEdit *log_value_title_text;

    logValues->log_values_names_sorted.clear();
    for (int i = 0; i < logValues->log_value_id.count(); i++)
    {
        if (logValues->log_value_protocol.at(i) == protocol && logValues->log_value_enabled.at(i) == "1")
        {
            logValues->log_values_names_sorted.append(logValues->log_value_name.at(i));
        }
    }
    sort(logValues->log_values_names_sorted.begin(), logValues->log_values_names_sorted.end(), less<QString>());

    for (int i = 0; i < logValues->dashboard_log_value_id.length(); i++)
    {
        log_value_label = new QLabel("Gauge " + QString::number(i + 1));
        log_value_combobox = new QComboBox();
        log_units_combobox = new QComboBox();
        log_value_title_text = new QLineEdit();

        log_value_combobox->setObjectName("Gauge value " + QString::number(i));
        log_value_combobox->addItems(logValues->log_values_names_sorted);
        log_units_combobox->setObjectName("Gauge unit " + QString::number(i));
        log_value_title_text->setObjectName("Gauge title " + QString::number(i));

        gauge_value_layout->addWidget(log_value_label, i + 1, 0);
        gauge_value_layout->addWidget(log_value_combobox, i + 1, 1);
        gauge_value_layout->addWidget(log_units_combobox, i + 1, 2);
        gauge_value_layout->addWidget(log_value_title_text, i + 1, 3);

        for (int j = 0; j < logValues->log_value_id.length(); j++)
        {
            if (logValues->log_value_protocol.at(j) == protocol)
            {
                if (logValues->dashboard_log_value_id.at(i) == logValues->log_value_id.at(j))
                {
                    QStringList units = logValues->log_value_units.at(j).split(",");
                    //qDebug() << "Units:" << units;
                    for (int k = 1; k < units.length(); k += 7)
                    {
                        log_units_combobox->addItem(units.at(k));
                    }
                    for (int k = 0; k < logValues->log_values_names_sorted.length(); k++)
                    {
                        if (logValues->log_values_names_sorted.at(k) == logValues->log_value_name.at(j))
                        {
                            log_value_combobox->setCurrentIndex(k);
                            break;
                        }
                    }
                    break;
                }
            }
        }
        connect(log_value_combobox, SIGNAL(currentIndexChanged(int)), this, SLOT(change_log_gauge_value(int)));
    }

    for (int i = 0; i < logValues->lower_panel_log_value_id.length(); i++)
    {
        log_value_label = new QLabel("Digital " + QString::number(i + 1));
        log_value_combobox = new QComboBox();
        log_units_combobox = new QComboBox();
        log_value_title_text = new QLineEdit();

        log_value_combobox->setObjectName("Digital value " + QString::number(i));
        log_value_combobox->addItems(logValues->log_values_names_sorted);
        log_units_combobox->setObjectName("Digital unit " + QString::number(i));
        log_value_title_text->setObjectName("Digital title " + QString::number(i));

        digital_value_layout->addWidget(log_value_label, i, 0);
        digital_value_layout->addWidget(log_value_combobox, i, 1);
        digital_value_layout->addWidget(log_units_combobox, i, 2);
        digital_value_layout->addWidget(log_value_title_text, i, 3);

        for (int j = 0; j < logValues->log_value_id.length(); j++)
        {
            if (logValues->log_value_protocol.at(j) == protocol)
            {
                if (logValues->lower_panel_log_value_id.at(i) == logValues->log_value_id.at(j))
                {
                    QStringList units = logValues->log_value_units.at(j).split(",");
                    //qDebug() << "Units:" << units;
                    for (int k = 1; k < units.length(); k += 7)
                    {
                        log_units_combobox->addItem(units.at(k));
                    }
                    for (int k = 0; k < logValues->log_values_names_sorted.length(); k++)
                    {
                        if (logValues->log_values_names_sorted.at(k) == logValues->log_value_name.at(j))
                        {
                            log_value_combobox->setCurrentIndex(k);
                            break;
                        }
                    }
                    break;
                }
            }
        }
        connect(log_value_combobox, SIGNAL(currentIndexChanged(int)), this, SLOT(change_log_digital_value(int)));
    }

    logValues->log_switches_names_sorted.clear();
    for (int i = 0; i < logValues->log_switch_id.count(); i++)
    {
        if (logValues->log_switch_protocol.at(i) == "SSM" && logValues->log_switch_enabled.at(i) == "1")
        {
            logValues->log_switches_names_sorted.append(logValues->log_switch_name.at(i));
        }
    }
    sort(logValues->log_switches_names_sorted.begin(), logValues->log_switches_names_sorted.end(), less<QString>());

    for (int i = 0; i < logValues->lower_panel_switch_id.length(); i++)
    {
        log_value_label = new QLabel("Switch " + QString::number(i + 1));
        log_value_combobox = new QComboBox();
        log_value_title_text = new QLineEdit();

        log_value_combobox->setObjectName("Switch value " + QString::number(i));
        log_value_combobox->addItems(logValues->log_switches_names_sorted);
        log_value_title_text->setObjectName("Switch title " + QString::number(i));

        switch_value_layout->addWidget(log_value_label, i, 0);
        switch_value_layout->addWidget(log_value_combobox, i, 1);
        switch_value_layout->addWidget(log_value_title_text, i, 2);

        for (int j = 0; j < logValues->log_switch_id.length(); j++)
        {
            if (logValues->log_switch_protocol.at(j) == protocol)
            {
                if (logValues->lower_panel_switch_id.at(i) == logValues->log_switch_id.at(j))
                {
                    for (int k = 0; k < logValues->log_switches_names_sorted.length(); k++)
                    {
                        if (logValues->log_switches_names_sorted.at(k) == logValues->log_switch_name.at(j))
                        {
                            log_value_combobox->setCurrentIndex(k);
                        }
                    }
                }
            }
        }
        connect(log_value_combobox, SIGNAL(currentIndexChanged(int)), this, SLOT(change_log_switch_value(int)));
    }

    change_log_values_dialog->exec();
}

void MainWindow::change_log_gauge_value(int index)
{
    QObject* obj = sender();
    QStringList comboIndex = obj->objectName().split(" ");
    QComboBox *log_gauge_box;
    log_gauge_box = obj->parent()->findChild<QComboBox*>(obj->objectName());

    if (log_gauge_box)
    {
        for (int i = 0; i < logValues->log_value_name.length(); i++)
        {
            if (logValues->log_value_protocol.at(i) == protocol)
            {
                if (logValues->log_value_name.at(i) == log_gauge_box->currentText())
                {
                    logValues->dashboard_log_value_id.replace(comboIndex.at(2).toUInt(), logValues->log_value_id.at(i));
                }
            }
        }
        update_logboxes(protocol);
        fileActions->read_logger_conf(logValues, ecuid, true);
    }
}

void MainWindow::change_log_digital_value(int index)
{
    QObject* obj = sender();
    QStringList comboIndex = obj->objectName().split(" ");
    QComboBox *log_digital_box;
    log_digital_box = obj->parent()->findChild<QComboBox*>(obj->objectName());

    if (log_digital_box)
    {
        for (int i = 0; i < logValues->log_value_name.length(); i++)
        {
            if (logValues->log_value_protocol.at(i) == protocol)
            {
                if (logValues->log_value_name.at(i) == log_digital_box->currentText())
                {
                    qDebug() << "Change value at" << comboIndex.at(2) << "to" << logValues->log_value_id.at(i);
                    logValues->lower_panel_log_value_id.replace(comboIndex.at(2).toUInt(), logValues->log_value_id.at(i));
                }
            }
        }
        qDebug() << "Update logboxes";
        update_logboxes(protocol);
        qDebug() << "Update logfile";
        fileActions->read_logger_conf(logValues, ecuid, true);
        qDebug() << "Done";
    }
}

void MainWindow::change_log_switch_value(int index)
{
    QObject* obj = sender();
    QStringList comboIndex = obj->objectName().split(" ");
    QComboBox *log_switch_box;
    log_switch_box = obj->parent()->findChild<QComboBox*>(obj->objectName());

    if (log_switch_box)
    {
        for (int i = 0; i < logValues->log_switch_name.length(); i++)
        {
            if (logValues->log_switch_protocol.at(i) == protocol)
            {
                if (logValues->log_switch_name.at(i) == log_switch_box->currentText())
                {
                    logValues->lower_panel_switch_id.replace(comboIndex.at(2).toUInt(), logValues->log_switch_id.at(i));
                }
            }
        }
        update_logboxes(protocol);
        fileActions->read_logger_conf(logValues, ecuid, true);
    }
}

