#ifndef CALIBRATION_TREEWIDGET_H
#define CALIBRATION_TREEWIDGET_H

#include <QFile>
#include <QFileInfo>
#include <QTreeWidget>
#include <QLabel>
#include <QPushButton>

#include <file_actions.h>

class CalibrationTreeWidget : public QWidget
{
    Q_OBJECT

signals:
    void LOG_E(QString message, bool timestamp, bool linefeed);
    void LOG_W(QString message, bool timestamp, bool linefeed);
    void LOG_I(QString message, bool timestamp, bool linefeed);
    void LOG_D(QString message, bool timestamp, bool linefeed);

public:
    CalibrationTreeWidget();

    QTreeWidget *buildCalibrationFilesTree(int ecuCalDefIndex, QTreeWidget *filesTreeWidget, FileActions::EcuCalDefStructure *ecuCalDef);
    QTreeWidget *buildCalibrationDataTree(QTreeWidget *dataTreeWidget, FileActions::EcuCalDefStructure *ecuCalDef);
    void *calibrationDataTreeWidgetItemExpanded(FileActions::EcuCalDefStructure *ecuCalDef, QString categoryName);
    void *calibrationDataTreeWidgetItemCollapsed(FileActions::EcuCalDefStructure *ecuCalDef, QString categoryName);
/*
    QStringList RomInfoStrings = {
        "XmlId",
        "InternalIdAddress",
        "Make",
        "Model",
        "Submodel",
        "Market",
        "Transmission",
        "Year",
        "ECU ID",
        "Internal ID",
        "Memory Model",
        "Checksum Module",
        "Rom Base",
        "Flash Method",
        "File Size",
        "Def File",
    };

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
        DefFile,
    };
*/
signals:
    void closeRom();

private:

private slots:

};

#endif // CALIBRATION_TREEWIDGET_H
