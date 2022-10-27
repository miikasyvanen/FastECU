#include "mainwindow.h"
#include "ui_mainwindow.h"

void MainWindow::menu_action_triggered(QString action)
{
    // FILE MENU
    if (action == "new"){
        qDebug() << action;}
    if (action == "open_calibration")
        open_calibration_file(NULL);
    if (action == "save_calibration")
        save_calibration_file();
    if (action == "save_calibration_as")
        save_calibration_file_as();
    if (action == "close_calibration")
        close_calibration();
    if (action == "quit")
        close_app();

    // EDIT MENU
    if (action == "undo")
        qDebug() << action;
    if (action == "redo")
        qDebug() << action;
    if (action == "copy")
        copy_value();
    if (action == "paste")
        paste_value();
    if (action == "ecu_definition_manager")
        ecu_definition_manager();
    if (action == "logger_definition_manager")
        logger_definition_manager();

    // TUNE MENU
    if (action == "fine_inc" || action == "fine_dec" || action == "coarse_inc" || action == "coarse_dec")
        inc_dec_value(action);
    if (action == "set_value")
        set_value();
    if (action == "interpolate_horizontal" || action == "interpolate_vertical" || action == "interpolate_bidirectional")
        interpolate_value(action);
    if (action == "toggle_realtime")
        toggle_realtime();
    if (action == "toggle_log_to_file")
        toggle_log_to_file();

    // ECU MENU
    if (action == "read_rom_from_ecu")
        start_ecu_operations("read");
    if (action == "test_write_rom_to_ecu")
        start_ecu_operations("test_write");
    if (action == "write_rom_to_ecu")
        start_ecu_operations("write");

    // VIEW MENU

    // HELP MENU
    if (action == "about")
        QMessageBox::information(this, tr("FastECU v0.1b"), "FastECU is open source tuning software for Subaru ECUs.\n"
                                                            "\n"
                                                            "This is the first test version for read and write ECU ROM\n"
                                                            "via K-Line connection with Open Port 2.0 or generic OBD2\n"
                                                            "cable. Software is tested with Win7 32/64bit and Linux.\n"
                                                            "\n"
                                                            "There WILL be bugs and things that don't work. Be patient\n"
                                                            "with new versions development."
                                                            "\n"
                                                            "All liability lies with the user. I am not responsible any\n"
                                                            "harm, laws broken or bricked ECUs that can follow for using\n"
                                                            "this software.\n"
                                                            "\n"
                                                            "\n"
                                                            "Huge thanks to following:\n"
                                                            "\n"
                                                            "fenugrec - author of nisprog software\n"
                                                            "rimwall - modifier of nisprog kernels for Subaru use\n"
                                                            "");
}

void MainWindow::inc_dec_value(QString action)
{
    union mapData{
        uint8_t one_byte_value[4];
        uint16_t two_byte_value[2];
        uint32_t four_byte_value;
        float float_value;
    } mapDataValue;

    int mapRomNumber = 0;
    int mapNumber = 0;
    QString mapName = "";
    QString mapType = "";
    bool bStatus;

    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (w)
    {
        QStringList mapWindowString = w->objectName().split(",");
        mapRomNumber = mapWindowString.at(0).toInt();
        mapNumber = mapWindowString.at(1).toInt();
        mapName = mapWindowString.at(2);
        mapType = mapWindowString.at(3);

        QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
        if (mapTableWidget)
        {
            if (mapTableWidget)
            {
                float mapCoarseIncValue = ecuCalDef[mapRomNumber]->CoarseIncList[mapNumber].toFloat();
                float mapFineIncValue = ecuCalDef[mapRomNumber]->FineIncList[mapNumber].toFloat();
                QStringList mapDataCellText = ecuCalDef[mapRomNumber]->MapData.at(mapNumber).split(",");
                QString map_format = ecuCalDef[mapRomNumber]->FormatList[mapNumber];
                QString map_value_to_byte = ecuCalDef[mapRomNumber]->ToByteList[mapNumber];
                QString map_value_from_byte = ecuCalDef[mapRomNumber]->FromByteList[mapNumber];
                QString map_value_storagetype = ecuCalDef[mapRomNumber]->StorageTypeList[mapNumber];
                QString map_value_endian = ecuCalDef[mapRomNumber]->EndianList[mapNumber];
                uint32_t map_data_address = ecuCalDef[mapRomNumber]->AddressList[mapNumber].toUInt(&bStatus, 16);
                int mapXSize = ecuCalDef[mapRomNumber]->XSizeList[mapNumber].toInt();
                int mapYSize = ecuCalDef[mapRomNumber]->YSizeList[mapNumber].toInt();

                if (!mapTableWidget->selectedRanges().isEmpty())
                {
                    QList<QTableWidgetSelectionRange> selected_range = mapTableWidget->selectedRanges();
                    int firstCol = selected_range.begin()->leftColumn() - 1;
                    int firstRow = selected_range.begin()->topRow() - 1;
                    int lastCol = selected_range.begin()->rightColumn() - 1;
                    int lastRow = selected_range.begin()->bottomRow() - 1;

                    if (selected_range.begin()->leftColumn() == 0 && mapYSize > 1){
                        mapDataCellText = ecuCalDef[mapRomNumber]->YScaleData.at(mapNumber).split(",");
                        mapCoarseIncValue = ecuCalDef[mapRomNumber]->YScaleCoarseIncList[mapNumber].toFloat();
                        mapFineIncValue = ecuCalDef[mapRomNumber]->YScaleFineIncList[mapNumber].toFloat();
                        map_format = ecuCalDef[mapRomNumber]->YScaleFormatList[mapNumber];
                        map_value_to_byte = ecuCalDef[mapRomNumber]->YScaleToByteList[mapNumber];
                        map_value_from_byte = ecuCalDef[mapRomNumber]->YScaleFromByteList[mapNumber];
                        map_value_storagetype = ecuCalDef[mapRomNumber]->YScaleStorageTypeList[mapNumber];
                        map_value_endian = ecuCalDef[mapRomNumber]->YScaleEndianList[mapNumber];
                        map_data_address = ecuCalDef[mapRomNumber]->YScaleAddressList[mapNumber].toUInt(&bStatus, 16);
                        firstCol++;
                        lastCol++;
                        mapXSize = 1;
                    }
                    else if (selected_range.begin()->topRow() == 0 && mapXSize > 1){
                        mapDataCellText = ecuCalDef[mapRomNumber]->XScaleData.at(mapNumber).split(",");
                        mapCoarseIncValue = ecuCalDef[mapRomNumber]->XScaleCoarseIncList[mapNumber].toFloat();
                        mapFineIncValue = ecuCalDef[mapRomNumber]->XScaleFineIncList[mapNumber].toFloat();
                        map_format = ecuCalDef[mapRomNumber]->XScaleFormatList[mapNumber];
                        map_value_to_byte = ecuCalDef[mapRomNumber]->XScaleToByteList[mapNumber];
                        map_value_from_byte = ecuCalDef[mapRomNumber]->XScaleFromByteList[mapNumber];
                        map_value_storagetype = ecuCalDef[mapRomNumber]->XScaleStorageTypeList[mapNumber];
                        map_value_endian = ecuCalDef[mapRomNumber]->XScaleEndianList[mapNumber];
                        map_data_address = ecuCalDef[mapRomNumber]->XScaleAddressList[mapNumber].toUInt(&bStatus, 16);
                        firstRow++;
                        lastRow++;
                    }
                    else{
                        if (mapXSize == 1) {
                            firstRow++;
                            lastRow++;
                        }
                        if (mapYSize == 1) {
                            firstCol++;
                            lastCol++;
                        }
                    }

                    double map_max_value;
                    if (map_value_storagetype == "uint8")
                        map_max_value = 0xff;
                    if (map_value_storagetype == "uint16")
                        map_max_value = 0xffff;
                    if (map_value_storagetype == "uint24")
                        map_max_value = 0xffffff;
                    if (map_value_storagetype == "float")
                        map_max_value = 0xfffffff;
                    if (map_value_storagetype == "uint32")
                        map_max_value = 0xffffffff;

                    for (int j = firstRow; j <= lastRow; j++)
                    {
                        for (int i = firstCol; i <= lastCol; i++)
                        {
                            float mapItemValue = mapDataCellText.at(j * mapXSize + i).toFloat();

                            uint16_t map_value_index = j * mapXSize + i;
                            QString rom_data_value = get_rom_data_value(mapRomNumber, map_data_address, map_value_index, map_value_storagetype, map_value_endian);

                            mapDataValue.float_value = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_to_byte, QString::number(mapItemValue)));
                            qDebug() << mapItemValue << rom_data_value.toFloat() << mapDataValue.float_value;

                            if (map_value_storagetype.startsWith("uint"))
                                mapDataValue.four_byte_value = (uint32_t)(qRound(mapDataValue.float_value));

                            qDebug() << mapItemValue << rom_data_value.toFloat() << mapDataValue.float_value;
                            while (rom_data_value.toFloat() == mapDataValue.float_value)
                            {
                                if (action == "coarse_inc")
                                    mapItemValue += mapCoarseIncValue;
                                if (action == "fine_inc")
                                    mapItemValue += mapFineIncValue;
                                if (action == "coarse_dec")
                                    mapItemValue -= mapCoarseIncValue;
                                if (action == "fine_dec")
                                    mapItemValue -= mapFineIncValue;

                                mapDataValue.float_value = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_to_byte, QString::number(mapItemValue)));
                                if (map_value_storagetype.startsWith("uint"))
                                    mapDataValue.four_byte_value = (uint32_t)(qRound(mapDataValue.float_value));

                                if ((int)mapDataValue.four_byte_value < 0) {
                                    mapDataValue.four_byte_value = 0;
                                    if (map_value_storagetype.startsWith("uint"))
                                        mapItemValue = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_from_byte, QString::number(mapDataValue.four_byte_value)));
                                    else
                                        mapItemValue = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_from_byte, QString::number(mapDataValue.float_value)));
                                    break;
                                }

                                if (map_value_storagetype.startsWith("uint")) {
                                    if (mapDataValue.four_byte_value >= map_max_value) {
                                        mapDataValue.four_byte_value = map_max_value;
                                        mapItemValue = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_from_byte, QString::number(mapDataValue.four_byte_value)));
                                        break;
                                    }
                                }
                                else {
                                    if (mapDataValue.float_value >= (float)map_max_value) {
                                        mapDataValue.float_value = (float)map_max_value;
                                        mapItemValue = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_from_byte, QString::number(mapDataValue.float_value)));
                                        break;
                                    }
                                }
                            }
                            if (map_value_storagetype.startsWith("uint"))
                                mapItemValue = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_from_byte, QString::number((float)mapDataValue.four_byte_value)));
                            else
                                mapItemValue = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_from_byte, QString::number(mapDataValue.float_value)));

                            mapDataCellText.replace(j * mapXSize + i, QString::number(mapItemValue));

                            if (mapDataValue.float_value == 0)
                                mapDataValue.float_value = 0.0f;

                            set_rom_data_value(mapRomNumber, map_data_address, map_value_index, map_value_storagetype, map_value_endian, mapDataValue.float_value);
                        }
                    }
                    if (selected_range.begin()->leftColumn() == 0 && mapYSize > 1)
                        ecuCalDef[mapRomNumber]->YScaleData.replace(mapNumber, mapDataCellText.join(","));
                    else if (selected_range.begin()->topRow() == 0 && mapXSize > 1)
                        ecuCalDef[mapRomNumber]->XScaleData.replace(mapNumber, mapDataCellText.join(","));
                    else
                        ecuCalDef[mapRomNumber]->MapData.replace(mapNumber, mapDataCellText.join(","));

                    set_maptablewidget_items();
                }
            }
        }
    }
}

void MainWindow::set_value()
{
    union mapData{
        uint8_t one_byte_value[4];
        uint16_t two_byte_value[2];
        uint32_t four_byte_value;
        float float_value;
    } mapDataValue;

    bool bStatus;

    int mapRomNumber = 0;
    int mapNumber = 0;
    QString mapName = "";

    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (w)
    {
        QStringList mapWindowString = w->objectName().split(",");
        mapRomNumber = mapWindowString.at(0).toInt();
        mapNumber = mapWindowString.at(1).toInt();
        mapName = mapWindowString.at(2);

        QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
        if (mapTableWidget)
        {
            QString text = QInputDialog::getText(this, tr("QInputDialog::getText()"),
                                                       tr("Set value: (ie: x20/+20/-20/20"),
                                                       QLineEdit::Normal, "", &bStatus);

            if (bStatus && !text.isEmpty()){
                QStringList mapDataCellText = ecuCalDef[mapRomNumber]->MapData.at(mapNumber).split(",");
                QString map_format = ecuCalDef[mapRomNumber]->FormatList[mapNumber];
                QString map_value_to_byte = ecuCalDef[mapRomNumber]->ToByteList[mapNumber];
                QString map_value_from_byte = ecuCalDef[mapRomNumber]->FromByteList[mapNumber];
                QString map_value_storagetype = ecuCalDef[mapRomNumber]->StorageTypeList[mapNumber];
                QString map_value_endian = ecuCalDef[mapRomNumber]->EndianList[mapNumber];
                uint32_t map_data_address = ecuCalDef[mapRomNumber]->AddressList[mapNumber].toUInt(&bStatus, 16);
                int mapXSize = ecuCalDef[mapRomNumber]->XSizeList[mapNumber].toInt();
                int mapYSize = ecuCalDef[mapRomNumber]->YSizeList[mapNumber].toInt();

                if (!mapTableWidget->selectedRanges().isEmpty()){
                    QList<QTableWidgetSelectionRange> selected_range = mapTableWidget->selectedRanges();
                    int firstCol = selected_range.begin()->leftColumn() - 1;
                    int firstRow = selected_range.begin()->topRow() - 1;
                    int lastCol = selected_range.begin()->rightColumn() - 1;
                    int lastRow = selected_range.begin()->bottomRow() - 1;

                    if (selected_range.begin()->leftColumn() == 0){
                        qDebug() << "Y scale";
                        mapDataCellText = ecuCalDef[mapRomNumber]->YScaleData.at(mapNumber).split(",");
                        map_format = ecuCalDef[mapRomNumber]->YScaleFormatList[mapNumber];
                        map_value_to_byte = ecuCalDef[mapRomNumber]->YScaleToByteList[mapNumber];
                        map_value_from_byte = ecuCalDef[mapRomNumber]->YScaleFromByteList[mapNumber];
                        map_value_storagetype = ecuCalDef[mapRomNumber]->YScaleStorageTypeList[mapNumber];
                        map_value_endian = ecuCalDef[mapRomNumber]->YScaleEndianList[mapNumber];
                        map_data_address = ecuCalDef[mapRomNumber]->YScaleAddressList[mapNumber].toUInt(&bStatus, 16);
                        firstCol += 1;
                        lastCol += 1;
                        mapXSize = 1;
                    }
                    else if (selected_range.begin()->topRow() == 0){
                        qDebug() << "X scale";
                        mapDataCellText = ecuCalDef[mapRomNumber]->XScaleData.at(mapNumber).split(",");
                        map_format = ecuCalDef[mapRomNumber]->XScaleFormatList[mapNumber];
                        map_value_to_byte = ecuCalDef[mapRomNumber]->XScaleToByteList[mapNumber];
                        map_value_from_byte = ecuCalDef[mapRomNumber]->XScaleFromByteList[mapNumber];
                        map_value_storagetype = ecuCalDef[mapRomNumber]->XScaleStorageTypeList[mapNumber];
                        map_value_endian = ecuCalDef[mapRomNumber]->XScaleEndianList[mapNumber];
                        map_data_address = ecuCalDef[mapRomNumber]->XScaleAddressList[mapNumber].toUInt(&bStatus, 16);
                        firstRow += 1;
                        lastRow += 1;
                    }
                    else{
                        if (mapXSize == 1) {
                            firstRow++;
                            lastRow++;
                        }
                        if (mapYSize == 1) {
                            firstCol++;
                            lastCol++;
                        }
                    }

                    double map_max_value;
                    if (map_value_storagetype == "uint8")
                        map_max_value = 0xff;
                    if (map_value_storagetype == "uint16")
                        map_max_value = 0xffff;
                    if (map_value_storagetype == "uint24")
                        map_max_value = 0xffffff;
                    if (map_value_storagetype == "float")
                        map_max_value = 0xfffffff;
                    if (map_value_storagetype == "uint32")
                        map_max_value = 0xffffffff;

                    for (int j = firstRow; j <= lastRow; j++){
                        for (int i = firstCol; i <= lastCol; i++){
                            float mapItemValue = mapDataCellText.at(j * mapXSize + i).toFloat();

                            uint16_t map_value_index = j * mapXSize + i;
                            QString rom_data_value = get_rom_data_value(mapRomNumber, map_data_address, map_value_index, map_value_storagetype, map_value_endian);

                            mapDataValue.float_value = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_to_byte, QString::number(mapItemValue)));

                            if (map_value_storagetype.startsWith("uint"))
                                mapDataValue.four_byte_value = (uint32_t)(qRound(mapDataValue.float_value));

                            while (rom_data_value.toFloat() == mapDataValue.float_value)
                            {
                                if (text.at(0) == '+'){
                                    QStringList mapItemText = text.split("+");
                                    mapItemValue = mapItemValue + mapItemText[1].toFloat();
                                }
                                else if (text.at(0) == '-'){
                                    QStringList mapItemText = text.split("-");
                                    mapItemValue = mapItemValue - mapItemText[1].toFloat();
                                }
                                else if (text.at(0) == '*'){
                                    QStringList mapItemText = text.split("*");
                                    mapItemValue = mapItemValue * mapItemText[1].toFloat();
                                }
                                else
                                    mapItemValue = text.toFloat();

                                mapDataValue.float_value = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_to_byte, QString::number(mapItemValue)));
                                if (map_value_storagetype.startsWith("uint"))
                                    mapDataValue.four_byte_value = (uint32_t)(qRound(mapDataValue.float_value));

                                if ((int)mapDataValue.four_byte_value < 0) {
                                    mapDataValue.four_byte_value = 0;
                                    if (map_value_storagetype.startsWith("uint"))
                                        mapItemValue = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_from_byte, QString::number(mapDataValue.four_byte_value)));
                                    else
                                        mapItemValue = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_from_byte, QString::number(mapDataValue.float_value)));
                                    break;
                                }

                                if (map_value_storagetype.startsWith("uint")) {
                                    if (mapDataValue.four_byte_value >= map_max_value) {
                                        mapDataValue.four_byte_value = map_max_value;
                                        mapItemValue = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_from_byte, QString::number(mapDataValue.four_byte_value)));
                                        break;
                                    }
                                }
                                else {
                                    if (mapDataValue.float_value >= (float)map_max_value) {
                                        mapDataValue.float_value = (float)map_max_value;
                                        mapItemValue = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_from_byte, QString::number(mapDataValue.float_value)));
                                        break;
                                    }
                                }
                            }
                            if (map_value_storagetype.startsWith("uint"))
                                mapItemValue = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_from_byte, QString::number((float)mapDataValue.four_byte_value)));
                            else
                                mapItemValue = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_from_byte, QString::number(mapDataValue.float_value)));

                            mapDataCellText.replace(j * mapXSize + i, QString::number(mapItemValue));

                            if (mapDataValue.float_value == 0)
                                mapDataValue.float_value = 0.0f;

                            set_rom_data_value(mapRomNumber, map_data_address, map_value_index, map_value_storagetype, map_value_endian, mapDataValue.float_value);
                        }
                    }
                    if (selected_range.begin()->leftColumn() == 0)
                        ecuCalDef[mapRomNumber]->YScaleData.replace(mapNumber, mapDataCellText.join(","));
                    else if (selected_range.begin()->topRow() == 0)
                        ecuCalDef[mapRomNumber]->XScaleData.replace(mapNumber, mapDataCellText.join(","));
                    else
                        ecuCalDef[mapRomNumber]->MapData.replace(mapNumber, mapDataCellText.join(","));

                    set_maptablewidget_items();
                }
            }
        }
    }
}

void MainWindow::interpolate_value(QString action)
{
    int mapRomNumber = 0;
    int mapNumber = 0;
    QString mapName = "";

    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (w)
    {
        QStringList mapWindowString = w->objectName().split(",");
        mapRomNumber = mapWindowString.at(0).toInt();
        mapNumber = mapWindowString.at(1).toInt();
        mapName = mapWindowString.at(2);

        QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
        if (mapTableWidget)
        {
            QString mapFormat = ecuCalDef[mapRomNumber]->FormatList[mapNumber];
            int decimalCount = 0;
            int mapXSize = ecuCalDef[mapRomNumber]->XSizeList[mapNumber].toInt();

            if (!mapTableWidget->selectedRanges().isEmpty()){
                QList<QTableWidgetSelectionRange> selected_range = mapTableWidget->selectedRanges();
                int firstCol = selected_range.begin()->leftColumn() - 1;
                int firstRow = selected_range.begin()->topRow() - 1;
                int lastCol = selected_range.begin()->rightColumn() - 1;
                int lastRow = selected_range.begin()->bottomRow() - 1;

                QStringList mapDataCellText;

                if (selected_range.begin()->leftColumn() == 0){
                    mapDataCellText = ecuCalDef[mapRomNumber]->YScaleData.at(mapNumber).split(",");
                    mapFormat = ecuCalDef[mapRomNumber]->YScaleFormatList[mapNumber];
                    firstCol += 1;
                    lastCol += 1;
                    mapXSize = 1;
                }
                else if (selected_range.begin()->topRow() == 0){
                    mapDataCellText = ecuCalDef[mapRomNumber]->XScaleData.at(mapNumber).split(",");
                    mapFormat = ecuCalDef[mapRomNumber]->XScaleFormatList[mapNumber];
                    firstRow += 1;
                    lastRow += 1;
                }
                else{
                    mapDataCellText = ecuCalDef[mapRomNumber]->MapData.at(mapNumber).split(",");
                }
                if (mapFormat.contains("0"))
                    decimalCount = mapFormat.count(QLatin1Char('0')) - 1;
                else
                    decimalCount = 0;


                int interpolateColCount = lastCol - firstCol + 1;
                int interpolateRowCount = lastRow - firstRow + 1;
                float cellValue[interpolateColCount][interpolateRowCount];

                float topLeftValue = mapDataCellText.at(firstRow * mapXSize + firstCol).toFloat();
                float topRightValue = mapDataCellText.at(firstRow * mapXSize + lastCol).toFloat();
                float bottomLeftValue = mapDataCellText.at(lastRow * mapXSize + firstCol).toFloat();
                float bottomRightValue = mapDataCellText.at(lastRow * mapXSize + lastCol).toFloat();

                float leftRowAdder = 0;
                float rightRowAdder = 0;
                float colAdder = 0;
                float rowAdder = 0;
                if (interpolateRowCount > 1){
                    leftRowAdder = (bottomLeftValue - topLeftValue) / (float)(interpolateRowCount - 1);
                    rightRowAdder = (bottomRightValue - topRightValue) / (float)(interpolateRowCount - 1);
                }
                for (int j = 0; j < interpolateRowCount; j++){
                    for (int i = 0; i < interpolateColCount; i++){
                        cellValue[i][j] = mapDataCellText.at((firstRow + j) * mapXSize + firstCol + i).toFloat();
                    }
                }
                if (action == "interpolate_horizontal"){
                    for (int j = 0; j < interpolateRowCount; j++){
                        if (interpolateColCount > 1)
                            colAdder = (cellValue[interpolateColCount - 1][j] - cellValue[0][j]) / (float)(interpolateColCount - 1);
                        for (int i = 0; i < interpolateColCount; i++){
                            if (interpolateColCount > 1)
                                cellValue[i][j] = cellValue[0][j] + i * colAdder;

                        }
                    }
                }
                if (action == "interpolate_vertical"){
                    for (int i = 0; i < interpolateColCount; i++){
                        if (interpolateRowCount > 1)
                            rowAdder = (cellValue[i][interpolateRowCount - 1] - cellValue[i][0]) / (float)(interpolateRowCount - 1);
                        for (int j = 0; j < interpolateRowCount; j++){
                            if (interpolateRowCount > 1)
                                cellValue[i][j] = cellValue[i][0] + j * rowAdder;

                        }
                    }
                }
                if (action == "interpolate_bidirectional"){
                    for (int j = 0; j < interpolateRowCount; j++){
                        cellValue[0][j] = cellValue[0][0] + j * leftRowAdder;
                        if (interpolateColCount > 1){
                            cellValue[interpolateColCount - 1][j] = cellValue[interpolateColCount - 1][0] + j * rightRowAdder;
                        }
                    }
                    for (int j = 0; j < interpolateRowCount; j++){
                        if (interpolateColCount > 1)
                            colAdder = (cellValue[interpolateColCount - 1][j] - cellValue[0][j]) / (float)(interpolateColCount - 1);
                        for (int i = 0; i < interpolateColCount; i++){
                            if (interpolateColCount > 1)
                                cellValue[i][j] = cellValue[0][j] + i * colAdder;
                        }
                    }
                }

                for (int j = 0; j < interpolateRowCount; j++){
                    for (int i = 0; i < interpolateColCount; i++){
                        mapDataCellText.replace((firstRow + j) * mapXSize + firstCol + i, QString::number(cellValue[i][j], 'f', decimalCount));
                    }
                }
                if (mapTableWidget->selectedRanges().front().leftColumn() == 0)
                    ecuCalDef[mapRomNumber]->YScaleData.replace(mapNumber, mapDataCellText.join(","));
                else if (mapTableWidget->selectedRanges().front().topRow() == 0)
                    ecuCalDef[mapRomNumber]->XScaleData.replace(mapNumber, mapDataCellText.join(","));
                else
                    ecuCalDef[mapRomNumber]->MapData.replace(mapNumber, mapDataCellText.join(","));

                set_maptablewidget_items();
            }
        }
    }
}

void MainWindow::copy_value()
{
    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (w)
    {
        QStringList mapWindowString = w->objectName().split(",");

        QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
        if (mapTableWidget){
            QModelIndexList cells = mapTableWidget->selectionModel()->selectedIndexes();
            //qSort(cells); // Necessary, otherwise they are in column order

            QString text;
            int currentRow = 0;
            foreach (const QModelIndex& cell, cells) {
                if (text.length() == 0) {
                } else if (cell.row() != currentRow) {
                    text += '\n';
                } else {
                    text += '\t';
                }
                currentRow = cell.row();
                text += cell.data().toString();
            }

            QApplication::clipboard()->setText(text);
        }
    }
}

void MainWindow::paste_value()
{
    int mapRomNumber = 0;
    int mapNumber = 0;

    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (w)
    {
        QStringList mapWindowString = w->objectName().split(",");
        mapRomNumber = mapWindowString.at(0).toInt();
        mapNumber = mapWindowString.at(1).toInt();

        QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
        if (mapTableWidget){
            if (!mapTableWidget->selectedRanges().isEmpty()){

                QStringList mapDataCellText = ecuCalDef[mapRomNumber]->MapData.at(mapNumber).split(",");
                QString pasteString = QApplication::clipboard()->text();
                QStringList rows = pasteString.split('\n');
                QString mapFormat = ecuCalDef[mapRomNumber]->FormatList[mapNumber];
                int mapXSize = ecuCalDef[mapRomNumber]->XSizeList[mapNumber].toInt();
                int mapYSize = ecuCalDef[mapRomNumber]->YSizeList[mapNumber].toInt();

                int firstCol = mapTableWidget->selectedRanges().front().leftColumn() - 1;
                int firstRow = mapTableWidget->selectedRanges().front().topRow() - 1;
                int numRows = rows.count();
                int numColumns = rows.first().count('\t') + 1;

                for (int i = 0; i < numRows; ++i) {
                    QStringList columns = rows[i].split('\t');
                    for (int j = 0; j < numColumns; ++j) {
                            if ((i + firstRow) < mapYSize && (j + firstCol) < mapXSize)
                                mapDataCellText.replace((i + firstRow) * mapXSize + (j + firstCol), columns[j]);
                    }
                }
                ecuCalDef[mapRomNumber]->MapData.replace(mapNumber, mapDataCellText.join(","));
                set_maptablewidget_items();
            }
        }
    }
}

void MainWindow::ecu_definition_manager()
{
    QDialog *definitions_manager_dialog = new QDialog;
    definitions_manager_dialog->setObjectName("ecu_definition_manager_dialog");
    definitions_manager_dialog->setFixedWidth(640);
    definitions_manager_dialog->setFixedHeight(240);
    definitions_manager_dialog->setWindowModality(Qt::ApplicationModal);
    definitions_manager_dialog->resize(800, 600);
    definitions_manager_dialog->setWindowTitle("ECU definition manager");
    definitions_manager_dialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *definitions_manager_layout = new QVBoxLayout;
    definitions_manager_dialog->setLayout(definitions_manager_layout);

    QListWidget *definition_files = new QListWidget;
    definition_files->setObjectName("ecu_definition_files_list");
    definition_files->setSelectionMode(QAbstractItemView::ExtendedSelection);
    for (int i = 0; i < configValues->ecu_definition_files.length(); i++)
    {
        new QListWidgetItem(configValues->ecu_definition_files.at(i), definition_files);
    }
    definitions_manager_layout->addWidget(definition_files);

    QWidget *definitions_manager_widget = new QWidget;
    QHBoxLayout *definitions_manager_buttons = new QHBoxLayout;
    definitions_manager_layout->addWidget(definitions_manager_widget);
    definitions_manager_widget->setLayout(definitions_manager_buttons);

    QPushButton *add_new_file = new QPushButton("Add new file");
    QPushButton *remove_file = new QPushButton("Remove file");
    QPushButton *close = new QPushButton("Close");
    QSpacerItem *spacer = new QSpacerItem(20, 20, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    definitions_manager_buttons->addWidget(add_new_file);
    definitions_manager_buttons->addWidget(remove_file);
    definitions_manager_buttons->addSpacerItem(spacer);
    definitions_manager_buttons->addWidget(close);

    connect(add_new_file, SIGNAL(clicked()), this, SLOT(add_new_ecu_definition_file()));
    connect(remove_file, SIGNAL(clicked()), this, SLOT(remove_ecu_definition_file()));
    connect(close, SIGNAL(clicked()), definitions_manager_dialog, SLOT(close()));
    definitions_manager_dialog->exec();

}

void MainWindow::logger_definition_manager()
{

}

void MainWindow::toggle_realtime()
{
    if (!loggingState)
    {
        logging_counter = 0;
        loggingState = true;
        log_ssm_values();
        logparams_poll_timer->start();
    }
    else
    {
        loggingState = false;
        log_params_request_started = false;
        log_ssm_values();
        delay(200);
        logparams_poll_timer->stop();
    }
}

void MainWindow::toggle_log_to_file()
{
    bool state = true;

    if (state)
        qDebug() << "Log to file on";
    else
        qDebug() << "Log to file off";
}


void MainWindow::set_maptablewidget_items()
{
    int mapRomNumber = 0;
    int mapNumber = 0;
    QString mapName = "";

    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (w)
    {
        QStringList mapWindowString = w->objectName().split(",");
        mapRomNumber = mapWindowString.at(0).toInt();
        mapNumber = mapWindowString.at(1).toInt();
        mapName = mapWindowString.at(2);

        QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
        if (mapTableWidget)
        {

            int xSize = ecuCalDef[mapRomNumber]->XSizeList.at(mapNumber).toInt();
            int ySize = ecuCalDef[mapRomNumber]->YSizeList.at(mapNumber).toInt();
            int mapSize = xSize * ySize;

            QFont cellFont = mapTableWidget->font();
            cellFont.setPointSize(cellFontSize);
            cellFont.setFamily("Franklin Gothic");

            if (xSize > 1)
            {
                QStringList xScaleCellText = ecuCalDef[mapRomNumber]->XScaleData.at(mapNumber).split(",");
                for (int i = 0; i < xSize; i++)
                {
                    QTableWidgetItem *cellItem = new QTableWidgetItem;
                    cellItem->setTextAlignment(Qt::AlignCenter);
                    cellItem->setFont(cellFont);
                    if (i < xScaleCellText.count())
                        cellItem->setText(QString::number(xScaleCellText.at(i).toFloat(), 'f', get_mapvalue_decimal_count(ecuCalDef[mapRomNumber]->XScaleFormatList.at(mapNumber))));
                    if (ySize > 1)
                        mapTableWidget->setItem(0, i + 1, cellItem);
                    else
                        mapTableWidget->setItem(0, i, cellItem);
                }
            }
            if (ySize > 1)
            {
                QStringList yScaleCellText = ecuCalDef[mapRomNumber]->YScaleData.at(mapNumber).split(",");
                for (int i = 0; i < ySize; i++)
                {
                    QTableWidgetItem *cellItem = new QTableWidgetItem;
                    cellItem->setTextAlignment(Qt::AlignCenter);
                    cellItem->setFont(cellFont);
                    if (i < yScaleCellText.count())
                        cellItem->setText(QString::number(yScaleCellText.at(i).toFloat(), 'f', get_mapvalue_decimal_count(ecuCalDef[mapRomNumber]->YScaleFormatList.at(mapNumber))));
                    mapTableWidget->setItem(i + 1, 0, cellItem);
                }
            }
            QStringList mapDataCellText = ecuCalDef[mapRomNumber]->MapData.at(mapNumber).split(",");
            for (int i = 0; i < mapSize; i++)
            {
                QTableWidgetItem *cellItem = new QTableWidgetItem;
                cellItem->setTextAlignment(Qt::AlignCenter);
                cellItem->setFont(cellFont);
                int mapItemColor = get_map_cell_colors(ecuCalDef[mapRomNumber], mapDataCellText.at(i).toFloat(), mapNumber);
                int mapItemColorRed = (mapItemColor >> 16) & 0xff;
                int mapItemColorGreen = (mapItemColor >> 8) & 0xff;
                int mapItemColorBlue = mapItemColor & 0xff;
                cellItem->setBackground(QBrush(QColor(mapItemColorRed , mapItemColorGreen, mapItemColorBlue, 255)));
                if (ecuCalDef[mapRomNumber]->TypeList.at(mapNumber) == "1D")
                    cellItem->setForeground(Qt::black);
                else
                    cellItem->setForeground(Qt::white);

                if (i < mapDataCellText.count())
                    cellItem->setText(QString::number(mapDataCellText.at(i).toFloat(), 'f', get_mapvalue_decimal_count(ecuCalDef[mapRomNumber]->FormatList.at(mapNumber))));
                int yPos = i / xSize + 1;
                int xPos = i - (yPos - 1) * xSize + 1;
                if (xSize == 1) {
                    yPos--;
                }
                if (ySize == 1) {
                    xPos--;
                }
                mapTableWidget->setItem(yPos, xPos, cellItem);
            }
        }
    }
}

QString MainWindow::get_rom_data_value(uint8_t map_rom_number, uint32_t map_data_address, uint16_t map_value_index, QString map_value_storagetype, QString map_value_endian)
{
    union mapData{
        uint8_t one_byte_value[4];
        uint16_t two_byte_value[2];
        uint32_t four_byte_value;
        float float_value;
    } mapDataValue;

    mapDataValue.float_value = 0;

    bool ok = false;
    QString value;
    uint8_t map_value_storagesize = 0;

    if (map_value_storagetype == "uint8")
        map_value_storagesize = 1;
    if (map_value_storagetype == "uint16")
        map_value_storagesize = 2;
    if (map_value_storagetype == "uint24")
        map_value_storagesize = 3;
    if (map_value_storagetype == "uint32" || map_value_storagetype == "float")
        map_value_storagesize = 4;

    uint32_t data_byte = 0;
    uint32_t byte_address = map_data_address + (map_value_index * map_value_storagesize);
    if (ecuCalDef[map_rom_number]->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef[map_rom_number]->FileSize < (170 * 1024) && byte_address > 0x27FFF)
        byte_address -= 0x8000;

    for (int k = 0; k < map_value_storagesize; k++)
    {
        if (map_value_endian == "little" || map_value_endian == "float")
        {
            data_byte = (data_byte << 8) + (uint8_t)ecuCalDef[map_rom_number]->FullRomData.at(byte_address + map_value_storagesize - 1 - k);
            mapDataValue.one_byte_value[k] = (uint8_t)ecuCalDef[map_rom_number]->FullRomData.at(byte_address + map_value_storagesize - 1 - k);
        }
        else
        {
            data_byte = (data_byte << 8) + (uint8_t)ecuCalDef[map_rom_number]->FullRomData.at(byte_address + k);
            mapDataValue.one_byte_value[k] = (uint8_t)ecuCalDef[map_rom_number]->FullRomData.at(byte_address + k);
        }
    }
    if (map_value_storagetype.startsWith("uint"))
        mapDataValue.four_byte_value = data_byte;

    value = QString::number(mapDataValue.float_value);

    return value;
}

void MainWindow::set_rom_data_value(uint8_t map_rom_number, uint32_t map_data_address, uint16_t map_value_index, QString map_value_storagetype, QString map_value_endian, float map_value)
{
    union mapData{
        uint8_t one_byte_value[4];
        uint16_t two_byte_value[2];
        uint32_t four_byte_value;
        float float_value;
    } mapDataValue;

    mapDataValue.float_value = map_value;

    uint8_t map_value_storagesize = 0;

    if (map_value_storagetype == "uint8")
        map_value_storagesize = 1;
    if (map_value_storagetype == "uint16")
        map_value_storagesize = 2;
    if (map_value_storagetype == "uint24")
        map_value_storagesize = 3;
    if (map_value_storagetype == "uint32" || map_value_storagetype == "float")
        map_value_storagesize = 4;

    uint32_t byte_address = map_data_address + (map_value_index * map_value_storagesize);
    if (ecuCalDef[map_rom_number]->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef[map_rom_number]->FileSize < (170 * 1024) && byte_address > 0x27FFF)
        byte_address -= 0x8000;

    for (int k = 0; k < map_value_storagesize; k++)
    {
        if (map_value_endian == "little")
        {
            ecuCalDef[map_rom_number]->FullRomData[byte_address + k] = (uint8_t)(mapDataValue.one_byte_value[map_value_storagesize - 1 - k]);
        }
        else
        {
            ecuCalDef[map_rom_number]->FullRomData[byte_address + k] = (uint8_t)(mapDataValue.one_byte_value[map_value_storagesize - 1 - k]);
        }
    }
}

int MainWindow::get_mapvalue_decimal_count(QString valueFormat)
{
    if (valueFormat.contains("0"))
        return valueFormat.count(QLatin1Char('0')) - 1;
    else
        return 0;
}

int MainWindow::get_map_cell_colors(FileActions::EcuCalDefStructure *ecuCalDef, float mapDataValue, int mapIndex)
{

    int mapCellColors;
    int mapCellColorRed = 0;
    int mapCellColorGreen = 0;
    int mapCellColorBlue = 0;
    float mapMaxValue = ecuCalDef->MaxValueList.at(mapIndex).toFloat();

    if (ecuCalDef->TypeList.at(mapIndex) != "1D"){
        mapCellColorRed = 255 - (int)(mapDataValue / mapMaxValue * 255);
        if (mapCellColorRed < 0)
            mapCellColorRed = 0;
        if (mapCellColorRed > 255)
            mapCellColorRed = 255;
        //mapCellColorRed = 0;
        mapCellColorGreen = 255 - (int)(mapDataValue / mapMaxValue * 255);
        if (mapCellColorGreen < 0)
            mapCellColorGreen = 0;
        if (mapCellColorGreen > 255)
            mapCellColorGreen = 255;
        mapCellColorGreen = 0;
        mapCellColorBlue = (int)(mapDataValue / mapMaxValue * 255);
        if (mapCellColorBlue < 0)
            mapCellColorBlue = 0;
        if (mapCellColorBlue > 255)
            mapCellColorBlue = 255;
        //mapCellColorBlue = 0;
    }
    else{
        mapCellColorRed = 255;
        mapCellColorGreen = 255;
        mapCellColorBlue = 255;

    }

    mapCellColors = (mapCellColorRed << 16) + (mapCellColorGreen << 8) + (mapCellColorBlue);
    return mapCellColors;
}
