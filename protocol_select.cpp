#include "protocol_select.h"
#include "ui_protocol_select.h"

ProtocolSelect::ProtocolSelect(FileActions::ConfigValuesStructure *configValues, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::ProtocolSelectWindow)
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
    QStringList car_version_tree_widget_headers = {"Version","Type","kW","HP","Fuel","Year","ECU","Mode","CHK","RD","WR","Family"};
    ui->car_version_tree_widget->setHeaderLabels(car_version_tree_widget_headers);
    ui->car_version_tree_widget->setColumnWidth(0, 175);
    ui->car_version_tree_widget->setColumnWidth(1, 125);
    ui->car_version_tree_widget->setColumnWidth(2, 50);
    ui->car_version_tree_widget->setColumnWidth(3, 50);
    ui->car_version_tree_widget->setColumnWidth(4, 50);
    ui->car_version_tree_widget->setColumnWidth(5, 50);
    ui->car_version_tree_widget->setColumnWidth(6, 175);
    ui->car_version_tree_widget->setColumnWidth(7, 50);
    ui->car_version_tree_widget->setColumnWidth(8, 40);
    ui->car_version_tree_widget->setColumnWidth(9, 40);
    ui->car_version_tree_widget->setColumnWidth(10, 40);
    ui->car_version_tree_widget->setColumnWidth(11, 125);

    int width = 0;
    for (int i = 0; i < ui->car_version_tree_widget->columnCount(); i++)
        width += ui->car_version_tree_widget->columnWidth(i);

    qDebug() << "Full width =" << width;
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
    configValues->flash_protocol_selected_make = configValues->flash_protocol_make.at(index);
    configValues->flash_protocol_selected_model = configValues->flash_protocol_model.at(index);
    configValues->flash_protocol_selected_version = configValues->flash_protocol_version.at(index);
    configValues->flash_protocol_selected_family = configValues->flash_protocol_family.at(index);
    configValues->flash_protocol_selected_description = configValues->flash_protocol_description.at(index);

    QStringList car_makes;
    QStringList car_makes_sorted;

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
            ui->car_make_tree_widget->setCurrentItem(item);
        else if (i == 0)
            ui->car_make_tree_widget->setCurrentItem(item);
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

ProtocolSelect::~ProtocolSelect()
{
    reject();
}

void ProtocolSelect::car_model_selected()
{
    configValues->flash_protocol_selected_id = flash_protocol_id;
    configValues->flash_protocol_selected_make = flash_protocol_make;
    configValues->flash_protocol_selected_model = flash_protocol_model;
    configValues->flash_protocol_selected_version = flash_protocol_version;
    configValues->flash_protocol_selected_family = flash_protocol_family;
    configValues->flash_protocol_selected_description = flash_protocol_description;
    configValues->flash_protocol_selected_flash_transport = configValues->flash_protocol_flash_transport.at(configValues->flash_protocol_selected_id.toInt());
    configValues->flash_protocol_selected_log_transport = configValues->flash_protocol_log_transport.at(configValues->flash_protocol_selected_id.toInt());
    configValues->flash_protocol_selected_log_protocol = configValues->flash_protocol_log_protocol.at(configValues->flash_protocol_selected_id.toInt());

    qDebug() << "Selected car model:";
    qDebug() << configValues->flash_protocol_id.at(configValues->flash_protocol_selected_id.toInt());
    qDebug() << configValues->flash_protocol_make.at(configValues->flash_protocol_selected_id.toInt());
    qDebug() << configValues->flash_protocol_model.at(configValues->flash_protocol_selected_id.toInt());
    qDebug() << configValues->flash_protocol_version.at(configValues->flash_protocol_selected_id.toInt());
    qDebug() << configValues->flash_protocol_family.at(configValues->flash_protocol_selected_id.toInt());
    qDebug() << configValues->flash_protocol_description.at(configValues->flash_protocol_selected_id.toInt());

    accept();

    close();
}

void ProtocolSelect::car_make_treewidget_item_selected()
{
    QTreeWidgetItem *item = ui->car_make_tree_widget->selectedItems().at(0);
    if (item)//ui->car_make_tree_widget->topLevelItemCount())
    {
        QTreeWidgetItem *item = ui->car_make_tree_widget->selectedItems().at(0);
        QString selected_text = item->text(0);

        qDebug() << "Check models for manufacturer:" << selected_text;

        QString car_make = selected_text;
        flash_protocol_id.clear();
        flash_protocol_make = car_make;
        flash_protocol_model.clear();
        flash_protocol_version.clear();

        QStringList car_models;
        QStringList car_models_sorted;

        ui->car_model_tree_widget->clear();

        for (int i = 0; i < configValues->flash_protocol_id.length(); i++)
        {
            if (!car_models.contains(configValues->flash_protocol_model.at(i)) && configValues->flash_protocol_make.at(i) == car_make && configValues->flash_protocol_model.at(i) != "")
            {
                //qDebug() << "Model found:" << configValues->flash_protocol_model.at(i);
                car_models.append(configValues->flash_protocol_model.at(i));
            }
        }
        car_models_sorted = car_models;
        sort(car_models_sorted.begin(), car_models_sorted.end(), less<QString>());
        for (int i = 0; i < car_models_sorted.length(); i++)
        {
            QTreeWidgetItem *item = new QTreeWidgetItem();
            item->setText(0, car_models_sorted.at(i));
            item->setFirstColumnSpanned(true);
            ui->car_model_tree_widget->addTopLevelItem(item);
            if (car_models_sorted.at(i) == configValues->flash_protocol_selected_model)
                ui->car_model_tree_widget->setCurrentItem(item);
            else if (i == 0)
                ui->car_model_tree_widget->setCurrentItem(item);
        }
    }
}

void ProtocolSelect::car_model_treewidget_item_selected()
{
    QTreeWidgetItem *item = ui->car_model_tree_widget->selectedItems().at(0);
    if (item)//ui->car_model_tree_widget->topLevelItemCount())
    {
        QTreeWidgetItem *item = ui->car_model_tree_widget->selectedItems().at(0);
        QString selected_text = item->text(0);

        qDebug() << "Check versions for model:" << selected_text;

        QString car_model = selected_text;
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
        QStringList mode;
        QStringList checksum;
        QStringList read;
        QStringList write;
        QStringList family;
        QStringList description;

        ui->car_version_tree_widget->clear();

        for (int i = 0; i < configValues->flash_protocol_id.length(); i++)
        {
            if (configValues->flash_protocol_model.at(i) == car_model && configValues->flash_protocol_make.at(i) == flash_protocol_make)
            {
                qDebug() << "Model found:" << configValues->flash_protocol_model.at(i);
                id.append(configValues->flash_protocol_id.at(i));
                version.append(configValues->flash_protocol_version.at(i));
                type.append(configValues->flash_protocol_type.at(i));
                kw.append(configValues->flash_protocol_kw.at(i));
                hp.append(configValues->flash_protocol_hp.at(i));
                fuel.append(configValues->flash_protocol_fuel.at(i));
                year.append(configValues->flash_protocol_year.at(i));
                ecu.append(configValues->flash_protocol_ecu.at(i));
                mode.append(configValues->flash_protocol_mode.at(i));
                checksum.append(configValues->flash_protocol_checksum.at(i));
                read.append(configValues->flash_protocol_read.at(i));
                write.append(configValues->flash_protocol_write.at(i));
                family.append(configValues->flash_protocol_family.at(i));
                description.append(configValues->flash_protocol_description.at(i));
            }
        }
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
            if (checksum.at(i) == "yes")
                item->setCheckState(8, Qt::Checked);
            else if (checksum.at(i) == "no")
                item->setCheckState(8, Qt::Unchecked);
            else
                item->setText(8, "n/a");
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

            if (id.at(i) == configValues->flash_protocol_selected_id)
                ui->car_version_tree_widget->setCurrentItem(item);
            else if (i == 0)
                ui->car_version_tree_widget->setCurrentItem(item);

        }
    }
}

void ProtocolSelect::car_version_treewidget_item_selected()
{
    QTreeWidgetItem *item = ui->car_version_tree_widget->selectedItems().at(0);
    if (item)//ui->car_version_tree_widget->topLevelItemCount())
    {
        QTreeWidgetItem *item = ui->car_version_tree_widget->selectedItems().at(0);
        QString selected_text = item->text(0);

        qDebug() << "Selected version for model" << flash_protocol_model << "is" << selected_text;

        ui->select_button->setEnabled(true);

        QString car_version = selected_text;
        flash_protocol_version.clear();
        flash_protocol_version = car_version;
        flash_protocol_family = item->text(11);
        flash_protocol_id = item->text(12);
        flash_protocol_description = item->text(13);

        //flash_protocol_id = selected_text;//car_version_treewidget_item->text(11);

        qDebug() << "Protocol ID:" << flash_protocol_id;
    }
}
