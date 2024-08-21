#ifndef PROTOCOL_SELECT_H
#define PROTOCOL_SELECT_H

#include <QWidget>
#include <QStringListModel>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QCheckBox>
#include <QScreen>

#include "file_actions.h"

QT_BEGIN_NAMESPACE
namespace Ui {
    class ProtocolSelect;
}
QT_END_NAMESPACE

class ProtocolSelect : public QDialog
{
    Q_OBJECT

public:
    explicit ProtocolSelect(FileActions::ConfigValuesStructure *configValues, QWidget *parent = nullptr);
    ~ProtocolSelect();

private:
    Ui::ProtocolSelect *ui;
};

#endif // PROTOCOL_SELECT_H
