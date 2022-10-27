#ifndef CALIBRATION_MAPS_H
#define CALIBRATION_MAPS_H

#include <QMainWindow>
#include <QDebug>
#include <QWidget>
#include <QString>
#include <QMdiSubWindow>
#include <QComboBox>
#include <QCheckBox>

#include <file_actions.h>
#include <verticallabel.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class CalibrationMaps;
}
QT_END_NAMESPACE

class CalibrationMaps : public QWidget
{
    Q_OBJECT

public:
    explicit CalibrationMaps(FileActions::EcuCalDefStructure *ecuCalDef, int romIndex, int mapIndex, QRect mdiAreaSize, QWidget *parent = nullptr);
    ~CalibrationMaps();

    int mapCellWidthSelectable = 240;
    int mapCellWidth1D = 96;
    int mapCellWidth = 54;
    int mapCellHeight = 26;
    int cellFontSize = mapCellHeight / 2.35;

    int startCol = 0;
    int startRow = 0;
    int xSize = 0;
    int ySize = 0;
    int xSizeOffset = 0;
    int ySizeOffset = 0;

private:
    enum RomInfoEnum {
        XmlId,
        InternalIdAddress,
        Make,
        Model,
        SubModel,
        Market,
        Transmission,
        Year,
        EcuId,
        InternalIdString,
        MemModel,
        ChecksumModule,
        RomBase,
        FlashMethod,
        FileSize,
    };

    void setMapTableWidgetSize(int maxWidth, int maxHeight, int sizeX);
    void setMapTableWidgetItems(FileActions::EcuCalDefStructure *ecuCalDef, int mapIndex);
    int getMapValueDecimalCount(QString valueFormat);
    int getMapCellColors(FileActions::EcuCalDefStructure *ecuCalDef, float mapDataValue, int mapIndex);

private slots:
    //void fetchFromEcu();
    //void storeToEcu();
    void cellClicked(int row, int col);
    void cellPressed(int row, int col);
    void cellChanged(int curRow, int curCol, int prevRow, int prevCol);

signals:
    void fetchFromEcuButtonClicked();
    void storeToEcuButtonClicked();
    void selectable_combobox_item_changed(QString);
    void checkbox_state_changed(int);

private:
    Ui::CalibrationMaps *ui;
};

#endif // CALIBRATION_MAPS_H
