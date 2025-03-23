#include "calibration_maps.h"
#include <ui_calibration_map_table.h>

CalibrationMaps::CalibrationMaps(FileActions::EcuCalDefStructure *ecuCalDef, int romIndex, int mapIndex, QRect mdiAreaSize, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::CalibrationMaps)
{
    ui->setupUi(this);

    this->setParent(parent);
    this->setAttribute(Qt::WA_DeleteOnClose);

    QString mapWindowObjectName = QString::number(romIndex) + "," + QString::number(mapIndex) + "," + ecuCalDef->NameList.at(mapIndex);

    this->setObjectName(mapWindowObjectName);
    this->setWindowTitle(ecuCalDef->NameList.at(mapIndex) + " - " + ecuCalDef->FileName);

    QString xScaleUnitsTitle = "";
    if (ecuCalDef->XScaleNameList.at(mapIndex) != " ")
    {
        if (ecuCalDef->XScaleUnitsList.at(mapIndex) != " ")
            xScaleUnitsTitle = ecuCalDef->XScaleNameList.at(mapIndex) + " (" + ecuCalDef->XScaleUnitsList.at(mapIndex) + ")";
        else
            xScaleUnitsTitle = ecuCalDef->XScaleNameList.at(mapIndex);
    }
    ui->xScaleUnitsLabel->setText(xScaleUnitsTitle);

    if (mapIndex < ecuCalDef->UnitsList.length())
        ui->mapDataUnitsLabel->setText(ecuCalDef->UnitsList.at(mapIndex));

    if (ecuCalDef->TypeList.at(mapIndex) == "Switch")
    {
        //qDebug() << "Switchable map";
        mapWindowObjectName = mapWindowObjectName + "," + "Switch";
        mapCellWidth = mapCellWidthSelectable;
        xSize = 1;
        ySize = 1;
        ui->xScaleUnitsLabel->setFixedHeight(0);
    }
    if (ecuCalDef->TypeList.at(mapIndex) == "MultiSelectable")
    {
        //qDebug() << "MultiSelectable map";
        mapWindowObjectName = mapWindowObjectName + "," + "MultiSelectable";
        mapCellWidth = mapCellWidthSelectable;
        xSize = 1;
        ySize = 1;
        ui->xScaleUnitsLabel->setFixedHeight(0);
    }
    if (ecuCalDef->TypeList.at(mapIndex) == "Selectable")
    {
        //qDebug() << "Selectable map";
        mapWindowObjectName = mapWindowObjectName + "," + "Selectable";
        mapCellWidth = mapCellWidthSelectable;
        xSize = 1;
        ySize = 1;
        ui->xScaleUnitsLabel->setFixedHeight(0);
    }
    if (ecuCalDef->TypeList.at(mapIndex) == "1D")
    {
        qDebug() << "1D map";
        mapWindowObjectName = mapWindowObjectName + "," + "1D";
        mapCellWidth = mapCellWidth1D;
        xSize = 1;
        ySize = 1;
        ui->xScaleUnitsLabel->setFixedHeight(0);
    }
    if (ecuCalDef->TypeList.at(mapIndex) == "2D")
    {
        //qDebug() << "2D map" << ecuCalDef->NameList.at(mapIndex) << ecuCalDef->XSizeList.at(mapIndex).toInt() << ecuCalDef->YSizeList.at(mapIndex).toInt();
        if (ecuCalDef->YSizeList.at(mapIndex).toInt() > 1 || ecuCalDef->XSizeList.at(mapIndex).toInt() > 1)
            this->setWindowIcon(QIcon(":/icons/2D-64-W.png"));
        else
            this->setWindowIcon(QIcon(":/icons/1D-64-W.png"));
        if (ecuCalDef->XScaleTypeList.at(mapIndex) == "Static Y Axis")
            mapWindowObjectName = mapWindowObjectName + "," + "Static Y Axis";
        if (ecuCalDef->XScaleTypeList.at(mapIndex) == "Static X Axis")
            mapWindowObjectName = mapWindowObjectName + "," + "Static X Axis";
        else if (ecuCalDef->YSizeList.at(mapIndex).toInt() > 1)
            mapWindowObjectName = mapWindowObjectName + "," + "Y Axis";
        else if (ecuCalDef->XSizeList.at(mapIndex).toInt() > 1)
            mapWindowObjectName = mapWindowObjectName + "," + "X Axis";
        else
            mapWindowObjectName = mapWindowObjectName + "," + "X Axis";
        xSizeOffset = 0;
        ySizeOffset = 0;
        if (ecuCalDef->YSizeList.at(mapIndex).toInt() > 1)
            xSizeOffset = 1;
        if (ecuCalDef->XSizeList.at(mapIndex).toInt() > 1 || ecuCalDef->XScaleTypeList.at(mapIndex) == "Static Y Axis" || ecuCalDef->XScaleTypeList.at(mapIndex) == "Static X Axis")
            ySizeOffset = 1;
        xSize = ecuCalDef->XSizeList.at(mapIndex).toInt() + xSizeOffset;
        ySize = ecuCalDef->YSizeList.at(mapIndex).toInt() + ySizeOffset;
    }
    if (ecuCalDef->TypeList.at(mapIndex) == "3D")
    {
        //qDebug() << "3D map" << ecuCalDef->NameList.at(mapIndex) << ecuCalDef->XSizeList.at(mapIndex).toInt() << ecuCalDef->YSizeList.at(mapIndex).toInt();
        this->setWindowIcon(QIcon(":/icons/3D-64-W.png"));
        mapWindowObjectName = mapWindowObjectName + "," + "3D";
        xSizeOffset = 0;
        ySizeOffset = 0;
        if (ecuCalDef->YSizeList.at(mapIndex).toInt() > 1)
            xSizeOffset = 1;
        if (ecuCalDef->XSizeList.at(mapIndex).toInt() > 1)
            ySizeOffset = 1;
        xSize = ecuCalDef->XSizeList.at(mapIndex).toInt() + xSizeOffset;
        ySize = ecuCalDef->YSizeList.at(mapIndex).toInt() + ySizeOffset;

        VerticalLabel *yScaleUnitsLabel = new VerticalLabel();
        yScaleUnitsLabel->setAlignment(Qt::AlignCenter);
        yScaleUnitsLabel->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum );
        ui->horizontalLayout_2->insertWidget(0, yScaleUnitsLabel);
        QString yScaleUnitsTitle = ecuCalDef->YScaleNameList.at(mapIndex) + " (" + ecuCalDef->YScaleUnitsList.at(mapIndex) + ")";
        yScaleUnitsLabel->setText(yScaleUnitsTitle);

    }

    //qDebug() << "Create map" << mapWindowObjectName;
    this->setObjectName(mapWindowObjectName);
    ui->mapDataTableWidget->setObjectName(mapWindowObjectName);
    ui->mapDataTableWidget->setColumnCount(xSize);
    ui->mapDataTableWidget->setRowCount(ySize);
    ui->mapDataTableWidget->setStyleSheet("QTableWidget::item { padding: 3px }");
    ui->mapNameLabel->setText(ecuCalDef->NameList.at(mapIndex));

    if (ecuCalDef->TypeList.at(mapIndex) == "3D")
    {
        QTableWidgetItem *cellItem = new QTableWidgetItem;
        cellItem->setFlags(Qt::NoItemFlags);
        cellItem->setBackground(QBrush(QColor(0xf0, 0xf0, 0xf0, 255)));
        ui->mapDataTableWidget->setItem(0, 0, cellItem);
    }

    setMapTableWidgetItems(ecuCalDef, mapIndex);
    ui->mapDataTableWidget->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    ui->mapDataTableWidget->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
/*
    if (ui->mapDataTableWidget->horizontalHeader()->width() < mapCellWidth)
    {
        for (int col = 0; col < ui->mapDataTableWidget->columnCount(); col++)
            ui->mapDataTableWidget->horizontalHeader()->resizeSection(col, mapCellWidth);
    }

    if (ui->mapDataTableWidget->verticalHeader()->height() < mapCellHeight)
    {
        for (int row = 0; row < ui->mapDataTableWidget->rowCount(); row++)
            ui->mapDataTableWidget->verticalHeader()->resizeSection(row, mapCellHeight);
    }
*/
    setMapTableWidgetSize(mdiAreaSize.width()-15, mdiAreaSize.height()-15, xSize);

    if (ecuCalDef->TypeList.at(mapIndex) != "Selectable" && ecuCalDef->TypeList.at(mapIndex) != "Switch")
    {
        connect(ui->mapDataTableWidget, SIGNAL(cellClicked(int, int)), this, SLOT (cellClicked(int, int)));
        connect(ui->mapDataTableWidget, SIGNAL(cellPressed(int, int)), this, SLOT (cellPressed(int, int)));
        ///connect(ui->mapDataTableWidget, SIGNAL(cellEntered(int, int)), this, SLOT (cellActivated(int, int)));
        connect(ui->mapDataTableWidget, SIGNAL(currentCellChanged(int, int, int, int)), this, SLOT (cellChanged(int, int, int, int)));
    }
}

CalibrationMaps::~CalibrationMaps()
{
    delete ui;
}

void CalibrationMaps::setMapTableWidgetSize(int maxWidth, int maxHeight, int xSize){
    int w = 0;
    int h = 0;

    w += ui->mapDataTableWidget->contentsMargins().left() + ui->mapDataTableWidget->contentsMargins().right();
    h += ui->mapDataTableWidget->contentsMargins().top() + ui->mapDataTableWidget->contentsMargins().bottom();
    w += ui->mapDataTableWidget->verticalHeader()->width();
    for (int i = 0; i < ui->mapDataTableWidget->columnCount(); i++)
        w += ui->mapDataTableWidget->columnWidth(i);
    for (int i=0; i < ui->mapDataTableWidget->rowCount(); i++)
        h += ui->mapDataTableWidget->rowHeight(i);

    if (w > maxWidth)
    {
        w = maxWidth - 40;
        h += ui->mapDataTableWidget->horizontalHeader()->height() + 15;
    }
    else
        h += ui->mapDataTableWidget->horizontalHeader()->height();
    if (h > maxHeight)
    {
        h = maxHeight - 40;
        w += ui->mapDataTableWidget->verticalHeader()->width() + 15;
    }
    else
        w += ui->mapDataTableWidget->verticalHeader()->width();

    ui->mapDataTableWidget->setMinimumWidth(w);
    ui->mapDataTableWidget->setMaximumWidth(w);
    ui->mapDataTableWidget->setMinimumHeight(h);
    ui->mapDataTableWidget->setMaximumHeight(h);

    ui->mapDataTableWidget->setFixedWidth(w);
    ui->mapDataTableWidget->setFixedHeight(h);
}

void CalibrationMaps::setMapTableWidgetItems(FileActions::EcuCalDefStructure *ecuCalDef, int mapIndex)
{
    int xSize = ecuCalDef->XSizeList.at(mapIndex).toInt();
    int ySize = ecuCalDef->YSizeList.at(mapIndex).toInt();
    int mapSize = xSize * ySize;
    int maxWidth = 0;
    //qDebug() << "Map size:" << xSize << "x" << ySize << "=" << mapSize;

    QFont cellFont = ui->mapDataTableWidget->font();
    cellFont.setPointSize(cellFontSize);
    //cellFont.setBold(true);
    cellFont.setFamily("Franklin Gothic");
    //qDebug() << "Cell font size =" << cellFont.pointSize();

    if (ecuCalDef->TypeList.at(mapIndex) == "Switch")
    {
        //qDebug() << "Map:" << ecuCalDef->NameList.at(mapIndex) << "type 'switch'";
        bool checked = false;
        bool bStatus = false;
        QString state;

        QCheckBox *checkbox = new QCheckBox("On/Off");

        QStringList switch_states = ecuCalDef->StateList.at(mapIndex).split(",");
        //qDebug() << "Switch state list:" << switch_states;
        int switch_states_length = (switch_states.length() - 1);
        for (int i = 0; i < switch_states_length; i += 2)
        {
            QStringList switch_data_length = switch_states.at(i + 1).split(" ");
            QString switch_data = switch_states.at(i + 1);
            QString map_data;
            uint32_t byte_address = ecuCalDef->AddressList.at(mapIndex).toUInt(&bStatus, 16);
            if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize.toUInt() < (190 * 1024) && byte_address > 0x27FFF)
                byte_address -= 0x8000;
            for (int j = 0; j < switch_data_length.length(); j++)
            {
                map_data.append(QString("%1 ").arg(ecuCalDef->FullRomData.at(byte_address + j) & 0xFF,2,16,QLatin1Char('0')));
            }
            map_data.remove(map_data.length() - 1, 1);
            //qDebug() << map_data;
            if (switch_data == map_data)
                state = switch_states.at(i);
        }
        //qDebug() << "Switch state:" << state;

        if (state == "on")
            checked = true;
        else
            checked = false;

        checkbox->setChecked(checked);
        ui->mapDataTableWidget->setCellWidget(0, 0, checkbox);
        connect(checkbox, SIGNAL(stateChanged(int)), this, SIGNAL(checkbox_state_changed(int)));
    }
    else if (ecuCalDef->TypeList.at(mapIndex) == "MultiSelectable")
    {
        //qDebug() << "Map:" << ecuCalDef->NameList.at(mapIndex) << "type 'MultiSelectable'";
        QStringList mapDataCellText = ecuCalDef->MapData.at(mapIndex).split(",");
        QStringList selectionsList = ecuCalDef->SelectionsNameList.at(mapIndex).split(",");
        QStringList yScaleCellText = ecuCalDef->YScaleUnitsList.at(mapIndex).split(",");

        for (int i = 0; i < yScaleCellText.length(); i++)
        {
            QTableWidgetItem *cellItem = new QTableWidgetItem;
            cellItem->setTextAlignment(Qt::AlignCenter);
            cellItem->setFont(cellFont);
            cellItem->setText(yScaleCellText.at(i));
            ui->mapDataTableWidget->setItem(i, 0, cellItem);

            QComboBox *selectableComboBox = new QComboBox();
            selectableComboBox->setFont(cellFont);
            selectableComboBox->setFixedWidth(mapCellWidthSelectable);
            for (int j = 0; j < selectionsList.length(); j++){
                selectableComboBox->addItem(selectionsList.at(j));
            }
            selectableComboBox->setObjectName("selectableComboBox");
            selectableComboBox->setCurrentIndex(mapDataCellText.at(0).toInt());
            ui->mapDataTableWidget->setCellWidget(i, 1, selectableComboBox);
            connect(selectableComboBox, SIGNAL(currentTextChanged(QString)), this, SIGNAL(selectable_combobox_item_changed(QString)));
        }
    }
    else if (ecuCalDef->TypeList.at(mapIndex) == "Selectable")
    {
        //qDebug() << "Map:" << ecuCalDef->NameList.at(mapIndex) << "type 'Selectable'";
        QString mapDataCellText = ecuCalDef->MapData.at(mapIndex);//.split(",");
        QStringList selectionNameList = ecuCalDef->SelectionsNameList.at(mapIndex).split(",");
        QStringList selectionValueList = ecuCalDef->SelectionsValueList.at(mapIndex).split(",");
        int currentIndex = 0;

        QComboBox *selectableComboBox = new QComboBox();
        selectableComboBox->setFont(cellFont);
        selectableComboBox->setFixedWidth(mapCellWidthSelectable);
        for (int j = 0; j < selectionNameList.length() - 1; j++){
            if (selectionNameList.at(j) != "")
                selectableComboBox->addItem(selectionNameList.at(j));
            //qDebug() << "Set item: " + selectionNameList.at(j);
        }
        selectableComboBox->setObjectName("selectableComboBox");
        for (int i = 0; i < selectionNameList.length() - 1; i++)
        {
            if (selectionValueList.at(i).toUpper() == mapDataCellText.toUpper())
                currentIndex = i;
            //qDebug() << "Set item index: " + selectionNameList.at(i);
        }
        selectableComboBox->setCurrentIndex(currentIndex);
        ui->mapDataTableWidget->setCellWidget(0, 0, selectableComboBox);
        connect(selectableComboBox, SIGNAL(currentTextChanged(QString)), this, SIGNAL(selectable_combobox_item_changed(QString)));
    }
    else if (xSize > 1 && (ecuCalDef->XScaleTypeList.at(mapIndex) == "Static Y Axis" || ecuCalDef->XScaleTypeList.at(mapIndex) == "Static X Axis"))
    {
        //qDebug() << "Map:" << ecuCalDef->NameList.at(mapIndex) << "type: 'Static'";

        QStringList xScaleCellText;
        if (ecuCalDef->XScaleTypeList.at(mapIndex) == "Static Y Axis" || ecuCalDef->XScaleTypeList.at(mapIndex) == "Static X Axis")
            xScaleCellText = ecuCalDef->XScaleStaticDataList.at(mapIndex).split(",");
        else
            xScaleCellText = ecuCalDef->XScaleData.at(mapIndex).split(",");

        for (int i = 0; i < xSize; i++)
        {
            QTableWidgetItem *cellItem = new QTableWidgetItem;
            QString xScaleCellDataText = xScaleCellText.at(i);
            if (ecuCalDef->XScaleTypeList.at(mapIndex) == "Static Y Axis" || ecuCalDef->XScaleTypeList.at(mapIndex) == "Static X Axis")
                xScaleCellDataText = xScaleCellText.at(i);
            else
                xScaleCellDataText = QString::number(xScaleCellText.at(i).toFloat(), 'f', getMapValueDecimalCount(ecuCalDef->XScaleFormatList.at(mapIndex)));
            //qDebug() << "xScaleCellDataText:" << xScaleCellDataText;
            //qDebug() << "Y: xSize" << i << "/" << xSize;

            cellItem->setTextAlignment(Qt::AlignCenter);
            cellItem->setFont(cellFont);
            if (i < xScaleCellText.count())
                cellItem->setText(xScaleCellDataText);
            //qDebug() << "cellItem->text():" << cellItem->text();
            ui->mapDataTableWidget->setItem(0, i + xSizeOffset, cellItem);
/*
            if (ySize > 1)
                ui->mapDataTableWidget->setItem(0, i + 1, cellItem);
            else
                ui->mapDataTableWidget->setItem(0, i, cellItem);
*/
            QFontMetrics fm(cellFont);
            int width = fm.horizontalAdvance(xScaleCellDataText) + 20;
            if (width > maxWidth)
                maxWidth = width;
            ui->mapDataTableWidget->horizontalHeader()->resizeSection(i, maxWidth);
        }
    }
    else if (ySize > 1 && ecuCalDef->TypeList.at(mapIndex) != "Switch")
    {
        //qDebug() << "Map:" << ecuCalDef->NameList.at(mapIndex) << "type 'Y Axis 2D'";
        QStringList yScaleCellText = ecuCalDef->YScaleData.at(mapIndex).split(",");
        int maxWidth = 0;
        for (int i = 0; i < ySize; i++)
        {
            QTableWidgetItem *cellItem = new QTableWidgetItem;
            QString yScaleCellDataText = yScaleCellText.at(i);
            yScaleCellDataText = QString::number(yScaleCellText.at(i).toFloat(), 'f', getMapValueDecimalCount(ecuCalDef->YScaleFormatList.at(mapIndex)));

            cellItem->setTextAlignment(Qt::AlignCenter);
            cellItem->setFont(cellFont);
            if (i < yScaleCellText.count())
                cellItem->setText(yScaleCellDataText);
            ui->mapDataTableWidget->setItem(i + ySizeOffset, 0, cellItem);

            QFontMetrics fm(cellFont);
            int width = fm.horizontalAdvance(yScaleCellDataText) + 20;
            if (width > maxWidth)
                maxWidth = width;

        }
        ui->mapDataTableWidget->horizontalHeader()->resizeSection(0, maxWidth);
    }

    if (ecuCalDef->TypeList.at(mapIndex) == "1D")
    {
        //qDebug() << "Map:" << ecuCalDef->NameList.at(mapIndex) << "type '1D'";
        QStringList mapDataCellText = ecuCalDef->MapData.at(mapIndex).split(",");
        QTableWidgetItem *cellItem = new QTableWidgetItem;
        cellItem->setTextAlignment(Qt::AlignCenter);
        cellItem->setFont(cellFont);
        cellItem->setForeground(Qt::black);

        cellItem->setText(QString::number(mapDataCellText.at(0).toFloat(), 'f', getMapValueDecimalCount(ecuCalDef->FormatList.at(mapIndex))));
        ui->mapDataTableWidget->setItem(0, 0, cellItem);

    }

    if (ecuCalDef->TypeList.at(mapIndex) == "2D" || ecuCalDef->TypeList.at(mapIndex) == "3D")
    {
        //qDebug() << "Map:" << ecuCalDef->NameList.at(mapIndex) << "type '2D/3D'";
        QStringList mapDataCellText = ecuCalDef->MapData.at(mapIndex).split(",");
        QStringList xScaleCellText = ecuCalDef->XScaleData.at(mapIndex).split(",");

        ecuCalDef->MapCellColorMin[mapIndex] = QString::number(mapDataCellText.at(0).toFloat());
        ecuCalDef->MapCellColorMax[mapIndex] = QString::number(mapDataCellText.at(0).toFloat());

        for (int i = 0; i < (xSize * ySize); i++)
        {
            if (mapDataCellText.at(i).toFloat() < ecuCalDef->MapCellColorMin.at(mapIndex).toFloat())
                ecuCalDef->MapCellColorMin[mapIndex] = QString::number(mapDataCellText.at(i).toFloat());
            if (mapDataCellText.at(i).toFloat() > ecuCalDef->MapCellColorMax.at(mapIndex).toFloat())
                ecuCalDef->MapCellColorMax[mapIndex] = QString::number(mapDataCellText.at(i).toFloat());
            //if (ecuCalDef->MapCellColorMin[mapIndex].toFloat() == 0 || ecuCalDef->MapCellColorMin[mapIndex].toFloat() == 0)
            //    qDebug() << "i:" << i << mapDataCellText.at(i) << ecuCalDef->MapCellColorMin[mapIndex] << ecuCalDef->MapCellColorMax[mapIndex];
        }

        for (int i = 0; i < xSize; i++)
        {
            QTableWidgetItem *cellItem = new QTableWidgetItem;
            QString xScaleCellDataText;

            if (xScaleCellText.at(i) == " ") {
                xScaleCellText.insert(i, QString::number(i));
                xScaleCellDataText = xScaleCellText.at(i);
            }
            else if (ecuCalDef->XScaleTypeList.at(mapIndex) == "Static Y Axis" || ecuCalDef->XScaleTypeList.at(mapIndex) == "Static X Axis")
                xScaleCellDataText = xScaleCellText.at(i);
            else
                xScaleCellDataText = QString::number(xScaleCellText.at(i).toFloat(), 'f', getMapValueDecimalCount(ecuCalDef->XScaleFormatList.at(mapIndex)));

            //qDebug() << "xScaleCellDataText:" << xScaleCellDataText;

            cellItem->setTextAlignment(Qt::AlignCenter);
            cellItem->setFont(cellFont);
            if (i < xScaleCellText.count())
                cellItem->setText(xScaleCellDataText);
            ui->mapDataTableWidget->setItem(0, i + xSizeOffset, cellItem);

            QFontMetrics fm(cellFont);
            int width = fm.horizontalAdvance(xScaleCellDataText) + 20;
            if (width > maxWidth)
                maxWidth = width;
        }
        for (int i = 0; i < xSize; i++)
        {
            //if (xSize > 1)
                ui->mapDataTableWidget->horizontalHeader()->resizeSection(i + xSizeOffset, maxWidth);
            //else
                //ui->mapDataTableWidget->horizontalHeader()->resizeSection(i, maxWidth);
        }

        for (int i = 0; i < mapSize; i++)
        {
            QTableWidgetItem *cellItem = new QTableWidgetItem;
            cellItem->setTextAlignment(Qt::AlignCenter);
            cellItem->setFont(cellFont);
            int mapItemColor = getMapCellColors(ecuCalDef, mapDataCellText.at(i).toFloat(), mapIndex);
            int mapItemColorRed = (mapItemColor >> 16) & 0xff;
            int mapItemColorGreen = (mapItemColor >> 8) & 0xff;
            int mapItemColorBlue = mapItemColor & 0xff;
            //qDebug() << mapItemColorRed << mapItemColorGreen << mapItemColorBlue;
            cellItem->setBackground(QBrush(QColor(mapItemColorRed , mapItemColorGreen, mapItemColorBlue, 255)));
            if (ecuCalDef->TypeList.at(mapIndex) == "1D")
                cellItem->setForeground(Qt::black);
            else
                cellItem->setForeground(Qt::black);

            if (i < mapDataCellText.count())
                cellItem->setText(QString::number(mapDataCellText.at(i).toFloat(), 'f', getMapValueDecimalCount(ecuCalDef->FormatList.at(mapIndex))));
            //qDebug() << mapDataCellText.at(i);
            int yPos = 0;
            int xPos = 0;
            if (ecuCalDef->XSizeList.at(mapIndex).toUInt() > 1 || ecuCalDef->XScaleTypeList.at(mapIndex) == "Static Y Axis" || ecuCalDef->XScaleTypeList.at(mapIndex) == "Static X Axis")
                yPos = i / xSize + ySizeOffset;
            else
                yPos = i / xSize;
            if (ecuCalDef->YSizeList.at(mapIndex).toUInt() > 1)
                xPos = i - (yPos - ySizeOffset) * xSize + xSizeOffset;
            else
                xPos = i - (yPos - ySizeOffset) * xSize;

            /*
            xPos = i - (yPos - 1) * xSize + xSizeOffset;//0;
            if (ySize > 1 && xSize > 1)
                xPos = i - (yPos - 1) * xSize + 1;
            else
                xPos = i - (yPos - 1) * xSize + 1;
                */
            ui->mapDataTableWidget->setItem(yPos, xPos, cellItem);
        }
    }

}

int CalibrationMaps::getMapValueDecimalCount(QString valueFormat)
{
    //qDebug() << "Value format" << valueFormat;
    if (valueFormat.contains("."))
        return valueFormat.split(".").at(1).count(QLatin1Char('0'));
    else
        return 0;
}

int CalibrationMaps::getMapCellColors(FileActions::EcuCalDefStructure *ecuCalDef, float mapDataValue, int mapIndex)
{
    int mapCellColors;
    float mapMinValue = 0;
    float mapMaxValue = 0;
    float scale_start = (210.0/360.0);

    mapMinValue = ecuCalDef->MapCellColorMin.at(mapIndex).toFloat();
    mapMaxValue = ecuCalDef->MapCellColorMax.at(mapIndex).toFloat();

    QColor color;
    float color_scale = (1 - (mapDataValue - mapMinValue) / (mapMaxValue - mapMinValue)) * scale_start;
    float color_value = scale_start - color_scale;
    double r = 0;
    double g = 0;
    double b = 0;

    if (color_value < 0)
        color_value = 0;

    color.setHsvF(color_value, 0.85, 0.85);
    color.getRgbF(&r, &g, &b);
    mapCellColors = ((int)(r*255)<<16) + ((int)(g*255)<<8) + b*255;

    //qDebug() << "Map min:" << mapMinValue << "Map max:" << mapMaxValue << "color scale:" << color_scale << "scale start:" << scale_start;

    return mapCellColors;
}

void CalibrationMaps::cellClicked(int row, int col)
{
    startCol = col;
    startRow = row;
    //qDebug() << "Cell" << col << ":" << row << "clicked";

    QStringList objectName = ui->mapDataTableWidget->objectName().split(",");
    int cols = ui->mapDataTableWidget->columnCount();
    int rows = ui->mapDataTableWidget->rowCount();

    for (int i = 0; i < cols; i++)
    {
        for (int j = 0; j < rows; j++)
        {
            ui->mapDataTableWidget->item(j, i)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled);
        }
    }
    ui->mapDataTableWidget->item(row, col)->setSelected(true);
    if (objectName.at(3) == "3D")
        ui->mapDataTableWidget->item(0, 0)->setFlags(Qt::ItemIsEditable);

    if ((objectName.at(3) == "Static Y Axis" || objectName.at(3) == "Static X Axis") && rows > 1)
    {
        for (int j = 0; j < cols; j++)
        {
            ui->mapDataTableWidget->item(0, j)->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
        }
    }
}

void CalibrationMaps::cellPressed(int row, int col)
{
    startCol = col;
    startRow = row;
    //qDebug() << "Cell" << col << ":" << row << "pressed";

    int cols = ui->mapDataTableWidget->columnCount();
    int rows = ui->mapDataTableWidget->rowCount();

    QStringList objectName = ui->mapDataTableWidget->objectName().split(",");
    for (int i = 0; i < cols; i++)
    {
        for (int j = 0; j < rows; j++)
        {
            ui->mapDataTableWidget->item(j, i)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled);
        }
    }
    ui->mapDataTableWidget->item(row, col)->setSelected(true);
    if (objectName.at(3) == "3D")
        ui->mapDataTableWidget->item(0, 0)->setFlags(Qt::ItemIsEditable);

    if ((objectName.at(3) == "Static Y Axis" || objectName.at(3) == "Static X Axis") && rows > 1)
    {
        for (int j = 0; j < cols; j++)
        {
            ui->mapDataTableWidget->item(0, j)->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
        }
    }
}

void CalibrationMaps::cellChanged(int curRow, int curCol, int prevRow, int prevCol)
{
    //qDebug() << "Startcell =" << startCol << ":" << startRow << " and current cell is" << curCol << ":" << curRow;

    int cols = ui->mapDataTableWidget->columnCount();
    int rows = ui->mapDataTableWidget->rowCount();

    //qDebug() << "cellChanged" << ui->mapDataTableWidget->objectName().split(",").at(3);

    /* Check for 3D table */
    QStringList objectName = ui->mapDataTableWidget->objectName().split(",");
    if (startCol == 0 && objectName.at(3) == "3D")
    {
        for (int i = 1; i < cols; i++)
        {
            for (int j = 0; j < rows; j++)
            {
                //ui->mapDataTableWidget->item(0, i)->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
                ui->mapDataTableWidget->item(j, i)->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
            }
        }
    }
    else if (startRow == 0 && (objectName.at(3) == "3D" || objectName.at(3) == "X Axis" || objectName.at(3) == "Y Axis"))
    {
        for (int i = 0; i < cols; i++)
        {
            for (int j = 1; j < rows; j++)
            {
                //ui->mapDataTableWidget->item(j, 0)->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
                ui->mapDataTableWidget->item(j, i)->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
            }
        }
    }
    else
    {
        if (objectName.at(3) == "3D" || objectName.at(3) == "X Axis")
        {
            for (int i = 0; i < cols; i++)
            {
                ui->mapDataTableWidget->item(0, i)->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
                ui->mapDataTableWidget->item(0, i)->setSelected(false);
            }
        }
        if (objectName.at(3) == "3D")
        {
            for (int j = 0; j < rows; j++)
            {
                ui->mapDataTableWidget->item(j, 0)->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
                ui->mapDataTableWidget->item(j, 0)->setSelected(false);
            }
        }
    }
    if (objectName.at(3) == "3D")
        ui->mapDataTableWidget->item(0, 0)->setFlags(Qt::ItemIsEditable);

    if ((objectName.at(3) == "Static Y Axis" || objectName.at(3) == "Static X Axis") && rows > 1)
    {
        for (int j = 0; j < cols; j++)
        {
            ui->mapDataTableWidget->item(0, j)->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
        }
    }
}

