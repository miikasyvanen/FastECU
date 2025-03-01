#ifndef HEXEDIT_H
#define HEXEDIT_H

#include <QWidget>
#include <QStringList>
#include <QTextCodec>
#include <QScrollBar>
#include <QStandardItemModel>

#include <file_actions.h>
#include <kernelmemorymodels.h>
#include "ui_hexedit.h"

namespace Ui {
class HexEdit;
}

class HexEdit : public QDialog
{
    Q_OBJECT

public:
    explicit HexEdit(FileActions::EcuCalDefStructure *ecuCalDef, QWidget *parent = nullptr);
    ~HexEdit();

    void run();

private:
    FileActions::EcuCalDefStructure *ecuCalDef;

    int cursorPrevX = 0;
    int cursorPrevY = 0;
    int cursorCurrX = 0;
    int cursorCurrY = 0;

    int mcu_type_index;
    QString mcu_type_string;

    Ui::HexEdit *ui;

private slots:
    void getCursorPosition();
};

#endif // HEXEDIT_H
