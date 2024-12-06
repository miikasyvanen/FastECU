#include "calibration_treewidget.h"

CalibrationTreeWidget::CalibrationTreeWidget()
{

}

QTreeWidget *CalibrationTreeWidget::buildCalibrationFilesTree(int ecuCalDefIndex, QTreeWidget *filesTreeWidget, FileActions::EcuCalDefStructure *ecuCalDef)
{
    QString filename(ecuCalDef->FileName);

    /**************************
     * Create files tree
     *************************/
    QTreeWidget *calFilesTree = filesTreeWidget;
    calFilesTree->setAnimated(true);
    calFilesTree->setFocusPolicy(Qt::NoFocus);
    //calFilesTree->setStyleSheet("QListView::item:selected {background : transparent; border: solid 2px red;}");
    QFont filesItemFont;
    filesItemFont.setPixelSize(14);
    calFilesTree->setFont(filesItemFont);

    QTreeWidgetItem *topLevelFilesTreeItem = new QTreeWidgetItem();
    topLevelFilesTreeItem->setText(2, QString::number(ecuCalDefIndex));
    topLevelFilesTreeItem->setCheckState(0, Qt::Unchecked);
    topLevelFilesTreeItem->setFirstColumnSpanned(true);
    calFilesTree->addTopLevelItem(topLevelFilesTreeItem);

    topLevelFilesTreeItem->setText(0, filename);
    if (ecuCalDef->IdList.length())
        topLevelFilesTreeItem->setText(1, ecuCalDef->IdList.at(0));
    calFilesTree->expandItem(topLevelFilesTreeItem);

    for (int i = 0; i < calFilesTree->topLevelItemCount(); i++)
    {
        calFilesTree->topLevelItem(i)->setCheckState(0, Qt::Unchecked);
        calFilesTree->topLevelItem(i)->setSelected(false);
    }
    topLevelFilesTreeItem->setSelected(true);
    topLevelFilesTreeItem->setCheckState(0, Qt::Checked);

    return calFilesTree;
}

QTreeWidget *CalibrationTreeWidget::buildCalibrationDataTree(QTreeWidget *dataTreeWidget, FileActions::EcuCalDefStructure *ecuCalDef)
{
    bool treeChildCreated = false;

    /**************************
     * Create data tree
     *************************/
    dataTreeWidget->clear();

    QTreeWidget *calDataTree = dataTreeWidget;

    calDataTree->setAnimated(true);
    calDataTree->setFocusPolicy(Qt::NoFocus);

    QFont dataItemFont;
    dataItemFont.setPixelSize(12);
    calDataTree->setFont(dataItemFont);

    // If OEM ecu file, add ROM info
    if (ecuCalDef->RomInfo.length())
    {
        QTreeWidgetItem * topLevelDataTreeItem = new QTreeWidgetItem();
        topLevelDataTreeItem->setText(0, "ROM Info");
        calDataTree->addTopLevelItem(topLevelDataTreeItem);
        if (ecuCalDef->RomInfoExpanded == "1")
            topLevelDataTreeItem->setExpanded(true);

        for (int i = 0; i < ecuCalDef->RomInfo.length(); i++)
        {
            QTreeWidgetItem * item = new QTreeWidgetItem();
            item->setText(0, ecuCalDef->RomInfoStrings.at(i) + ": " + ecuCalDef->RomInfo.at(i));
            calDataTree->topLevelItem(0)->addChild(item);
        }
    }

    for (int j = 0; j < ecuCalDef->NameList.count(); j++)
    {
        if (ecuCalDef->CategoryList[j] != "" && ecuCalDef->CategoryList[j] != " ")
        {
            if (ecuCalDef->NameList[j] != "")
            {
                treeChildCreated = false;
                for (int i = 0; i < calDataTree->topLevelItemCount(); i++){
                    if (calDataTree->topLevelItem(i)->text(0) == ecuCalDef->CategoryList[j]){
                        treeChildCreated = true;
                    }
                }
                if (!treeChildCreated)
                {
                    QTreeWidgetItem * topLevelDataTreeItem = new QTreeWidgetItem();
                    topLevelDataTreeItem->setText(0, ecuCalDef->CategoryList[j]);
                    calDataTree->addTopLevelItem(topLevelDataTreeItem);
                    treeChildCreated = false;
                    if (ecuCalDef->CategoryExpandedList.at(j) == "1")
                        topLevelDataTreeItem->setExpanded(true);
                }
                for (int i = 0; i < calDataTree->topLevelItemCount(); i++){
                    if (calDataTree->topLevelItem(i)->text(0) == ecuCalDef->CategoryList[j]){
                        QTreeWidgetItem * item = new QTreeWidgetItem();
                        if (ecuCalDef->TypeList[j] == "1D" || ecuCalDef->TypeList[j] == "Selectable" || (ecuCalDef->YSizeList.at(j).toInt() == 1 && ecuCalDef->XSizeList.at(j).toInt() == 1))
                            item->setIcon(0, QIcon(":/icons/1D-64.png"));
                        else if (ecuCalDef->TypeList[j] == "2D")
                            item->setIcon(0, QIcon(":/icons/2D-64.png"));
                        else if (ecuCalDef->TypeList[j] == "3D")
                            item->setIcon(0, QIcon(":/icons/3D-64.png"));
                        if (ecuCalDef->VisibleList.at(j) == "1")
                            item ->setCheckState(0,Qt::Checked);
                        else
                            item ->setCheckState(0,Qt::Unchecked);
                        item->setText(0, ecuCalDef->NameList[j]);
                        item->setText(1, QString::number(j));
                        calDataTree->topLevelItem(i)->addChild(item);
                        item->setToolTip(0, ecuCalDef->NameList[j] + ecuCalDef->DescriptionList[j]);
                    }
                }
            }
        }
    }

    return calDataTree;
}

void *CalibrationTreeWidget::calibrationDataTreeWidgetItemExpanded(FileActions::EcuCalDefStructure *ecuCalDef, QString categoryName)
{
    for (int i = 0; i < ecuCalDef->CategoryList.count(); i++)
    {
        if (ecuCalDef->CategoryList.at(i) == categoryName)
            ecuCalDef->CategoryExpandedList.replace(i, "1");
    }
    if (categoryName == "ROM Info")
        ecuCalDef->RomInfoExpanded = "1";

    return NULL;
}

void *CalibrationTreeWidget::calibrationDataTreeWidgetItemCollapsed(FileActions::EcuCalDefStructure *ecuCalDef, QString categoryName)
{
    for (int i = 0; i < ecuCalDef->CategoryList.count(); i++)
    {
        if (ecuCalDef->CategoryList.at(i) == categoryName)
            ecuCalDef->CategoryExpandedList.replace(i, "0");
    }
    if (categoryName == "ROM Info")
        ecuCalDef->RomInfoExpanded = "0";

    return NULL;
}
