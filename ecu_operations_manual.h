#ifndef ECU_OPERATIONS_MANUAL_H
#define ECU_OPERATIONS_MANUAL_H

#include <QMainWindow>
#include <QSerialPort>
#include <QTime>

#include <kernelcomms.h>
#include <kernelmemorymodels.h>
#include <file_actions.h>
#include <serial_port_actions.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class EcuManualOperationsWindow;
}
QT_END_NAMESPACE

class EcuManualOperations : public QWidget
{
    Q_OBJECT

public:
    explicit EcuManualOperations(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QWidget *parent = nullptr);
    ~EcuManualOperations();

private:
    QString mcu_type_string;

    SerialPortActions *serial;

    QStringList kernels = {
        "ssmk_HC16",
        "ssmk_SH7055_02fxt_35",
        "ssmk_SH7055_18",
        "ssmk_SH7058",
    };

    QStringList flash_methods = {
        "wrx02",
        "fxt02",
        "sti04",
        "sti05",
        "subarucan",
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
    };

    void add_mcu_type_combobox();
    void add_kernels_combobox();
    void add_flash_methods_combobox();

    Ui::EcuManualOperationsWindow *ui;

public slots:

private slots:
    void delay(int n);

};

#endif // ECU_OPERATIONS_MANUAL_H
