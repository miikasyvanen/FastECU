#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "serial_port_actions.h"

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
    if (action == "winols_csv_to_romraider_xml")
        winols_csv_to_romraider_xml();
    if (action == "settings")
        show_preferences_window();

    // TUNE MENU
    if (action == "fine_inc" || action == "fine_dec" || action == "coarse_inc" || action == "coarse_dec")
        inc_dec_value(action);
    if (action == "set_value")
        set_value();
    if (action == "interpolate_horizontal" || action == "interpolate_vertical" || action == "interpolate_bidirectional")
        interpolate_value(action);
    if (action == "toggle_realtime")
        toggle_realtime();
    if (action == "log_to_file")
        toggle_log_to_file();

    // ECU MENU
    if (action == "connect_to_ecu")
        connect_to_ecu();
    if (action == "disconnect_from_ecu")
        disconnect_from_ecu();
    if (action == "read_rom_from_ecu")
        start_ecu_operations("read");
    if (action == "test_write_rom_to_ecu")
        start_ecu_operations("test_write");
    if (action == "write_rom_to_ecu")
        start_ecu_operations("write");

    // VIEW MENU
    if (action == "setlogviews")
        change_gauge_values();

    // TESTING MENU
    if (action == "dtc_window")
        show_dtc_window();
    if (action == "hex_editor")
        show_hex_editor();

    if (action == "haltech_ic7")
        toggle_haltech_ic7_display();
    if (action == "simulate_obd")
        toggle_simulate_obd();
    if (action == "can_listener")
        toggle_can_listener();
    if (action == "biu_communication")
        show_subaru_biu_window();
    if (action == "get_key")
        show_subaru_get_key_window();
    if (action == "terminal")
        show_terminal_window();

    // HELP MENU
    if (action == "about")
        QMessageBox::information(this, tr("FastECU"),       "FastECU is open source tuning software for Subaru ECUs,\n"
                                                            "TCUs and also modifying BIU and ECUs of other car makes.\n"
                                                            "\n"
                                                            "This is beta test version for read and write ROMs via\n"
                                                            "K-Line and CAN connection with Open Port 2.0 or generic\n"
                                                            "OBD2 cable. Software is tested in Win7/Win10 32/64bit\n"
                                                            "and Linux amd64 and aarch64 platforms.\n"
                                                            "\n"
                                                            "There WILL be bugs and things that don't work. Be patient\n"
                                                            "with new versions relesed.\n"
                                                            "\n"
                                                            "All liability lies with the user. We are not responsible any\n"
                                                            "harm, laws broken or bricked ECUs that can follow for using\n"
                                                            "this software.\n"
                                                            "\n"
                                                            "\n"
                                                            "Huge thanks to following:\n"
                                                            "\n"
                                                            "fenugrec - author of nisprog software\n"
                                                            "rimwall - modifier of nisprog kernels for Subaru use\n"
                                                            "SergArb - testing and software development\n"
                                                            "alesv - testing and software development\n"
                                                            "jimihimisimi - testing and software development\n"
                                                            "\n"
                                                            "...and to all of you who had support software development by\n"
                                                            "donating! All, even the smallest amount of donates are welcome!\n"
                                 );
}

// A refusal goes to the log and to the status bar: with no dialog (by design) the log alone
// leaves the user looking at an edit that silently did nothing.
void MainWindow::report_refused_edit(const QString &reason)
{
    emit LOG_D(reason, true, true);
    statusBar()->showMessage(reason, 10000);
}

// Whether a value falls outside a table's limits (" " means the definition gave none). The margin is
// float noise, not leeway: 14.6f + 0.1f is above 14.7f, and an edit landing on a limit must not be refused.
// A value that is not a finite number is outside every limit (NaN compares false against both ends), and
// so is any value when a limit the definition did give is not a number: the file is wrong, and reading its
// limit as 0 would refuse or accept by accident.
static bool outside_table_limits(float value, const QString &min_value, const QString &max_value)
{
    if (!qIsFinite(value))
        return true;
    const auto noise = [](float limit) { return 1e-6f * qMax(1.0f, qAbs(limit)); };
    bool parsed = true;
    if (min_value != " ") {
        const float min_limit = min_value.toFloat(&parsed);
        if (!parsed || value < min_limit - noise(min_limit))
            return true;
    }
    if (max_value != " ") {
        const float max_limit = max_value.toFloat(&parsed);
        if (!parsed || value > max_limit + noise(max_limit))
            return true;
    }
    return false;
}

// Whether a raw cell text is a finite float. A float cell is stored as given, so a raw value that is not
// one (a scaling that overflowed to "inf", or "nan") would be written to the image as infinity or NaN.
static bool raw_is_finite_float(const QString &raw_text)
{
    bool parsed = false;
    const float value = raw_text.toFloat(&parsed);
    return parsed && qIsFinite(value);
}

// A limit as the status bar shows it: a limit that is not a number is quoted, so the user sees the
// definition file holds text there rather than a number the value exceeded.
static QString limit_text(const QString &limit)
{
    if (limit == " ")
        return "none";
    bool parsed = false;
    limit.toFloat(&parsed);
    return parsed ? limit : "\"" + limit + "\" (not a number)";
}

// Names the definition file, because a wrong limit in a definition is a real case: a user facing
// one needs to know where the limit comes from rather than assume the tool is broken.
static QString outside_limits_reason(const QString &function, const QString &value_text, const QString &min_value, const QString &max_value, const QString &cell)
{
    return function + ": refusing to write, \"" + value_text + "\" is outside the limits set by the definition file (min "
           + limit_text(min_value) + ", max " + limit_text(max_value) + ") | " + cell;
}

// Paste only: whether a value given outright is one the cell already has: what it stores, or what the window shows
// for it. Copy takes the shown text, so a stored 0.256 comes back from the clipboard as 0.26 and is still
// the cell's own value. Compared as numbers, because +0.5 and 0.5 are the same value.
static bool cell_already_has(float value, const QString &cell_text, int shown_decimals)
{
    const float stored_value = cell_text.toFloat();
    return value == stored_value || value == QString::number(stored_value, 'f', shown_decimals).toFloat();
}

void MainWindow::inc_dec_value(QString action)
{
    int rom_number = 0;
    int map_number = 0;
    QString map_name = "";
    QString map_type = "";
    bool bStatus;

    //qDebug() << "Inc/dec value:" << action;
    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (w)
    {
        QStringList mapWindowString = w->objectName().split(",");
        rom_number = mapWindowString.at(0).toInt();
        map_number = mapWindowString.at(1).toInt();
        map_name = mapWindowString.at(2);
        map_type = mapWindowString.at(3);

        QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
        if (mapTableWidget)
        {
            emit LOG_D("Map " + ecuCalDef[rom_number]->NameList.at(map_number) + " scaling " + ecuCalDef[rom_number]->MapScalingNameList.at(map_number) + " min / max: " + ecuCalDef[rom_number]->MinValueList.at(map_number) + " / " + ecuCalDef[rom_number]->MaxValueList.at(map_number), true, true);
            float map_coarse_inc_value = ecuCalDef[rom_number]->CoarseIncList[map_number].toFloat();
            float map_fine_inc_value = ecuCalDef[rom_number]->FineIncList[map_number].toFloat();
            QStringList map_data_cell_text = ecuCalDef[rom_number]->MapData.at(map_number).split(",");
            QString map_format = ecuCalDef[rom_number]->FormatList[map_number];
            QString map_value_to_byte = ecuCalDef[rom_number]->ToByteList[map_number];
            QString map_value_from_byte = ecuCalDef[rom_number]->FromByteList[map_number];
            QString map_value_storagetype = ecuCalDef[rom_number]->StorageTypeList[map_number];
            QString map_value_endian = ecuCalDef[rom_number]->EndianList[map_number];
            QString map_min_value = ecuCalDef[rom_number]->MinValueList[map_number];
            QString map_max_value = ecuCalDef[rom_number]->MaxValueList[map_number];
            uint32_t map_data_address = ecuCalDef[rom_number]->AddressList[map_number].toUInt(&bStatus, 16);
            int mapXSize = ecuCalDef[rom_number]->XSizeList[map_number].toInt();
            int mapYSize = ecuCalDef[rom_number]->YSizeList[map_number].toInt();

            if (!mapTableWidget->selectedRanges().isEmpty())
            {
                QList<QTableWidgetSelectionRange> selected_range = mapTableWidget->selectedRanges();
                int first_col = selected_range.begin()->leftColumn() - 1;
                int first_row = selected_range.begin()->topRow() - 1;
                int last_col = selected_range.begin()->rightColumn() - 1;
                int last_row = selected_range.begin()->bottomRow() - 1;

                if (selected_range.begin()->leftColumn() == 0 && mapYSize > 1){
                    if (ecuCalDef[rom_number]->XScaleTypeList.at(map_number) == "Static Y Axis" || ecuCalDef[rom_number]->XScaleTypeList.at(map_number) == "Static X Axis") {
                        report_refused_edit("inc_dec_value: refusing to write, the axis is static");
                        return;
                    }
                    if (selected_range.begin()->rightColumn() > 0) {
                        report_refused_edit("inc_dec_value: refusing to write, the selection spans the axis and the data");
                        return;
                    }
                    map_data_cell_text = ecuCalDef[rom_number]->YScaleData.at(map_number).split(",");
                    map_coarse_inc_value = ecuCalDef[rom_number]->YScaleCoarseIncList[map_number].toFloat();
                    map_fine_inc_value = ecuCalDef[rom_number]->YScaleFineIncList[map_number].toFloat();
                    map_format = ecuCalDef[rom_number]->YScaleFormatList[map_number];
                    map_value_to_byte = ecuCalDef[rom_number]->YScaleToByteList[map_number];
                    map_value_from_byte = ecuCalDef[rom_number]->YScaleFromByteList[map_number];
                    map_value_storagetype = ecuCalDef[rom_number]->YScaleStorageTypeList[map_number];
                    map_value_endian = ecuCalDef[rom_number]->YScaleEndianList[map_number];
                    map_min_value = ecuCalDef[rom_number]->YScaleMinValueList[map_number];
                    map_max_value = ecuCalDef[rom_number]->YScaleMaxValueList[map_number];
                    map_data_address = ecuCalDef[rom_number]->YScaleAddressList[map_number].toUInt(&bStatus, 16);
                    first_col++;
                    last_col++;
                    mapXSize = 1;
                }
                else if (selected_range.begin()->topRow() == 0 && mapXSize > 1){
                    if (ecuCalDef[rom_number]->XScaleTypeList.at(map_number) == "Static Y Axis" || ecuCalDef[rom_number]->XScaleTypeList.at(map_number) == "Static X Axis") {
                        report_refused_edit("inc_dec_value: refusing to write, the axis is static");
                        return;
                    }
                    if (selected_range.begin()->bottomRow() > 0) {
                        report_refused_edit("inc_dec_value: refusing to write, the selection spans the axis and the data");
                        return;
                    }
                    map_data_cell_text = ecuCalDef[rom_number]->XScaleData.at(map_number).split(",");
                    map_coarse_inc_value = ecuCalDef[rom_number]->XScaleCoarseIncList[map_number].toFloat();
                    map_fine_inc_value = ecuCalDef[rom_number]->XScaleFineIncList[map_number].toFloat();
                    map_format = ecuCalDef[rom_number]->XScaleFormatList[map_number];
                    map_value_to_byte = ecuCalDef[rom_number]->XScaleToByteList[map_number];
                    map_value_from_byte = ecuCalDef[rom_number]->XScaleFromByteList[map_number];
                    map_value_storagetype = ecuCalDef[rom_number]->XScaleStorageTypeList[map_number];
                    map_value_endian = ecuCalDef[rom_number]->XScaleEndianList[map_number];
                    map_min_value = ecuCalDef[rom_number]->XScaleMinValueList[map_number];
                    map_max_value = ecuCalDef[rom_number]->XScaleMaxValueList[map_number];
                    map_data_address = ecuCalDef[rom_number]->XScaleAddressList[map_number].toUInt(&bStatus, 16);
                    first_row++;
                    last_row++;
                    if (mapYSize == 1) {
                        first_col++;
                        last_col++;
                    }
                }
                else{
                    if (mapXSize == 1 && ecuCalDef[rom_number]->XScaleTypeList.at(map_number) != "Static Y Axis" && ecuCalDef[rom_number]->XScaleTypeList.at(map_number) != "Static X Axis") {
                        first_row++;
                        last_row++;
                    }
                    if (mapYSize == 1) {
                        first_col++;
                        last_col++;
                    }
                }

                // A selection with no data cell (a static axis label, or the corner of a 3D table) has nothing to edit.
                if (first_row < 0 || first_col < 0) {
                    report_refused_edit("inc_dec_value: refusing to write, the selection holds no data cell");
                    return;
                }

                // Work out every cell before writing any: a step that cannot be taken refuses the whole
                // selection, the same all-or-nothing rule set_value and paste_value follow.
                QStringList new_rom_data_values;
                for (int j = first_row; j <= last_row; j++)
                {
                    for (int i = first_col; i <= last_col; i++)
                    {
                        float map_item_value = map_data_cell_text.at(j * mapXSize + i).toFloat();

                        uint16_t map_value_index = j * mapXSize + i;
                        QString rom_data_value = get_rom_data_value(rom_number, map_data_address, map_value_index, map_value_storagetype, map_value_endian);
                        QString new_rom_data_value;

                        do {
                            if (map_coarse_inc_value == 0 || map_fine_inc_value == 0)
                                QMessageBox::warning(this, tr("Set value"), "Fine / Coarse inc value not set or set to zero in definition file!");

                            if (action == "coarse_inc")
                                map_item_value += map_coarse_inc_value;
                            if (action == "fine_inc")
                                map_item_value += map_fine_inc_value;
                            if (action == "coarse_dec")
                                map_item_value -= map_coarse_inc_value;
                            if (action == "fine_dec")
                                map_item_value -= map_fine_inc_value;

                            if (outside_table_limits(map_item_value, map_min_value, map_max_value)) {
                                report_refused_edit(outside_limits_reason("inc_dec_value", QString::number(map_item_value, 'g', 7), map_min_value, map_max_value, "row " + QString::number(j) + " column " + QString::number(i)));
                                return;
                            }

                            new_rom_data_value = QString::number(fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_to_byte, QString::number(map_item_value, 'g', fileActions->float_precision))), 'g', fileActions->float_precision);
                            if (map_value_storagetype != "float") {
                                // Refused rather than saturated: saturating would write a value nobody asked
                                // for, or leave new_rom_data_value equal to rom_data_value and this loop spinning.
                                bool out_of_range = false;
                                new_rom_data_value = settle_raw_integer(new_rom_data_value.toDouble(), map_value_storagetype, &out_of_range);
                                if (out_of_range) {
                                    report_refused_edit("inc_dec_value: refusing to write, \"" + QString::number(map_item_value, 'g', 7) + "\" does not fit a " + map_value_storagetype + " cell | row " + QString::number(j) + " column " + QString::number(i));
                                    return;
                                }
                            }
                            else if (!raw_is_finite_float(new_rom_data_value)) {
                                report_refused_edit("inc_dec_value: refusing to write, \"" + new_rom_data_value + "\" is not a valid float | row " + QString::number(j) + " column " + QString::number(i));
                                return;
                            }
                        } while (rom_data_value == new_rom_data_value);

                        new_rom_data_values.append(new_rom_data_value);
                    }
                }

                for (int j = first_row; j <= last_row; j++)
                {
                    for (int i = first_col; i <= last_col; i++)
                    {
                        uint16_t map_value_index = j * mapXSize + i;
                        const QString new_rom_data_value = new_rom_data_values.takeFirst();
                        const float map_item_value = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_from_byte, new_rom_data_value));

                        map_data_cell_text.replace(j * mapXSize + i, QString::number(map_item_value, 'g', fileActions->float_precision));

                        set_rom_data_value(rom_number, map_data_address, map_value_index, map_value_storagetype, map_value_endian, new_rom_data_value);
                    }
                }
                if (selected_range.begin()->leftColumn() == 0 && mapYSize > 1)
                    ecuCalDef[rom_number]->YScaleData.replace(map_number, map_data_cell_text.join(","));
                else if (selected_range.begin()->topRow() == 0 && mapXSize > 1)
                    ecuCalDef[rom_number]->XScaleData.replace(map_number, map_data_cell_text.join(","));
                else
                    ecuCalDef[rom_number]->MapData.replace(map_number, map_data_cell_text.join(","));

                set_maptablewidget_items();
            }
            else
                report_refused_edit("inc_dec_value: refusing to write, no cell is selected");
        }
    }
}

void MainWindow::set_value()
{
    bool bStatus;

    int rom_number = 0;
    int map_number = 0;
    QString map_name = "";

    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (w)
    {
        QStringList mapWindowString = w->objectName().split(",");
        rom_number = mapWindowString.at(0).toInt();
        map_number = mapWindowString.at(1).toInt();
        map_name = mapWindowString.at(2);

        QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
        if (mapTableWidget)
        {
            QString text = QInputDialog::getText(this, tr("QInputDialog::getText()"),
                                                       tr("Set value: (ie: x20 | +20 | -20 | /20 | 20)"),
                                                       QLineEdit::Normal, "", &bStatus);

            text.replace(",", ".");
            text = text.trimmed(); // " +5" must not lose its operator to a leading space

            if (bStatus && text.isEmpty())
                report_refused_edit("set_value: refusing to write, no value was entered");
            else if (bStatus){
                // Parse once, before anything is touched: the operand is what follows a leading
                // operator, or the whole text. toFloat gives 0 for text that is not a number and
                // accepts "nan" and "inf" as numbers (NaN then evaluates to a raw 0, file_actions.cpp:2909),
                // so every one of those would be written over the selected cells.
                // The prompt advertises "x20" as multiply, so x and X are accepted alongside *.
                const QChar set_operator = (text.at(0) == 'x' || text.at(0) == 'X') ? QChar('*') : text.at(0);
                const bool has_operator = set_operator == '+' || set_operator == '-' || set_operator == '*' || set_operator == '/';
                bool parsed = false;
                const float set_operand = (has_operator ? text.mid(1) : text).toFloat(&parsed);
                if (!parsed || !qIsFinite(set_operand)) {
                    report_refused_edit("set_value: refusing to write, \"" + text + "\" is not a valid number");
                    return;
                }
                // The operand is the same for every cell, so say so once rather than once per cell.
                if (set_operator == '/' && set_operand == 0) {
                    report_refused_edit("set_value: refusing to write, cannot divide by zero");
                    QMessageBox::warning(this, tr("Set value"), "Cannot divide by zero!");
                    return;
                }

                QStringList map_data_cell_text = ecuCalDef[rom_number]->MapData.at(map_number).split(",");
                QString map_format = ecuCalDef[rom_number]->FormatList[map_number];
                QString map_value_to_byte = ecuCalDef[rom_number]->ToByteList[map_number];
                QString map_value_from_byte = ecuCalDef[rom_number]->FromByteList[map_number];
                QString map_value_storagetype = ecuCalDef[rom_number]->StorageTypeList[map_number];
                QString map_value_endian = ecuCalDef[rom_number]->EndianList[map_number];
                QString map_min_value = ecuCalDef[rom_number]->MinValueList[map_number];
                QString map_max_value = ecuCalDef[rom_number]->MaxValueList[map_number];
                uint32_t map_data_address = ecuCalDef[rom_number]->AddressList[map_number].toUInt(&bStatus, 16);
                int map_x_size = ecuCalDef[rom_number]->XSizeList[map_number].toInt();
                int map_y_size = ecuCalDef[rom_number]->YSizeList[map_number].toInt();

                if (!mapTableWidget->selectedRanges().isEmpty()){
                    QList<QTableWidgetSelectionRange> selected_range = mapTableWidget->selectedRanges();
                    int firstCol = selected_range.begin()->leftColumn() - 1;
                    int firstRow = selected_range.begin()->topRow() - 1;
                    int lastCol = selected_range.begin()->rightColumn() - 1;
                    int lastRow = selected_range.begin()->bottomRow() - 1;

                    if (selected_range.begin()->leftColumn() == 0 && map_y_size > 1){
                        if (ecuCalDef[rom_number]->XScaleTypeList.at(map_number) == "Static Y Axis" || ecuCalDef[rom_number]->XScaleTypeList.at(map_number) == "Static X Axis") {
                            report_refused_edit("set_value: refusing to write, the axis is static");
                            return;
                        }
                        if (selected_range.begin()->rightColumn() > 0) {
                            report_refused_edit("set_value: refusing to write, the selection spans the axis and the data");
                            return;
                        }
                        map_data_cell_text = ecuCalDef[rom_number]->YScaleData.at(map_number).split(",");
                        map_format = ecuCalDef[rom_number]->YScaleFormatList[map_number];
                        map_value_to_byte = ecuCalDef[rom_number]->YScaleToByteList[map_number];
                        map_value_from_byte = ecuCalDef[rom_number]->YScaleFromByteList[map_number];
                        map_value_storagetype = ecuCalDef[rom_number]->YScaleStorageTypeList[map_number];
                        map_value_endian = ecuCalDef[rom_number]->YScaleEndianList[map_number];
                        map_min_value = ecuCalDef[rom_number]->YScaleMinValueList[map_number];
                        map_max_value = ecuCalDef[rom_number]->YScaleMaxValueList[map_number];
                        map_data_address = ecuCalDef[rom_number]->YScaleAddressList[map_number].toUInt(&bStatus, 16);
                        firstCol += 1;
                        lastCol += 1;
                        map_x_size = 1;
                    }
                    else if (selected_range.begin()->topRow() == 0 && map_x_size > 1){
                        if (ecuCalDef[rom_number]->XScaleTypeList.at(map_number) == "Static Y Axis" || ecuCalDef[rom_number]->XScaleTypeList.at(map_number) == "Static X Axis") {
                            report_refused_edit("set_value: refusing to write, the axis is static");
                            return;
                        }
                        if (selected_range.begin()->bottomRow() > 0) {
                            report_refused_edit("set_value: refusing to write, the selection spans the axis and the data");
                            return;
                        }
                        map_data_cell_text = ecuCalDef[rom_number]->XScaleData.at(map_number).split(",");
                        map_format = ecuCalDef[rom_number]->XScaleFormatList[map_number];
                        map_value_to_byte = ecuCalDef[rom_number]->XScaleToByteList[map_number];
                        map_value_from_byte = ecuCalDef[rom_number]->XScaleFromByteList[map_number];
                        map_value_storagetype = ecuCalDef[rom_number]->XScaleStorageTypeList[map_number];
                        map_value_endian = ecuCalDef[rom_number]->XScaleEndianList[map_number];
                        map_min_value = ecuCalDef[rom_number]->XScaleMinValueList[map_number];
                        map_max_value = ecuCalDef[rom_number]->XScaleMaxValueList[map_number];
                        map_data_address = ecuCalDef[rom_number]->XScaleAddressList[map_number].toUInt(&bStatus, 16);
                        firstRow += 1;
                        lastRow += 1;
                        if (map_y_size == 1) {
                            firstCol++;
                            lastCol++;
                        }
                    }
                    else{
                        qDebug() << "x:" << map_x_size << "y:" << map_y_size;
                        if (map_x_size == 1 && ecuCalDef[rom_number]->XScaleTypeList.at(map_number) != "Static Y Axis" && ecuCalDef[rom_number]->XScaleTypeList.at(map_number) != "Static X Axis") {
                            firstRow++;
                            lastRow++;
                        }
                        if (map_y_size == 1) {
                            firstCol++;
                            lastCol++;
                        }
                    }

                    // A selection with no data cell (a static axis label, or the corner of a 3D table) has nothing to edit.
                    if (firstRow < 0 || firstCol < 0) {
                        report_refused_edit("set_value: refusing to write, the selection holds no data cell");
                        return;
                    }

                    // The edited value of one cell, in table units: the operator applied to the cell's current value.
                    auto edited_value = [&](float current_value) {
                        if (set_operator == '+')
                            return current_value + set_operand;
                        if (set_operator == '-')
                            return current_value - set_operand;
                        if (set_operator == '*')
                            return current_value * set_operand;
                        if (set_operator == '/')
                            return current_value / set_operand;
                        return set_operand;
                    };
                    auto raw_text_for = [&](float value) {
                        return QString::number(fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_to_byte, QString::number(value, 'g', fileActions->float_precision))), 'g', fileActions->float_precision);
                    };
                    // A cell the edit would leave as it is gets no write and no judgement: stock values can sit
                    // outside their limits, and restating one harms nothing. Compared with the stored value alone,
                    // not the rounded text the window shows (that is for paste, whose text came from the window):
                    // a typed value is a request, and 14.7 on a cell holding 14.74 is a change to make.
                    auto leaves_cell_as_is = [&](const QString &cell_text, float value) {
                        return value == cell_text.toFloat();
                    };

                    // Refuse before any cell or byte changes, so a selection is written whole or not at all. Nothing
                    // is clamped: the limit or range end would be a value nobody asked for, in an image that FastECU
                    // then checksums so nothing downstream rejects it. A float cell is stored as given, so a raw value
                    // that is not a finite float (a result that overflowed, or a scaling that does) would be written
                    // as infinity. An integer cell would saturate at the end of its range.
                    for (int j = firstRow; j <= lastRow; j++)
                    {
                        for (int i = firstCol; i <= lastCol; i++)
                        {
                            const QString &cell_text = map_data_cell_text.at(j * map_x_size + i);
                            const float value = edited_value(cell_text.toFloat());
                            if (leaves_cell_as_is(cell_text, value))
                                continue;
                            const QString cell_name = "row " + QString::number(j) + " column " + QString::number(i);
                            if (outside_table_limits(value, map_min_value, map_max_value)) {
                                report_refused_edit(outside_limits_reason("set_value", QString::number(value, 'g', 7), map_min_value, map_max_value, cell_name));
                                return;
                            }
                            const QString raw_text = raw_text_for(value);
                            if (map_value_storagetype == "float") {
                                if (!raw_is_finite_float(raw_text)) {
                                    report_refused_edit("set_value: refusing to write, \"" + raw_text + "\" is not a valid " + map_value_storagetype + " | index 0x" + QString::number(j * map_x_size + i, 16));
                                    return;
                                }
                            }
                            else {
                                bool out_of_range = false;
                                settle_raw_integer(raw_text.toDouble(), map_value_storagetype, &out_of_range);
                                if (out_of_range) {
                                    report_refused_edit("set_value: refusing to write, \"" + QString::number(value, 'g', 7) + "\" does not fit a " + map_value_storagetype + " cell | " + cell_name);
                                    return;
                                }
                            }
                        }
                    }

                    for (int j = firstRow; j <= lastRow; j++)
                    {
                        for (int i = firstCol; i <= lastCol; i++)
                        {
                            float map_item_value = edited_value(map_data_cell_text.at(j * map_x_size + i).toFloat());
                            if (leaves_cell_as_is(map_data_cell_text.at(j * map_x_size + i), map_item_value))
                                continue;

                            uint16_t map_value_index = j * map_x_size + i;
                            QString rom_data_value = raw_text_for(map_item_value);
                            if (map_value_storagetype != "float")
                                rom_data_value = settle_raw_integer(rom_data_value.toDouble(), map_value_storagetype);
                            map_item_value = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_from_byte, rom_data_value));
                            qDebug() << "map_item_value" << map_item_value;
                            map_data_cell_text.replace(j * map_x_size + i, QString::number(map_item_value, 'g', fileActions->float_precision));
                            qDebug() << j * map_x_size + i << QString::number(map_item_value);

                            set_rom_data_value(rom_number, map_data_address, map_value_index, map_value_storagetype, map_value_endian, rom_data_value);
                        }
                    }
                    if (selected_range.begin()->leftColumn() == 0 && map_y_size > 1)
                        ecuCalDef[rom_number]->YScaleData.replace(map_number, map_data_cell_text.join(","));
                    else if (selected_range.begin()->topRow() == 0 && map_x_size > 1)
                        ecuCalDef[rom_number]->XScaleData.replace(map_number, map_data_cell_text.join(","));
                    else
                        ecuCalDef[rom_number]->MapData.replace(map_number, map_data_cell_text.join(","));

                    set_maptablewidget_items();
                }
                else
                    report_refused_edit("set_value: refusing to write, no cell is selected");
            }
        }
    }
}

void MainWindow::interpolate_value(QString action)
{
    bool bStatus = false;

    int rom_number = 0;
    int map_number = 0;
    QString map_name = "";

    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (w)
    {
        QStringList mapWindowString = w->objectName().split(",");
        rom_number = mapWindowString.at(0).toInt();
        map_number = mapWindowString.at(1).toInt();
        map_name = mapWindowString.at(2);

        QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
        if (mapTableWidget)
        {
            QStringList map_data_cell_text = ecuCalDef[rom_number]->MapData.at(map_number).split(",");
            QString map_format = ecuCalDef[rom_number]->FormatList[map_number];
            QString map_value_to_byte = ecuCalDef[rom_number]->ToByteList[map_number];
            QString map_value_from_byte = ecuCalDef[rom_number]->FromByteList[map_number];
            QString map_value_storagetype = ecuCalDef[rom_number]->StorageTypeList[map_number];
            QString map_value_endian = ecuCalDef[rom_number]->EndianList[map_number];
            QString map_min_value = ecuCalDef[rom_number]->MinValueList[map_number];
            QString map_max_value = ecuCalDef[rom_number]->MaxValueList[map_number];
            uint32_t map_data_address = ecuCalDef[rom_number]->AddressList[map_number].toUInt(&bStatus, 16);
            int map_x_size = ecuCalDef[rom_number]->XSizeList[map_number].toInt();
            int map_y_size = ecuCalDef[rom_number]->YSizeList[map_number].toInt();

            if (!mapTableWidget->selectedRanges().isEmpty()){
                QList<QTableWidgetSelectionRange> selected_range = mapTableWidget->selectedRanges();
                int firstCol = selected_range.begin()->leftColumn() - 1;
                int firstRow = selected_range.begin()->topRow() - 1;
                int lastCol = selected_range.begin()->rightColumn() - 1;
                int lastRow = selected_range.begin()->bottomRow() - 1;

                if (selected_range.begin()->leftColumn() == 0 && map_y_size > 1){
                    if (ecuCalDef[rom_number]->XScaleTypeList.at(map_number) == "Static Y Axis" || ecuCalDef[rom_number]->XScaleTypeList.at(map_number) == "Static X Axis") {
                        report_refused_edit("interpolate_value: refusing to write, the axis is static");
                        return;
                    }
                    if (selected_range.begin()->rightColumn() > 0) {
                        report_refused_edit("interpolate_value: refusing to write, the selection spans the axis and the data");
                        return;
                    }
                    map_data_cell_text = ecuCalDef[rom_number]->YScaleData.at(map_number).split(",");
                    map_format = ecuCalDef[rom_number]->YScaleFormatList[map_number];
                    map_value_to_byte = ecuCalDef[rom_number]->YScaleToByteList[map_number];
                    map_value_from_byte = ecuCalDef[rom_number]->YScaleFromByteList[map_number];
                    map_value_storagetype = ecuCalDef[rom_number]->YScaleStorageTypeList[map_number];
                    map_value_endian = ecuCalDef[rom_number]->YScaleEndianList[map_number];
                    map_min_value = ecuCalDef[rom_number]->YScaleMinValueList[map_number];
                    map_max_value = ecuCalDef[rom_number]->YScaleMaxValueList[map_number];
                    map_data_address = ecuCalDef[rom_number]->YScaleAddressList[map_number].toUInt(&bStatus, 16);
                    firstCol += 1;
                    lastCol += 1;
                    map_x_size = 1;
                }
                else if (selected_range.begin()->topRow() == 0 && map_x_size > 1){
                    if (ecuCalDef[rom_number]->XScaleTypeList.at(map_number) == "Static Y Axis" || ecuCalDef[rom_number]->XScaleTypeList.at(map_number) == "Static X Axis") {
                        report_refused_edit("interpolate_value: refusing to write, the axis is static");
                        return;
                    }
                    if (selected_range.begin()->bottomRow() > 0) {
                        report_refused_edit("interpolate_value: refusing to write, the selection spans the axis and the data");
                        return;
                    }
                    map_data_cell_text = ecuCalDef[rom_number]->XScaleData.at(map_number).split(",");
                    map_format = ecuCalDef[rom_number]->XScaleFormatList[map_number];
                    map_value_to_byte = ecuCalDef[rom_number]->XScaleToByteList[map_number];
                    map_value_from_byte = ecuCalDef[rom_number]->XScaleFromByteList[map_number];
                    map_value_storagetype = ecuCalDef[rom_number]->XScaleStorageTypeList[map_number];
                    map_value_endian = ecuCalDef[rom_number]->XScaleEndianList[map_number];
                    map_min_value = ecuCalDef[rom_number]->XScaleMinValueList[map_number];
                    map_max_value = ecuCalDef[rom_number]->XScaleMaxValueList[map_number];
                    map_data_address = ecuCalDef[rom_number]->XScaleAddressList[map_number].toUInt(&bStatus, 16);
                    firstRow += 1;
                    lastRow += 1;
                    if (map_y_size == 1) {
                        firstCol++;
                        lastCol++;
                    }
                }
                else{
                    if (map_x_size == 1 && ecuCalDef[rom_number]->XScaleTypeList.at(map_number) != "Static Y Axis" && ecuCalDef[rom_number]->XScaleTypeList.at(map_number) != "Static X Axis") {
                        firstRow++;
                        lastRow++;
                    }
                    if (map_y_size == 1) {
                        firstCol++;
                        lastCol++;
                    }
                }

                // A selection with no data cell (a static axis label, or the corner of a 3D table) has nothing to edit.
                if (firstRow < 0 || firstCol < 0) {
                    report_refused_edit("interpolate_value: refusing to write, the selection holds no data cell");
                    return;
                }

                int interpolateColCount = lastCol - firstCol + 1;
                int interpolateRowCount = lastRow - firstRow + 1;
                float cellValue[128][128];
                //float cellValue[interpolateColCount][interpolateRowCount];

                float topLeftValue = map_data_cell_text.at(firstRow * map_x_size + firstCol).toFloat();
                float topRightValue = map_data_cell_text.at(firstRow * map_x_size + lastCol).toFloat();
                float bottomLeftValue = map_data_cell_text.at(lastRow * map_x_size + firstCol).toFloat();
                float bottomRightValue = map_data_cell_text.at(lastRow * map_x_size + lastCol).toFloat();

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
                        cellValue[i][j] = map_data_cell_text.at((firstRow + j) * map_x_size + firstCol + i).toFloat();
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

                // One definition of each, so the validation loop and the write loop cannot disagree about which cells they touch.
                auto leaves_cell_as_is = [&](int i, int j) {
                    return cellValue[i][j] == map_data_cell_text.at((firstRow + j) * map_x_size + firstCol + i).toFloat();
                };
                auto raw_text_for = [&](float value) {
                    return QString::number(fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_to_byte, QString::number(value, 'g', fileActions->float_precision))), 'g', fileActions->float_precision);
                };
                // Refuse before any write, so a refusal leaves the image and the window as they were. A result can lie
                // between its endpoints and still pass a limit, because an endpoint can already be past one; a cell the
                // interpolation leaves as it is is not judged against the limits, so stock values outside them do not
                // block it. The write loop below skips such a cell too, so neither loop judges a cell it leaves as it is.
                for (int j = 0; j < interpolateRowCount; j++){
                    for (int i = 0; i < interpolateColCount; i++){
                        if (leaves_cell_as_is(i, j))
                            continue;
                        const QString cell_name = "row " + QString::number(firstRow + j) + " column " + QString::number(firstCol + i);
                        if (outside_table_limits(cellValue[i][j], map_min_value, map_max_value)) {
                            report_refused_edit(outside_limits_reason("interpolate_value", QString::number(cellValue[i][j], 'g', 7), map_min_value, map_max_value, cell_name));
                            return;
                        }
                        const QString raw_text = raw_text_for(cellValue[i][j]);
                        if (map_value_storagetype == "float") {
                            if (!raw_is_finite_float(raw_text)) {
                                report_refused_edit("interpolate_value: refusing to write, \"" + raw_text + "\" is not a valid float | " + cell_name);
                                return;
                            }
                        }
                        else {
                            bool out_of_range = false;
                            settle_raw_integer(raw_text.toDouble(), map_value_storagetype, &out_of_range);
                            if (out_of_range) {
                                report_refused_edit("interpolate_value: refusing to write, \"" + QString::number(cellValue[i][j], 'g', 7) + "\" does not fit a " + map_value_storagetype + " cell | " + cell_name);
                                return;
                            }
                        }
                    }
                }

                for (int j = 0; j < interpolateRowCount; j++){
                    for (int i = 0; i < interpolateColCount; i++){
                        if (leaves_cell_as_is(i, j))
                            continue;
                        uint16_t map_value_index = (j + firstRow) * map_x_size + firstCol + i;
                        QString rom_data_value = raw_text_for(cellValue[i][j]);
                        if (map_value_storagetype != "float")
                            rom_data_value = settle_raw_integer(rom_data_value.toDouble(), map_value_storagetype);
                        float map_item_value = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_from_byte, rom_data_value));

                        map_data_cell_text.replace((firstRow + j) * map_x_size + firstCol + i, QString::number(map_item_value, 'g', fileActions->float_precision));

                        set_rom_data_value(rom_number, map_data_address, map_value_index, map_value_storagetype, map_value_endian, rom_data_value);
                    }
                }
                if (selected_range.begin()->leftColumn() == 0 && map_y_size > 1)
                    ecuCalDef[rom_number]->YScaleData.replace(map_number, map_data_cell_text.join(","));
                else if (selected_range.begin()->topRow() == 0 && map_x_size > 1)
                    ecuCalDef[rom_number]->XScaleData.replace(map_number, map_data_cell_text.join(","));
                else
                    ecuCalDef[rom_number]->MapData.replace(map_number, map_data_cell_text.join(","));

                set_maptablewidget_items();
            }
            else
                report_refused_edit("interpolate_value: refusing to write, no cell is selected");
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
    bool bStatus = false;

    int rom_number = 0;
    int map_number = 0;

    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (w)
    {
        QStringList mapWindowString = w->objectName().split(",");
        rom_number = mapWindowString.at(0).toInt();
        map_number = mapWindowString.at(1).toInt();

        QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
        if (mapTableWidget)
        {
            if (!mapTableWidget->selectedRanges().isEmpty())
            {
                QStringList mapDataCellText = ecuCalDef[rom_number]->MapData.at(map_number).split(",");
                QString pasteString = QApplication::clipboard()->text();
                QStringList rows = pasteString.split('\n');
                //QString mapFormat = ecuCalDef[mapRomNumber]->FormatList[mapNumber];
                QString map_value_storagetype = ecuCalDef[rom_number]->StorageTypeList[map_number];
                QString map_value_to_byte = ecuCalDef[rom_number]->ToByteList[map_number];
                QString map_value_from_byte = ecuCalDef[rom_number]->FromByteList[map_number];
                QString map_value_endian = ecuCalDef[rom_number]->EndianList[map_number];
                QString map_min_value = ecuCalDef[rom_number]->MinValueList[map_number];
                QString map_max_value = ecuCalDef[rom_number]->MaxValueList[map_number];
                uint32_t map_data_address = ecuCalDef[rom_number]->AddressList[map_number].toUInt(&bStatus, 16);
                int map_x_size = ecuCalDef[rom_number]->XSizeList[map_number].toInt();
                int map_y_size = ecuCalDef[rom_number]->YSizeList[map_number].toInt();

                const int shown_decimals = get_mapvalue_decimal_count(ecuCalDef[rom_number]->FormatList[map_number]);

                if (!mapTableWidget->selectedRanges().isEmpty()){
                    QList<QTableWidgetSelectionRange> selected_range = mapTableWidget->selectedRanges();
                    int firstCol = selected_range.begin()->leftColumn() - 1;
                    int firstRow = selected_range.begin()->topRow() - 1;

                    // Same corrections as the other edit functions: a one-wide table has no axis
                    // row or column in the grid, so the data cell is one further along.
                    if (map_x_size == 1 && ecuCalDef[rom_number]->XScaleTypeList.at(map_number) != "Static Y Axis" && ecuCalDef[rom_number]->XScaleTypeList.at(map_number) != "Static X Axis")
                        firstRow++;
                    if (map_y_size == 1)
                        firstCol++;
                    // Paste writes map data only: a selection that starts on an axis row or column is not pasted into.
                    if (firstRow < 0 || firstCol < 0) {
                        report_refused_edit("paste_value: refusing to write, the selection starts on an axis");
                        return;
                    }

                    // A spreadsheet's copy ends in a newline, which split() turns into a last row that is
                    // not data. trimmed() because a Windows line ending leaves a \r on that row.
                    while (!rows.isEmpty() && rows.last().trimmed().isEmpty())
                        rows.removeLast();
                    if (rows.isEmpty()) {
                        report_refused_edit("paste_value: refusing to write, the clipboard holds no values");
                        return;
                    }

                    int numRows = rows.count();
                    int numColumns = rows.first().count('\t') + 1;

                    auto raw_text_for = [&](const QString &cell_text) {
                        return QString::number(fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_to_byte, cell_text)), 'g', fileActions->float_precision);
                    };

                    // Refuse the whole paste before anything is written: writing the cells around a bad
                    // one would leave part of the block in the image, and a stray line in the clipboard
                    // would put the rest of the block on the wrong rows. A row shorter than the first
                    // leaves the cells past its end alone. Only cells that land inside the table are
                    // judged, because only those are written. toFloat accepts "nan" and "inf" as numbers
                    // (NaN then evaluates to a raw 0, file_actions.cpp:2909), so those are refused as well.
                    // A cell that already has the pasted value is skipped first, in this pass and the next:
                    // stock values can sit outside their limits, and pasting a table's own values back must
                    // neither be refused nor rewrite a cell with the rounded text the window shows. Nothing
                    // is clamped: the limit or range end would be a value nobody asked for, in an image that
                    // FastECU then checksums so nothing downstream rejects it.
                    for (int j = 0; j < numRows; ++j) {
                        QStringList columns = rows[j].split('\t');
                        for (int i = 0; i < numColumns && i < columns.count(); ++i) {
                            if ((j + firstRow) < map_y_size && (i + firstCol) < map_x_size)
                            {
                                bool parsed = false;
                                const float pasted_value = columns[i].toFloat(&parsed);
                                if (!parsed || !qIsFinite(pasted_value)) {
                                    report_refused_edit("paste_value: refusing to write, \"" + columns[i] + "\" is not a valid number | pasted row " + QString::number(j) + " column " + QString::number(i));
                                    return;
                                }
                                if (cell_already_has(pasted_value, mapDataCellText.at((j + firstRow) * map_x_size + (i + firstCol)), shown_decimals))
                                    continue;
                                const QString cell_name = "pasted row " + QString::number(j) + " column " + QString::number(i);
                                if (outside_table_limits(pasted_value, map_min_value, map_max_value)) {
                                    report_refused_edit(outside_limits_reason("paste_value", columns[i].trimmed(), map_min_value, map_max_value, cell_name));
                                    return;
                                }
                                const QString raw_text = raw_text_for(columns[i].trimmed());
                                // A float cell is stored as given, so a raw value that is not a finite float
                                // (a scaling that overflowed) would be written as infinity.
                                if (map_value_storagetype == "float") {
                                    if (!raw_is_finite_float(raw_text)) {
                                        report_refused_edit("paste_value: refusing to write, \"" + raw_text + "\" is not a valid " + map_value_storagetype + " | " + cell_name);
                                        return;
                                    }
                                }
                                // An integer cell would saturate at the end of its range.
                                else {
                                    bool out_of_range = false;
                                    settle_raw_integer(raw_text.toDouble(), map_value_storagetype, &out_of_range);
                                    if (out_of_range) {
                                        report_refused_edit("paste_value: refusing to write, \"" + columns[i].trimmed() + "\" does not fit a " + map_value_storagetype + " cell | " + cell_name);
                                        return;
                                    }
                                }
                            }
                        }
                    }

                    for (int j = 0; j < numRows; ++j) {
                        QStringList columns = rows[j].split('\t');
                        for (int i = 0; i < numColumns && i < columns.count(); ++i) {
                            if ((j + firstRow) < map_y_size && (i + firstCol) < map_x_size)
                            {
                                if (cell_already_has(columns[i].toFloat(), mapDataCellText.at((j + firstRow) * map_x_size + (i + firstCol)), shown_decimals))
                                    continue;
                                uint16_t map_value_index = (j + firstRow) * map_x_size + firstCol + i;
                                mapDataCellText.replace((j + firstRow) * map_x_size + (i + firstCol), columns[i].trimmed());
                                QString rom_data_value = raw_text_for(mapDataCellText.at((j + firstRow) * map_x_size + (i + firstCol)));
                                if (map_value_storagetype != "float") {
                                    rom_data_value = settle_raw_integer(rom_data_value.toDouble(), map_value_storagetype);
                                    // The cell text was set from the pasted text above, before the raw value
                                    // was settled to its range; show what was written, as set_value does.
                                    mapDataCellText.replace((j + firstRow) * map_x_size + (i + firstCol), QString::number(fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(map_value_from_byte, rom_data_value)), 'g', fileActions->float_precision));
                                }

                                set_rom_data_value(rom_number, map_data_address, map_value_index, map_value_storagetype, map_value_endian, rom_data_value);
                            }
                        }
                    }
                    ecuCalDef[rom_number]->MapData.replace(map_number, mapDataCellText.join(","));
                    set_maptablewidget_items();
                }
            }
            else
                report_refused_edit("paste_value: refusing to write, no cell is selected");
        }
    }
}

int MainWindow::connect_to_ecu()
{
    ecuid.clear();
    ecu_init_complete = false;
    set_status_bar_label(false, false, "");
    serial->reset_connection();

    qDebug() << "Opening interface, please wait...";
    open_serial_port();
    if (serial->is_serial_port_open())
    {
        serial_port_list->setDisabled(true);
        refresh_serial_port_list->setDisabled(true);
        qDebug() << "Initialising ECU, please wait...";
        int loopcount = 0;
        while (!ecu_init_complete && loopcount < 5)
        {
            ecu_init();
            delay(500);
            loopcount++;
        }
        if (!ecu_init_complete)
            disconnect_from_ecu();
    }
    else
    {
        QMessageBox::warning(this, tr("Serial port"), "Could not open interface!");
        return STATUS_ERROR;
    }
    return STATUS_SUCCESS;
}

void MainWindow::disconnect_from_ecu()
{
    qDebug() << "Disconnecting...";
    ecuid.clear();
    ecu_init_complete = false;
    set_status_bar_label(false, false, "");
    serial->reset_connection();

    serial->set_serial_port_baudrate("4800");
    serial->set_serial_port_parity(QSerialPort::NoParity);

    serial_port_list->setEnabled(true);
    refresh_serial_port_list->setEnabled(true);
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
    for (int i = 0; i < configValues->romraider_definition_files.length(); i++)
    {
        new QListWidgetItem(configValues->romraider_definition_files.at(i), definition_files);
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

void MainWindow::set_realtime_state(bool state)
{
    QAction *logger;
    QList<QMenu*> menus = ui->menubar->findChildren<QMenu*>();
    foreach (QMenu *menu, menus) {
        foreach (QAction *action, menu->actions()) {
            if (action->isSeparator()) {

            } else if (action->menu()) {

            } else {
                if (action->text() == "Logging")
                {
                    action->setChecked(state);
                }
            }
        }
    }
}

void MainWindow::toggle_realtime()
{
    QAction *logger;
    QList<QMenu*> menus = ui->menubar->findChildren<QMenu*>();
    foreach (QMenu *menu, menus) {
        foreach (QAction *action, menu->actions()) {
            if (action->isSeparator()) {

            } else if (action->menu()) {

            } else {
                if (action->text() == "Logging")
                {
                    logger = action;
                    logging_state = logger->isChecked();
                }
            }
        }
    }

    if (logging_state)
    {
        qDebug() << "Start datalog";
        if (!ecu_init_complete)
        {
            if (connect_to_ecu())
            {
                QMessageBox::information(this, tr("ECU connection"), "Unable to connect to ECU");
                logger->setChecked(false);
                return;
            }
        }
        logging_counter = 0;
        logging_state = true;
        log_ssm_values();
        logparams_poll_timer->start();
    }
    else
    {
        qDebug() << "Stop datalog";
        if (datalog_file_open){
            datalog_file_open = false;
            datalog_file.close();
        }

        logging_state = false;
        log_params_request_started = false;
        log_ssm_values();
        delay(200);
        logparams_poll_timer->stop();

        //disconnect_from_ecu();
    }
}

void MainWindow::toggle_log_to_file()
{
    QList<QMenu*> menus = ui->menubar->findChildren<QMenu*>();
    foreach (QMenu *menu, menus) {
        foreach (QAction *action, menu->actions()) {
            if (action->isSeparator()) {

            } else if (action->menu()) {

            } else {
                if (action->text() == "Log to file")
                    write_datalog_to_file = action->isChecked();
            }
        }
    }

    if (!write_datalog_to_file){
        if (datalog_file_open){
            datalog_file_open = false;
            datalog_file.close();
        }
    }
}

void MainWindow::toggle_haltech_ic7_display()
{
    QList<QMenu*> menus = ui->menubar->findChildren<QMenu*>();
    foreach (QMenu *menu, menus) {
        foreach (QAction *action, menu->actions()) {
            if (action->isSeparator()) {

            } else if (action->menu()) {

            } else {
                if (action->text() == "Haltech IC-7")
                    haltech_ic7_display_on = action->isChecked();
            }
        }
    }
    if (haltech_ic7_display_on)
        test_haltech_ic7_display();
}

void MainWindow::toggle_simulate_obd()
{
    QList<QMenu*> menus = ui->menubar->findChildren<QMenu*>();
    foreach (QMenu *menu, menus) {
        foreach (QAction *action, menu->actions()) {
            if (action->isSeparator()) {

            } else if (action->menu()) {

            } else {
                if (action->text() == "Simulate OBD")
                    simulate_obd_on = action->isChecked();
            }
        }
    }
    if (simulate_obd_on)
        simulate_obd();
}

void MainWindow::toggle_can_listener()
{
    QList<QMenu*> menus = ui->menubar->findChildren<QMenu*>();
    foreach (QMenu *menu, menus) {
        foreach (QAction *action, menu->actions()) {
            if (action->isSeparator()) {

            } else if (action->menu()) {

            } else {
                if (action->text() == "CAN listener")
                    can_listener_on = action->isChecked();
            }
        }
    }
    if (can_listener_on)
        can_listener();
}

void MainWindow::show_dtc_window()
{
    serial->reset_connection();
    ecuid.clear();
    ecu_init_complete = false;

    QStringList spl;
    spl.append(serial_ports.at(serial_port_list->currentIndex()));
    serial->set_serial_port_list(spl);

    emit LOG_D("Starting DTC operations", true, true);

    DtcOperations dtcOperations(serial, this);
    QObject::connect(&dtcOperations, &DtcOperations::LOG_E, syslogger, &SystemLogger::log_messages);
    QObject::connect(&dtcOperations, &DtcOperations::LOG_W, syslogger, &SystemLogger::log_messages);
    QObject::connect(&dtcOperations, &DtcOperations::LOG_I, syslogger, &SystemLogger::log_messages);
    QObject::connect(&dtcOperations, &DtcOperations::LOG_D, syslogger, &SystemLogger::log_messages);

    dtcOperations.exec();
    //dtcOperations->run();

    emit LOG_D("DTC operations stopped", true, true);
}

void MainWindow::show_hex_editor()
{
    emit LOG_D("Show hex editor", true, true);

    int rom_number = 0;

    QTreeWidgetItem *selectedItem = NULL;
    int item_count = ui->calibrationFilesTreeWidget->selectedItems().count();
    if (item_count)
    {
        selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);
        rom_number = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);

        //HexEdit *hexEdit = new HexEdit(ecuCalDef[rom_number], this);
        HexEdit *hexEdit = new HexEdit(ecuCalDef[rom_number], this);
    }

}

void MainWindow::show_preferences_window()
{
    Settings settings(configValues);
    settings.exec();
    //fileActions->save_config_file();

}

void MainWindow::show_subaru_biu_window()
{
    logging_poll_timer->stop();

    serial->reset_connection();
    ecuid.clear();
    ecu_init_complete = false;
    serial->set_add_iso14230_header(false);
    serial->set_is_iso14230_connection(true);
    open_serial_port();
    serial->change_port_speed("10400");
    //serial->change_port_speed("4800");

    BiuOperationsSubaru biuOperationsSubaru(serial, this);
    QObject::connect(&biuOperationsSubaru, &BiuOperationsSubaru::LOG_E, syslogger, &SystemLogger::log_messages);
    QObject::connect(&biuOperationsSubaru, &BiuOperationsSubaru::LOG_W, syslogger, &SystemLogger::log_messages);
    QObject::connect(&biuOperationsSubaru, &BiuOperationsSubaru::LOG_I, syslogger, &SystemLogger::log_messages);
    QObject::connect(&biuOperationsSubaru, &BiuOperationsSubaru::LOG_D, syslogger, &SystemLogger::log_messages);

    biuOperationsSubaru.exec();

    emit LOG_D("BIU stopped", true, true);

    serial->set_add_iso14230_header(false);
}

void MainWindow::show_terminal_window()
{
    QStringList serial_port;
    serial_port.append(serial_ports.at(serial_port_list->currentIndex()));
    serial->set_serial_port_list(serial_port);
    DataTerminal hexCommander(serial, this);
    QObject::connect(&hexCommander, &DataTerminal::LOG_E, syslogger, &SystemLogger::log_messages);
    QObject::connect(&hexCommander, &DataTerminal::LOG_W, syslogger, &SystemLogger::log_messages);
    QObject::connect(&hexCommander, &DataTerminal::LOG_I, syslogger, &SystemLogger::log_messages);
    QObject::connect(&hexCommander, &DataTerminal::LOG_D, syslogger, &SystemLogger::log_messages);

    hexCommander.exec();
}

void MainWindow::show_subaru_get_key_window()
{

    GetKeyOperationsSubaru getKeyOperationsSubaru (this);
    getKeyOperationsSubaru.exec();
}

void MainWindow::winols_csv_to_romraider_xml()
{
    DefinitionFileConvert definitionFileMaker;
    definitionFileMaker.exec();
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

            int xSizeOffset = 0;
            int ySizeOffset = 0;

            if (ecuCalDef[mapRomNumber]->YSizeList.at(mapNumber).toInt() > 1)
                xSizeOffset = 1;
            if (ecuCalDef[mapRomNumber]->XSizeList.at(mapNumber).toInt() > 1 || ecuCalDef[mapRomNumber]->XScaleTypeList.at(mapNumber) == "Static Y Axis" || ecuCalDef[mapRomNumber]->XScaleTypeList.at(mapNumber) == "Static X Axis")
                ySizeOffset = 1;

            QFont cellFont = mapTableWidget->font();
            cellFont.setPointSize(cellFontSize);
            cellFont.setFamily("Franklin Gothic");

            if (xSize > 1)
            {
                QStringList xScaleCellText = ecuCalDef[mapRomNumber]->XScaleData.at(mapNumber).split(",");
                for (int i = 0; i < xSize; i++)
                {
                    QTableWidgetItem *cellItem;
                    if (ySize > 1)
                        cellItem = mapTableWidget->item(0, i + 1);
                    else
                        cellItem = mapTableWidget->item(0, i);

                    cellItem->setTextAlignment(Qt::AlignCenter);
                    cellItem->setFont(cellFont);

                    QString xScaleCellDataText;

                    if (xScaleCellText.at(i) == " ") {
                        xScaleCellText.insert(i, QString::number(i));
                        xScaleCellDataText = xScaleCellText.at(i);
                    }
                    else if (ecuCalDef[mapRomNumber]->XScaleTypeList.at(mapNumber) == "Static Y Axis" || ecuCalDef[mapRomNumber]->XScaleTypeList.at(mapNumber) == "Static X Axis")
                        xScaleCellDataText = xScaleCellText.at(i);
                    else
                        xScaleCellDataText = QString::number(xScaleCellText.at(i).toFloat(), 'f', get_mapvalue_decimal_count(ecuCalDef[mapRomNumber]->XScaleFormatList.at(mapNumber)));

                    if (i < xScaleCellText.count())
                        cellItem->setText(xScaleCellDataText);
                }
            }
            if (ySize > 1)
            {
                QStringList yScaleCellText = ecuCalDef[mapRomNumber]->YScaleData.at(mapNumber).split(",");
                for (int i = 0; i < ySize; i++)
                {
                    QTableWidgetItem *cellItem;
                    cellItem = mapTableWidget->item(i + 1, 0);

                    cellItem->setTextAlignment(Qt::AlignCenter);
                    cellItem->setFont(cellFont);
                    if (i < yScaleCellText.count())
                        cellItem->setText(QString::number(yScaleCellText.at(i).toFloat(), 'f', get_mapvalue_decimal_count(ecuCalDef[mapRomNumber]->YScaleFormatList.at(mapNumber))));
                }
            }
            QStringList mapDataCellText = ecuCalDef[mapRomNumber]->MapData.at(mapNumber).split(",");
            for (int i = 0; i < mapSize; i++)
            {
                int yPos = 0;
                int xPos = 0;
                // This condition MUST match the one ySizeOffset was set from above and the one
                // calibration_maps.cpp:467 built the grid with. It had only the XSize test, so a
                // 1x1 table with a static axis got yPos 0 while ySizeOffset was 1, and the xPos
                // line below -- which subtracts that offset -- then produced item(0,1) of a 2x1
                // grid, a null the next line dereferenced.
                if (ecuCalDef[mapRomNumber]->XSizeList.at(mapNumber).toUInt() > 1 || ecuCalDef[mapRomNumber]->XScaleTypeList.at(mapNumber) == "Static Y Axis" || ecuCalDef[mapRomNumber]->XScaleTypeList.at(mapNumber) == "Static X Axis")
                    yPos = i / xSize + ySizeOffset;
                else
                    yPos = i / xSize;
                if (ecuCalDef[mapRomNumber]->YSizeList.at(mapNumber).toUInt() > 1)
                    xPos = i - (yPos - ySizeOffset) * xSize + xSizeOffset;
                else
                    xPos = i - (yPos - ySizeOffset) * xSize;

                //qDebug() << "X pos:" << xPos << "Y pos:" << yPos;
                QTableWidgetItem *cellItem;// = new QTableWidgetItem;
                cellItem = mapTableWidget->item(yPos, xPos);

                cellItem->setTextAlignment(Qt::AlignCenter);
                cellItem->setFont(cellFont);
                int mapItemColor = get_map_cell_colors(ecuCalDef[mapRomNumber], mapDataCellText.at(i).toFloat(), mapNumber);
                int mapItemColorRed = (mapItemColor >> 16) & 0xff;
                int mapItemColorGreen = (mapItemColor >> 8) & 0xff;
                int mapItemColorBlue = mapItemColor & 0xff;
                cellItem->setBackground(QBrush(QColor(mapItemColorRed , mapItemColorGreen, mapItemColorBlue, 255)));
                //if (ecuCalDef[mapRomNumber]->TypeList.at(mapNumber) == "1D")
                    cellItem->setForeground(Qt::black);
                //else
                //    cellItem->setForeground(Qt::white);

                if (i < mapDataCellText.count())
                    cellItem->setText(QString::number(mapDataCellText.at(i).toFloat(), 'f', get_mapvalue_decimal_count(ecuCalDef[mapRomNumber]->FormatList.at(mapNumber))));
            }
        }

        //fileActions->checksum_correction(ecuCalDef[mapRomNumber]);

    }
}

QString MainWindow::get_rom_data_value(uint8_t rom_number, uint32_t data_address, uint16_t value_index, QString storagetype, QString endian)
{
    union map_data{
        int8_t sbyte_value[4];
        int16_t sword_value[2];
        int32_t sdword_value;
        uint8_t byte_value[4];
        uint16_t word_value[2];
        uint32_t dword_value;
        float float_value;
    } map_data_value;

    map_data_value.float_value = 0;

    QString value;
    uint8_t storagesize = 0;

    storagesize = 1;
    if (storagetype == "uint16" || storagetype == "int16")
        storagesize = 2;
    else if (storagetype == "uint24" || storagetype == "int24")
        storagesize = 3;
    else if (storagetype == "uint32" || storagetype == "int32" || storagetype == "float")
        storagesize = 4;

    uint32_t data_byte = 0x00;

    uint32_t byte_address = data_address + (value_index * storagesize);
    if (ecuCalDef[rom_number]->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef[rom_number]->FileSize.toUInt() < byte_address)
        byte_address -= 0x8000;

    for (int k = 0; k < storagesize; k++)
    {
        if (endian == "little" || storagetype == "float")
        {
            data_byte = (data_byte << 8) + (uint8_t)ecuCalDef[rom_number]->FullRomData.at(byte_address + storagesize - 1 - k);
            map_data_value.byte_value[k] = (uint8_t)ecuCalDef[rom_number]->FullRomData.at(byte_address + storagesize - 1 - k);
        }
        else
        {
            data_byte = (data_byte << 8) + (uint8_t)ecuCalDef[rom_number]->FullRomData.at(byte_address + k);
            map_data_value.byte_value[k] = (uint8_t)ecuCalDef[rom_number]->FullRomData.at(byte_address + k);
        }
    }
    if (storagetype.startsWith("uint"))
        map_data_value.dword_value = data_byte;

    // This text is what set_rom_data_value re-encodes, so it must carry every bit of the cell:
    // QString::number's default of 6 significant digits turned 3b7db23f into 3b7db248 on a
    // no-change write-back.
    if (storagetype.startsWith("float"))
        value = QString::number(map_data_value.float_value, 'g', fileActions->float_precision);
    else if (storagetype.startsWith("uint"))
        value = QString::number(map_data_value.dword_value);
    else if (storagetype.startsWith("int")) {
        // data_byte already holds the cell in its own byte order, the same integer uint reads
        // above. The union is filled with the most significant byte at index 0 for either
        // endian= (and float reads it that way on purpose), so on a little-endian host it holds
        // an integer cell byte-swapped and must not be read for this.
        if (storagesize == 1)
            value = QString::number((int8_t)data_byte);
        if (storagesize == 2)
            value = QString::number((int16_t)data_byte);
        if (storagesize == 3)
            value = QString::number((int32_t)(data_byte << 8) >> 8);
        if (storagesize == 4)
            value = QString::number((int32_t)data_byte);
    }

    return value;
}

// Rounds a raw (storage-side) value to an integer and saturates it to the range of the storage type.
// The range is tested on the double BEFORE rounding: qRound64 is undefined on infinity and above
// about 9.2e18. *saturated, when given, reports that the rounded value was outside the range.
// NaN cannot arrive: calculate_value_from_expression returns 0 for it (file_actions.cpp:2909).
QString MainWindow::settle_raw_integer(double raw_value, const QString &map_value_storagetype, bool *saturated)
{
    // An unrecognised storage type is treated as uint8, the width get_rom_data_value and
    // set_rom_data_value also default to.
    int bits = 8;
    if (map_value_storagetype.endsWith("16"))
        bits = 16;
    else if (map_value_storagetype.endsWith("24"))
        bits = 24;
    else if (map_value_storagetype.endsWith("32"))
        bits = 32;

    const bool is_signed = map_value_storagetype.startsWith("int");
    const qint64 range_min = is_signed ? -(Q_INT64_C(1) << (bits - 1)) : 0;
    const qint64 range_max = is_signed ? (Q_INT64_C(1) << (bits - 1)) - 1 : (Q_INT64_C(1) << bits) - 1;

    // One unit of slack: anything this close to the range is rounded below and judged exactly.
    qint64 rounded;
    if (raw_value < (double)range_min - 1)
        rounded = range_min - 1;
    else if (raw_value > (double)range_max + 1)
        rounded = range_max + 1;
    else
        rounded = qRound64(raw_value);

    const bool out_of_range = rounded < range_min || rounded > range_max;
    if (saturated)
        *saturated = out_of_range;
    if (rounded < range_min)
        rounded = range_min;
    else if (rounded > range_max)
        rounded = range_max;

    return QString::number(rounded);
}

void MainWindow::set_rom_data_value(uint8_t rom_number, uint32_t data_address, uint16_t value_index, QString storagetype, QString endian, QString map_value)
{
    union map_data{
        int8_t sbyte_value[4];
        int16_t sword_value[2];
        int32_t sdword_value;
        uint8_t byte_value[4];
        uint16_t word_value[2];
        uint32_t dword_value;
        float float_value;
    } map_data_value;

    // The raw storage value arrives as TEXT and is encoded HERE, by storage type, so the text must
    // carry the value at full precision (FileActions::float_precision, not QString::number's
    // default of 6 significant digits). It used to arrive as a float: every caller put the
    // text's integer form into the union's dword and passed the float alias, so a float cell
    // stored toInt() -- 0 for any fractional value -- as the float's raw bits, and an integer
    // cell survived only because that bit pattern happened to be a denormal the copy preserved
    // (a negative one made a NaN, which a copy may quiet).
    map_data_value.dword_value = 0;
    if (storagetype.startsWith("float"))
        map_data_value.float_value = map_value.toFloat();
    else {
        // Callers settle integers to their range first (settle_raw_integer), so a parse failure
        // here means a caller skipped that. Refuse rather than write 0 over the cell. The caller
        // cannot see the refusal (this returns void) and has already updated its cell text, so the
        // window and the image would then disagree; the log line is the only trace.
        bool parsed = false;
        if (storagetype.startsWith("uint"))
            map_data_value.dword_value = map_value.toUInt(&parsed);
        else
            map_data_value.sdword_value = map_value.toInt(&parsed);
        if (!parsed) {
            emit LOG_D("set_rom_data_value: refusing to write, \"" + map_value + "\" is not a valid " + storagetype + " | index 0x" + QString::number(value_index, 16), true, true);
            return;
        }
    }

    emit LOG_D("set_rom_data_value: 0x" + QString::number(map_data_value.dword_value, 16) + " | 0x" + QString::number(value_index, 16), true, true);

    uint8_t storagesize = 0;

    storagesize = 1;
    if (storagetype == "uint16" || storagetype == "int16")
        storagesize = 2;
    else if (storagetype == "uint24" || storagetype == "int24")
        storagesize = 3;
    else if (storagetype == "uint32" || storagetype == "int32" || storagetype == "float")
        storagesize = 4;

    uint32_t byte_address = data_address + (value_index * storagesize);
    if (ecuCalDef[rom_number]->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef[rom_number]->FileSize.toUInt() < (190 * 1024) && byte_address > 0x27FFF)
        byte_address -= 0x8000;

    // byte_value[0] is the LOW byte on a little-endian host, so byte_value[k] at increasing
    // addresses lays a value down little-endian and byte_value[storagesize - 1 - k] big-endian.
    // The two branches used to be the wrong way round for integers, and get_rom_data_value's old
    // signed decode swapped too, hiding it for signed cells; both are fixed, so nothing here
    // compensates for a reader.
    //
    // Float is big-endian regardless of endian=, because both readers read it that way
    // (get_rom_data_value above, file_actions.cpp:2371). That attribute is live for float:
    // SubaruDefs' RomRaider ecu_defs.xml carries 715 float tables with endian="little" per file,
    // so testing endian alone would read those big-endian and write them little-endian.
    //
    // Integers honour endian=. get_rom_data_value decodes signed and unsigned from one
    // byte-ordered integer, so a cell it reads writes back unchanged. The file reader does not
    // yet: file_actions.cpp:2373 and 2380 misdecode little-endian and signed integers, so what
    // the window shows for those can differ from what this function round-trips.
    for (int k = 0; k < storagesize; k++)
    {
        if (endian == "little" && storagetype != "float")
        {
            ecuCalDef[rom_number]->FullRomData[byte_address + k] = (uint8_t)(map_data_value.byte_value[k]);
        }
        else
        {
            ecuCalDef[rom_number]->FullRomData[byte_address + k] = (uint8_t)(map_data_value.byte_value[storagesize - 1 - k]);
        }
    }
}

int MainWindow::get_mapvalue_decimal_count(QString valueFormat)
{
    if (valueFormat.contains("."))
        return valueFormat.split(".").at(1).count(QLatin1Char('0'));
    else
        return 0;//valueFormat.count(QLatin1Char('0'));
}

int MainWindow::get_map_cell_colors(FileActions::EcuCalDefStructure *ecuCalDef, float mapDataValue, int mapIndex)
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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    double r = 0;
    double g = 0;
    double b = 0;
#else
    float r = 0;
    float g = 0;
    float b = 0;
#endif

    if (color_value < 0)
        color_value = 0;

    color.setHsvF(color_value, 0.85, 0.85);
    color.getRgbF(&r, &g, &b);
    mapCellColors = ((int)(r*255)<<16) + ((int)(g*255)<<8) + b*255;

    //qDebug() << "Map min:" << mapMinValue << "Map max:" << mapMaxValue << "color scale:" << color_scale << "scale start:" << scale_start;

    return mapCellColors;
}

bool MainWindow::check_rom_data_value(QString storagetype, QString rom_data_value, QString new_rom_data_value)
{
    uint8_t storagesize = 1;
    bool result = false;

    union map_data{
        int8_t sbyte_value[4];
        int16_t sword_value[2];
        int32_t sdword_value;
        uint8_t byte_value[4];
        uint16_t word_value[2];
        uint32_t dword_value;
        float float_value;
    } map_data_value;

    if (storagetype == "uint16" || storagetype == "int16")
        storagesize = 2;
    else if (storagetype == "uint24" || storagetype == "int24")
        storagesize = 3;
    else if (storagetype == "uint32" || storagetype == "int32" || storagetype == "float")
        storagesize = 4;

    if (storagetype.startsWith("uint"))
    {
        if (storagetype.startsWith("uint8"))
        {
//            if (rom_data_value.toUInt() >= 0 && new_rom_data_value.toUInt() )
        }

    }
    if (storagetype.startsWith("int"))
    {

    }
    if (storagetype.startsWith("float"))
    {

    }

    return result;
}

int MainWindow::test_haltech_ic7_display()
{
    QByteArray output;
    QByteArray received;

    uint16_t temp_base = 2300;

    uint16_t RPM = 0;
    uint16_t MAP = 0;
    uint16_t TPS = 0;

    uint16_t IDC = 0;
    uint16_t IGN = 0;

    uint16_t SPD = 0;
    uint16_t GEAR = 0;

    uint16_t BATT = 0;

    uint16_t CLT = 0;
    uint16_t IAT = 0;
    uint16_t FLT = 0;
    uint16_t OLT = 0;

    int i = 0;

    //serial_poll_timer->stop();
    //ssm_init_poll_timer->stop();
    logging_poll_timer->stop();

    serial->set_is_iso15765_connection(true);
    //serial->set_is_can_connection(true);
    serial->set_is_29_bit_id(false);
    serial->set_can_speed("1000000");

    serial->reset_connection();
    ecuid.clear();
    ecu_init_complete = false;
    open_serial_port();

    qDebug() << "Send data to Haltech IC-7 display";

    while (haltech_ic7_display_on)
    {
        i = 0;
        while (i < 101)
        {
            RPM = i * 60;
            MAP = i * 10;
            TPS = i * 10;

            IDC = i * 10;
            IGN = i * 5;

            SPD = i * 10;
            GEAR = i / 20 + 1;
            BATT = i / 2 + 100;

            CLT = temp_base + i * 10;
            IAT = temp_base + i * 10;
            FLT = temp_base + i * 10;
            OLT = temp_base + i * 10;

            qDebug() << "MAP:" << MAP << "TPS:" << TPS;

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x03);
            output.append((uint8_t)0x60);
            //output.append((uint8_t)0x00 & 0xFF);
            //output.append((uint8_t)0x00 & 0xFF);
            output.append((uint8_t)(RPM >> 8) & 0xFF);
            output.append((uint8_t)RPM & 0xFF);
            output.append((uint8_t)(MAP >> 8) & 0xFF);
            output.append((uint8_t)MAP & 0xFF);
            output.append((uint8_t)(TPS >> 8) & 0xFF);
            output.append((uint8_t)TPS & 0xFF);

            received = serial->write_serial_data_echo_check(output);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x03);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)(CLT >> 8) & 0xFF);
            output.append((uint8_t)CLT & 0xFF);
            output.append((uint8_t)(IAT >> 8) & 0xFF);
            output.append((uint8_t)IAT & 0xFF);
            output.append((uint8_t)(FLT >> 8) & 0xFF);
            output.append((uint8_t)FLT & 0xFF);
            output.append((uint8_t)(OLT >> 8) & 0xFF);
            output.append((uint8_t)OLT & 0xFF);

            received = serial->write_serial_data_echo_check(output);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x03);
            output.append((uint8_t)0x62);
            output.append((uint8_t)(IDC >> 8) & 0xFF);
            output.append((uint8_t)IDC & 0xFF);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)(IGN >> 8) & 0xFF);
            output.append((uint8_t)IGN & 0xFF);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);

            received = serial->write_serial_data_echo_check(output);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x03);
            output.append((uint8_t)0x70);
            output.append((uint8_t)(SPD >> 8) & 0xFF);
            output.append((uint8_t)SPD & 0xFF);
            output.append((uint8_t)(GEAR >> 8) & 0xFF);
            output.append((uint8_t)GEAR & 0xFF);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);

            received = serial->write_serial_data_echo_check(output);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x03);
            output.append((uint8_t)0x72);
            output.append((uint8_t)(BATT >> 8) & 0xFF);
            output.append((uint8_t)BATT & 0xFF);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);

            received = serial->write_serial_data_echo_check(output);

            i++;

            delay(20);
        }
    }

    /*
    output.append((uint8_t)0x03);
    output.append((uint8_t)0x60);
    output.append((uint8_t)0x03);
    output.append((uint8_t)0x61);

    output.append((uint8_t)0x03);
    output.append((uint8_t)0x68);
    output.append((uint8_t)0x03);
    output.append((uint8_t)0x69);

    output.append((uint8_t)0x03);
    output.append((uint8_t)0x73);
    output.append((uint8_t)0x03);
    output.append((uint8_t)0x74);
    output.append((uint8_t)0x03);
    output.append((uint8_t)0x75);
    output.append((uint8_t)0x03);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x03);
    output.append((uint8_t)0xE2);
*/
    serial->set_can_speed("500000");
    //serial_poll_timer->start();
    //ssm_init_poll_timer->start();

    return 0;
}

int MainWindow::simulate_obd()
{
    QByteArray output;
    QByteArray received;

    QByteArray sid3e = { "\x81\xf1\x12\x7e" };

    uint8_t sid_4d_ff_b4[] = { 0x4c, 0x00, 0xb4 };

    uint8_t sid_81[] = { 0x83, 0xf1, 0x12, 0xc1, 0xef, 0x8f };
    uint8_t sid_82[] = { 0x81, 0xf1, 0x12, 0xc2 };
    uint8_t sid_83_00[] = { 0x80, 0xf1, 0x12, 0x07, 0xc3, 0x00, 0x00, 0xef, 0x00, 0x78, 0x00 };
    uint8_t sid_83_02[] = { 0x80, 0xf1, 0x12, 0x07, 0xc3, 0x02, 0x00, 0x28, 0x00, 0x14, 0x00 };
    uint8_t sid_83_03[] = { 0x82, 0xf1, 0x12, 0xc3, 0x03 };

    uint8_t sid_27_01[] = { 0x80, 0xf1, 0x12, 0x05, 0x67, 0x01, 0xb3, 0x59, 0x2c };
    uint8_t sid_27_02[] = { 0x82, 0xf1, 0x12, 0x03, 0x67, 0x01, 0x34 };
    uint8_t sid_27_10[] = { 0x80, 0xf1, 0x12, 0x06, 0x67, 0x10, 0x10, 0x10, 0x10, 0x10 };
    uint8_t sid_27_11[] = { 0x82, 0xf1, 0x12, 0x03, 0x67, 0x11, 0x34 };

    uint8_t sid_1a_90[] = { 0x13, 0x5a, 0x90, 0x57, 0x44, 0x42, 0x32, 0x31, 0x31, 0x32, 0x32, 0x36, 0x31, 0x41, 0x32, 0x39, 0x32, 0x38, 0x36, 0x39 };
    uint8_t sid_21_09[] = { 0x80, 0xf1, 0x12, 0x66, 0x61, 0x09, 0x43, 0x52, 0x33, 0x30, 0x2d, 0x36, 0x34, 0x38, 0x2d, 0x44,
                            0x32, 0x4d, 0x31, 0x2d, 0x53, 0x32, 0x31, 0x31, 0x2d, 0x4d, 0x45, 0x30, 0x34, 0x30, 0x33, 0x2d,
                            0x30, 0x30, 0x31, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00
                          };

    qDebug() << "Simulating OBD communications";

    //serial_poll_timer->stop();
    //ssm_init_poll_timer->stop();
    logging_poll_timer->stop();

    serial->reset_connection();
    ecuid.clear();
    ecu_init_complete = false;

    serial->set_add_iso14230_header(false);
    //serial->set_add_iso14230_header(true);
    open_serial_port();
    //serial->change_port_speed("10400");
    serial->change_port_speed("9600");

    while (simulate_obd_on)
    {
        received.clear();
        output.clear();
        received = serial->read_serial_data(100);
        if (received != "")
        {
            qDebug() << "Received:" << parse_message_to_hex(received);
            // sid_4d_ff_b4
            if ((uint8_t)received.at(0) == 0x4d || (uint8_t)received.at(1) == 0xff || (uint8_t)received.at(2) == 0xb4)
                for (uint8_t i = 0; i < sizeof(sid_4d_ff_b4); i++) output.append((uint8_t)sid_4d_ff_b4[i]);
/*
            if ((uint8_t)received.at(3) == 0x3e || (uint8_t)received.at(4) == 0x3e)
                output = sid3e;

            if ((uint8_t)received.at(3) == 0x81 || (uint8_t)received.at(4) == 0x81)
                for (uint8_t i = 0; i < sizeof(sid_81); i++) output.append((uint8_t)sid_81[i]);
            if ((uint8_t)received.at(3) == 0x82 || (uint8_t)received.at(4) == 0x82)
                for (uint8_t i = 0; i < sizeof(sid_82); i++) output.append((uint8_t)sid_82[i]);
            if (((uint8_t)received.at(3) == 0x83 && (uint8_t)received.at(4) == 0x00) || ((uint8_t)received.at(4) == 0x83 && (uint8_t)received.at(5) == 0x00))
                for (uint8_t i = 0; i < sizeof(sid_83_00); i++) output.append((uint8_t)sid_83_00[i]);
            if (((uint8_t)received.at(3) == 0x83 && (uint8_t)received.at(4) == 0x02) || ((uint8_t)received.at(4) == 0x83 && (uint8_t)received.at(5) == 0x02))
                for (uint8_t i = 0; i < sizeof(sid_83_02); i++) output.append((uint8_t)sid_83_02[i]);
            if (((uint8_t)received.at(3) == 0x83 && (uint8_t)received.at(4) == 0x03) || ((uint8_t)received.at(4) == 0x83 && (uint8_t)received.at(5) == 0x03))
                for (uint8_t i = 0; i < sizeof(sid_83_03); i++) output.append((uint8_t)sid_83_03[i]);

            if (((uint8_t)received.at(3) == 0x27 && (uint8_t)received.at(4) == 0x01) || ((uint8_t)received.at(4) == 0x27 && (uint8_t)received.at(5) == 0x01))
                for (uint8_t i = 0; i < sizeof(sid_27_01); i++) output.append((uint8_t)sid_27_01[i]);
            if (((uint8_t)received.at(3) == 0x27 && (uint8_t)received.at(4) == 0x02) || ((uint8_t)received.at(4) == 0x27 && (uint8_t)received.at(5) == 0x02))
                for (uint8_t i = 0; i < sizeof(sid_27_02); i++) output.append((uint8_t)sid_27_02[i]);
            if (((uint8_t)received.at(3) == 0x27 && (uint8_t)received.at(4) == 0x10) || ((uint8_t)received.at(4) == 0x27 && (uint8_t)received.at(5) == 0x10))
                for (uint8_t i = 0; i < sizeof(sid_27_10); i++) output.append((uint8_t)sid_27_10[i]);
            if (((uint8_t)received.at(3) == 0x27 && (uint8_t)received.at(4) == 0x11) || ((uint8_t)received.at(4) == 0x27 && (uint8_t)received.at(5) == 0x11))
                for (uint8_t i = 0; i < sizeof(sid_27_11); i++) output.append((uint8_t)sid_27_11[i]);

            if ((uint8_t)received.at(1) == 0x1a && (uint8_t)received.at(2) == 0x90)
                for (uint8_t i = 0; i < sizeof(sid_1a_90); i++) output.append((uint8_t)sid_1a_90[i]);

            if ((uint8_t)received.at(3) == 0x21 && (uint8_t)received.at(4) == 0x09)
                for (uint8_t i = 0; i < sizeof(sid_21_09); i++) output.append((uint8_t)sid_21_09[i]);
*/
            if (output != "")
            {
                qDebug() << "Send msg:" << parse_message_to_hex(output);
                received = serial->write_serial_data_echo_check(output);
            }
        }
    }

    // 81 f1 12 7e
    // 81 f1 12 c2
    // 83 f1 12 c1 ef 8f
    // 80 f1 12 07 c3 02 00 28 00 14 00
    // 80 f1 12 07 c3 00 00 ef 00 78 00






    return 0;
}
