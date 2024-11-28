#include "protocol_select.h"
#include "ui_protocol_select.h"

ProtocolSelect::ProtocolSelect(FileActions::ConfigValuesStructure *configValues, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ProtocolSelect)
    , configValues(configValues)
{
    ui->setupUi(this);

    //ui->select_button->setEnabled(false);

    QStringList tree_widget_headers = {"Model","Protocol"};
    ui->treeWidget->setHeaderLabels(tree_widget_headers);
    ui->treeWidget->setFont(font);

    QRect  screenGeometry = this->geometry();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);

    //ui->treeWidget->setColumnWidth(0, 150);
    //ui->treeWidget->setColumnWidth(1, 75);

    QStringList protocols;
    QStringList protocols_sorted;
    QStringList descriptions_sorted;
    bool protocol_changed_saved = false;
    QFontMetrics fm(font);
    int text_width = 0;
    int protocol_width = 0;
    int description_width = 0;

    for (int i = 0; i < configValues->flash_protocol_id.length(); i++)
    {
        if (!protocols.contains(configValues->flash_protocol_protocol_name.at(i)))
        {
            //qDebug() << "Make found:" << configValues->flash_protocol_make.at(i);
            protocols.append(configValues->flash_protocol_protocol_name.at(i));
        }
    }

    protocols_sorted = protocols;
    sort(protocols_sorted.begin(), protocols_sorted.end(), less<QString>());

    for (int i = 0; i < protocols_sorted.length(); i++)
    {
        for (int j = 0; j < configValues->flash_protocol_id.length(); j++)
        {
            if (protocols_sorted.at(i) == configValues->flash_protocol_protocol_name.at(j))
            {
                qDebug() << protocols_sorted.at(i) << configValues->flash_protocol_protocol_name.at(j);
                descriptions_sorted.append(configValues->flash_protocol_description.at(j));
                text_width = fm.horizontalAdvance(descriptions_sorted.at(i));
                if (text_width > description_width)
                    description_width = text_width;

                break;
            }
        }

        text_width = fm.horizontalAdvance(protocols_sorted.at(i));
        if (text_width > protocol_width)
            protocol_width = text_width;
    }
    protocol_width += 20;
    description_width += 20;
    ui->treeWidget->setColumnWidth(0, protocol_width);
    ui->treeWidget->setColumnWidth(1, description_width);
    ui->treeWidget->setFixedWidth(protocol_width + description_width + 20);

    for (int i = 0; i < protocols_sorted.length(); i++)
    {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, protocols_sorted.at(i));
        item->setText(1, descriptions_sorted.at(i));
        item->setFirstColumnSpanned(true);
        ui->treeWidget->addTopLevelItem(item);
        if (protocols_sorted.at(i) == configValues->flash_protocol_selected_protocol_name)
        {
            protocol_changed_saved = true;
            ui->treeWidget->setCurrentItem(item);
        }
    }
    if (!protocol_changed_saved)
    {
        qDebug() << "Protocol changed to first item, no protocol selected previously";
        QTreeWidgetItem *item = ui->treeWidget->topLevelItem(0);
        ui->treeWidget->setCurrentItem(item);
    }

    connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &ProtocolSelect::protocol_treewidget_item_selected);
    connect(ui->treeWidget, &QTreeWidget::doubleClicked, this, &ProtocolSelect::car_model_selected);
    connect(ui->cancel_button, &QPushButton::clicked, this, &QDialog::close);
    connect(ui->select_button, &QPushButton::clicked, this, &ProtocolSelect::car_model_selected);

}

ProtocolSelect::~ProtocolSelect()
{
    delete(ui);
}

void ProtocolSelect::car_model_selected()
{
    QString protocol_name = ui->treeWidget->selectedItems().at(0)->text(0);
    qDebug() << "Selected protocol:" << protocol_name;

    for (int i = 0; i < configValues->flash_protocol_id.length(); i++)
    {
        if (configValues->flash_protocol_protocol_name.at(i) == protocol_name)
        {
            configValues->flash_protocol_selected_id = configValues->flash_protocol_id.at(i);
            configValues->flash_protocol_selected_make = configValues->flash_protocol_make.at(i);
            configValues->flash_protocol_selected_model = configValues->flash_protocol_model.at(i);
            configValues->flash_protocol_selected_version = configValues->flash_protocol_version.at(i);
            configValues->flash_protocol_selected_protocol_name = configValues->flash_protocol_protocol_name.at(i);
            configValues->flash_protocol_selected_description = ui->treeWidget->selectedItems().at(0)->text(1);
            configValues->flash_protocol_selected_log_protocol = configValues->flash_protocol_log_protocol.at(i);
            configValues->flash_protocol_selected_mcu = configValues->flash_protocol_mcu.at(i);
            configValues->flash_protocol_selected_checksum = configValues->flash_protocol_checksum.at(i);
        }
    }

    qDebug() << "Selected MCU:" << configValues->flash_protocol_selected_mcu;
    accept();

    close();
}

void ProtocolSelect::protocol_treewidget_item_selected()
{
    QTreeWidgetItem *item = ui->treeWidget->selectedItems().at(0);
    if (item)
    {
        QTreeWidgetItem *item = ui->treeWidget->selectedItems().at(0);
        QString selected_text = item->text(0);

        //ui->select_button->setEnabled(true);

    }
    qDebug() << "protocol selection applied";
}
