#include "vehicle_select.h"
#include "ui_vehicle_select.h"

VehicleSelect::VehicleSelect(FileActions::ConfigValuesStructure *configValues, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::VehicleSelectWindow)
{
    ui->setupUi(this);
    this->setParent(parent);
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect  screenGeometry = screen->geometry();
    this->move(screenGeometry.center() - this->rect().center());
    //this->show();

    this->configValues = configValues;
    ui->select_button->setEnabled(false);

    ui->car_make_tree_widget->setHeaderLabel("Manufacturer");
    ui->car_model_tree_widget->setHeaderLabel("Model");
    QStringList car_version_tree_widget_headers = {"Version","Type","kW","HP","Fuel","Year","ECU","Mode","CHK","RD","WR","Protocol"};
    ui->car_version_tree_widget->setHeaderLabels(car_version_tree_widget_headers);
    ui->car_version_tree_widget->setColumnWidth(0, 150);
    ui->car_version_tree_widget->setColumnWidth(1, 75);
    ui->car_version_tree_widget->setColumnWidth(2, 50);
    ui->car_version_tree_widget->setColumnWidth(3, 50);
    ui->car_version_tree_widget->setColumnWidth(4, 50);
    ui->car_version_tree_widget->setColumnWidth(5, 50);
    ui->car_version_tree_widget->setColumnWidth(6, 150);
    ui->car_version_tree_widget->setColumnWidth(7, 50);
    ui->car_version_tree_widget->setColumnWidth(8, 40);
    ui->car_version_tree_widget->setColumnWidth(9, 40);
    ui->car_version_tree_widget->setColumnWidth(10, 40);
    ui->car_version_tree_widget->setColumnWidth(11, 225);

    int width = 0;
    for (int i = 0; i < ui->car_version_tree_widget->columnCount(); i++)
        width += ui->car_version_tree_widget->columnWidth(i);

    //qDebug() << "Full width =" << width;
    ui->car_version_tree_widget->setFixedWidth(width);

    int height = width / 4 * 2.5 + 18;
    this->setFixedHeight(height);

    font = ui->car_make_tree_widget->font();
    font.setPointSize(header_font_size);
    font.setBold(header_font_bold);
    font.setFamily(header_font_family);
    ui->car_make_tree_widget->setFont(font);
    ui->car_model_tree_widget->setFont(font);
    ui->car_version_tree_widget->setFont(font);

    int index = configValues->flash_protocol_selected_id.toInt();
    configValues->flash_protocol_selected_id = configValues->flash_protocol_selected_id;
    configValues->flash_protocol_selected_mcu = configValues->flash_protocol_mcu.at(index);
    configValues->flash_protocol_selected_make = configValues->flash_protocol_make.at(index);
    configValues->flash_protocol_selected_model = configValues->flash_protocol_model.at(index);
    configValues->flash_protocol_selected_version = configValues->flash_protocol_version.at(index);
    configValues->flash_protocol_selected_family = configValues->flash_protocol_family.at(index);
    configValues->flash_protocol_selected_description = configValues->flash_protocol_description.at(index);

    QStringList car_makes;
    QStringList car_makes_sorted;
    bool car_make_changed_saved = false;

    for (int i = 0; i < configValues->flash_protocol_id.length(); i++)
    {
        if (!car_makes.contains(configValues->flash_protocol_make.at(i)))
        {
            //qDebug() << "Make found:" << configValues->flash_protocol_make.at(i);
            car_makes.append(configValues->flash_protocol_make.at(i));
        }
    }

    car_makes_sorted = car_makes;
    sort(car_makes_sorted.begin(), car_makes_sorted.end(), less<QString>());
    for (int i = 0; i < car_makes_sorted.length(); i++)
    {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        font = item->font(0);
        font.setPointSize(font_size);
        font.setBold(font_bold);
        font.setFamily(font_family);
        item->setFont(0, font);
        item->setText(0, car_makes_sorted.at(i));
        item->setFirstColumnSpanned(true);
        ui->car_make_tree_widget->addTopLevelItem(item);
        if (car_makes_sorted.at(i) == configValues->flash_protocol_selected_make)
        {
            car_make_changed_saved = true;
            ui->car_make_tree_widget->setCurrentItem(item);
        }
    }
    if (!car_make_changed_saved)
    {
        qDebug() << "Car make changed to first item, no make selected previously";
        QTreeWidgetItem *item = ui->car_model_tree_widget->topLevelItem(0);
        ui->car_model_tree_widget->setCurrentItem(item);
    }

    connect(ui->car_make_tree_widget, SIGNAL(itemSelectionChanged()), this, SLOT(car_make_treewidget_item_selected()));
    connect(ui->car_model_tree_widget, SIGNAL(itemSelectionChanged()), this, SLOT(car_model_treewidget_item_selected()));
    connect(ui->car_version_tree_widget, SIGNAL(itemSelectionChanged()), this, SLOT(car_version_treewidget_item_selected()));
    connect(ui->cancel_button, SIGNAL(clicked(bool)), this, SLOT(close()));
    connect(ui->select_button, SIGNAL(clicked(bool)), this, SLOT(car_model_selected()));
    //connect(ui->select_button, SIGNAL(clicked()), this, SLOT(accept()));
    //connect(ui->cancel_button, SIGNAL(clicked()), this, SLOT(reject()));

    const QModelIndex make_index = ui->car_make_tree_widget->selectionModel()->currentIndex();
    ui->car_make_tree_widget->setCurrentIndex(make_index);
    emit ui->car_make_tree_widget->itemSelectionChanged();

/*
    const QModelIndex model_index = ui->car_model_tree_widget->selectionModel()->currentIndex();
    ui->car_model_tree_widget->setCurrentIndex(model_index);
    emit ui->car_model_tree_widget->itemSelectionChanged();

    const QModelIndex version_index = ui->car_version_tree_widget->selectionModel()->currentIndex();
    ui->car_version_tree_widget->setCurrentIndex(version_index);
    emit ui->car_version_tree_widget->itemSelectionChanged();
*/
}

VehicleSelect::~VehicleSelect()
{
    reject();
}

void VehicleSelect::car_model_selected()
{
    configValues->flash_protocol_selected_id = flash_protocol_id;
    configValues->flash_protocol_selected_make = flash_protocol_make;
    configValues->flash_protocol_selected_model = flash_protocol_model;
    configValues->flash_protocol_selected_version = flash_protocol_version;
    configValues->flash_protocol_selected_family = flash_protocol_family;
    configValues->flash_protocol_selected_description = flash_protocol_description;
    configValues->flash_protocol_selected_log_protocol = configValues->flash_protocol_log_protocol.at(configValues->flash_protocol_selected_id.toInt());
    configValues->flash_protocol_selected_mcu = configValues->flash_protocol_mcu.at(configValues->flash_protocol_selected_id.toInt());
    configValues->flash_protocol_selected_checksum = configValues->flash_protocol_checksum.at(configValues->flash_protocol_selected_id.toInt());

    qDebug() << "Selected MCU:" << configValues->flash_protocol_selected_mcu;
    accept();

    close();
}

void VehicleSelect::car_make_treewidget_item_selected()
{
    qDebug() << "Car make selection changed";
    QTreeWidgetItem *item = ui->car_make_tree_widget->selectedItems().at(0);
    if (item)
    {
        QTreeWidgetItem *item = ui->car_make_tree_widget->selectedItems().at(0);
        QString selected_text = item->text(0);

        //qDebug() << "Check models for manufacturer:" << selected_text;

        QString car_make = selected_text;
        flash_protocol_id.clear();
        flash_protocol_make = car_make;
        flash_protocol_model.clear();
        flash_protocol_version.clear();

        QStringList car_models;
        QStringList car_models_sorted;
        bool car_model_changed_saved = false;

        // To delete treewidget items, disconnect itemSelectionChanged() signal first
        disconnect(ui->car_model_tree_widget, SIGNAL(itemSelectionChanged()), 0, 0);
        qDebug() << "Delete car_model_tree_widget items";
        int item_count = ui->car_model_tree_widget->topLevelItemCount();
        for (int i = 0; i < item_count; i++)
        {
            QTreeWidgetItem *item = ui->car_model_tree_widget->topLevelItem(0);
            delete item;
        }
        // Connect itemSelectionChanged() signal again
        connect(ui->car_model_tree_widget, SIGNAL(itemSelectionChanged()), this, SLOT(car_model_treewidget_item_selected()));

        qDebug() << "Add models data based on selected make";
        for (int i = 0; i < configValues->flash_protocol_id.length(); i++)
        {
            if (!car_models.contains(configValues->flash_protocol_model.at(i)) && configValues->flash_protocol_make.at(i) == car_make && configValues->flash_protocol_model.at(i) != "")
            {
                //qDebug() << "Model found:" << configValues->flash_protocol_model.at(i);
                car_models.append(configValues->flash_protocol_model.at(i));
            }
        }
        qDebug() << "Sort models data items alphabetically";
        car_models_sorted = car_models;
        sort(car_models_sorted.begin(), car_models_sorted.end(), less<QString>());

        qDebug() << "Add models data items to select";
        for (int i = 0; i < car_models_sorted.length(); i++)
        {
            QTreeWidgetItem *item = new QTreeWidgetItem();
            item->setText(0, car_models_sorted.at(i));
            item->setFirstColumnSpanned(true);
            ui->car_model_tree_widget->addTopLevelItem(item);
            if (car_models_sorted.at(i) == configValues->flash_protocol_selected_model)
            {
                qDebug() << "Car model changed to saved model";
                car_model_changed_saved = true;
                ui->car_model_tree_widget->setCurrentItem(item);
            }
        }
        if (!car_model_changed_saved)
        {
            qDebug() << "Car model changed to first item, no model selected previously";
            QTreeWidgetItem *item = ui->car_model_tree_widget->topLevelItem(0);
            ui->car_model_tree_widget->setCurrentItem(item);
        }
    }
    qDebug() << "Car make selection applied";
}

void VehicleSelect::car_model_treewidget_item_selected()
{
    qDebug() << "Car model selection changed";
    QTreeWidgetItem *item = ui->car_model_tree_widget->selectedItems().at(0);
    if (item)
    {
        QTreeWidgetItem *item = ui->car_model_tree_widget->selectedItems().at(0);
        QString selected_text = item->text(0);

        //qDebug() << "Check versions for model:" << selected_text;

        QString car_model = selected_text;
        QStringList car_versions;
        QStringList car_versions_sorted;
        bool car_version_changed_saved = false;

        flash_protocol_id.clear();
        flash_protocol_model = car_model;
        flash_protocol_version.clear();

        QStringList id;
        QStringList version;
        QStringList type;
        QStringList kw;
        QStringList hp;
        QStringList fuel;
        QStringList year;
        QStringList ecu;
        QStringList mcu;
        QStringList mode;
        QStringList checksum;
        QStringList read;
        QStringList write;
        QStringList family;
        QStringList description;

        // To delete treewidget items, disconnect itemSelectionChanged() signal first
        disconnect(ui->car_version_tree_widget, SIGNAL(itemSelectionChanged()), 0, 0);
        qDebug() << "Delete car_version_tree_widget items";
        int item_count = ui->car_version_tree_widget->topLevelItemCount();
        for (int i = 0; i < item_count; i++)
        {
            QTreeWidgetItem *item = ui->car_version_tree_widget->topLevelItem(0);
            delete item;
        }
        // Connect itemSelectionChanged() signal again
        connect(ui->car_version_tree_widget, SIGNAL(itemSelectionChanged()), this, SLOT(car_version_treewidget_item_selected()));

        qDebug() << "Add versions data based on selected model";
        for (int i = 0; i < configValues->flash_protocol_id.length(); i++)
        {
            if (configValues->flash_protocol_model.at(i) == car_model && configValues->flash_protocol_make.at(i) == flash_protocol_make)
            {
                //qDebug() << "Model found:" << configValues->flash_protocol_model.at(i);
                id.append(configValues->flash_protocol_id.at(i));
                version.append(configValues->flash_protocol_version.at(i));
                type.append(configValues->flash_protocol_type.at(i));
                kw.append(configValues->flash_protocol_kw.at(i));
                hp.append(configValues->flash_protocol_hp.at(i));
                fuel.append(configValues->flash_protocol_fuel.at(i));
                year.append(configValues->flash_protocol_year.at(i));
                ecu.append(configValues->flash_protocol_ecu.at(i));
                mcu.append(configValues->flash_protocol_mcu.at(i));
                mode.append(configValues->flash_protocol_mode.at(i));
                checksum.append(configValues->flash_protocol_checksum.at(i));
                read.append(configValues->flash_protocol_read.at(i));
                write.append(configValues->flash_protocol_write.at(i));
                family.append(configValues->flash_protocol_family.at(i));
                description.append(configValues->flash_protocol_description.at(i));
            }
        }

        qDebug() << "Add versions data items to select";
        for (int i = 0; i < version.length(); i++)
        {
            QTreeWidgetItem *item = new QTreeWidgetItem();

            item->setText(0, version.at(i));
            item->setText(1, type.at(i));
            item->setText(2, kw.at(i));
            item->setText(3, hp.at(i));
            item->setText(4, fuel.at(i));
            item->setText(5, year.at(i));
            item->setText(6, ecu.at(i));
            item->setText(7, mode.at(i));
            if (checksum.at(i) == "yes") {
                item->setCheckState(8, Qt::Checked);
                item->setForeground(8, Qt::darkGreen);
                item->setToolTip(8, "Checksum calculation supported");
            }
            else if (checksum.at(i) == "no") {
                item->setCheckState(8, Qt::Unchecked);
                item->setForeground(8, Qt::gray);
                item->setToolTip(8, "ROM has no checksum");
            }
            else if (checksum.at(i) == "n/a") {
                item->setCheckState(8, Qt::Checked);
                item->setForeground(8, Qt::red);
                item->setToolTip(8, "Checksum calculation NOT supported yet");
            }
            if (read.at(i) == "yes")
                item->setCheckState(9, Qt::Checked);
            else
                item->setCheckState(9, Qt::Unchecked);
            if (write.at(i) == "yes")
                item->setCheckState(10, Qt::Checked);
            else
                item->setCheckState(10, Qt::Unchecked);

            item->setText(11, family.at(i));
            item->setToolTip(11, family.at(i));
            item->setText(12, id.at(i));
            item->setText(13, description.at(i));
            //topLevelCarVersionTreeItem->setFirstColumnSpanned(true);
            ui->car_version_tree_widget->addTopLevelItem(item);

            qDebug() << "Check if car version selected";
            if (id.at(i) == configValues->flash_protocol_selected_id)
            {
                qDebug() << "Car version changed to saved model";
                car_version_changed_saved = true;
                ui->car_version_tree_widget->setCurrentItem(item);
            }
        }
        if (!car_version_changed_saved)
        {
            qDebug() << "Car version changed to first item, no version selected previously";
            QTreeWidgetItem *item = ui->car_version_tree_widget->topLevelItem(0);
            ui->car_version_tree_widget->setCurrentItem(item);
        }
    }
    qDebug() << "Car model selection applied";
}

void VehicleSelect::car_version_treewidget_item_selected()
{
    QTreeWidgetItem *item = ui->car_version_tree_widget->selectedItems().at(0);
    if (item)
    {
        QTreeWidgetItem *item = ui->car_version_tree_widget->selectedItems().at(0);
        QString selected_text = item->text(0);

        //qDebug() << "Selected version for model" << flash_protocol_model << "is" << selected_text;

        ui->select_button->setEnabled(true);

        QString car_version = selected_text;
        flash_protocol_version.clear();
        flash_protocol_version = car_version;
        flash_protocol_family = item->text(11);
        flash_protocol_id = item->text(12);
        flash_protocol_description = item->text(13);

        //flash_protocol_id = selected_text;//car_version_treewidget_item->text(11);

        //qDebug() << "Protocol ID:" << flash_protocol_id;
    }
    qDebug() << "Car version selection applied";
}
