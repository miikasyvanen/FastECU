#ifndef CARMODELSELECT_H
#define CARMODELSELECT_H

//#include <QDesktopWidget>
#include <QWidget>
#include <QStringListModel>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QCheckBox>
#include <QScreen>

#include "file_actions.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class VehicleSelect;
}
QT_END_NAMESPACE

class VehicleSelect : public QDialog
{
    Q_OBJECT

public:
    explicit VehicleSelect(FileActions::ConfigValuesStructure *configValues, QWidget *parent = nullptr);
    ~VehicleSelect();

private:

    QFont font;
    int font_size = 10;
    bool font_bold = false;
    QString font_family = "Franklin Gothic";
    int header_font_size = 10;
    bool header_font_bold = false;
    QString header_font_family = "Franklin Gothic";

    QString flash_protocol_id;
    QString flash_protocol_mcu;
    QString flash_protocol_make;
    QString flash_protocol_model;
    QString flash_protocol_version;
    QString flash_protocol_type;
    QString flash_protocol_kw;
    QString flash_protocol_hp;
    QString flash_protocol_fuel;
    QString flash_protocol_year;
    QString flash_protocol_ecu;
    QString flash_protocol_mode;
    QString flash_protocol_checksum;
    QString flash_protocol_read;
    QString flash_protocol_write;
    QString flash_protocol_flash_protocol;
    QString flash_protocol_log_protocol;
    QString flash_protocol_comms_protocol;
    QString flash_protocol_description;
    QString flash_protocol_family;

    FileActions::ConfigValuesStructure *configValues;
    Ui::VehicleSelect *ui;


private slots:
    void car_model_selected();
    void car_make_treewidget_item_selected();
    void car_model_treewidget_item_selected();
    void car_version_treewidget_item_selected();

};

#endif // CARMODELSELECT_H
