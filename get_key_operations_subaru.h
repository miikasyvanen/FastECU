#ifndef GETKEYOPERATIONSSUBARU_H
#define GETKEYOPERATIONSSUBARU_H

#include <QFileDialog>
#include <QMessageBox>

#include <ecu_operations.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class EcuOperationsWindow;
}
QT_END_NAMESPACE

class GetKeyOperationsSubaru : public QDialog
{
    Q_OBJECT

signals:
    void LOG_E(QString message, bool timestamp, bool linefeed);
    void LOG_W(QString message, bool timestamp, bool linefeed);
    void LOG_I(QString message, bool timestamp, bool linefeed);
    void LOG_D(QString message, bool timestamp, bool linefeed);

public:
    explicit GetKeyOperationsSubaru(QWidget *parent = nullptr);
    ~GetKeyOperationsSubaru();

private:
    void closeEvent(QCloseEvent *bar);

    bool kill_process = false;

    int linear_approx_test(void);
    int load_and_apply_linear_approx();
    uint8_t get_bit(uint32_t value, int bit_num);
    uint16_t applyMask(uint16_t value, uint16_t mask);
    uint16_t sBox(uint16_t sBoxInput);
    uint32_t roundAndFlip(uint32_t input, uint16_t keyInput);
    uint32_t manyRoundAndFlip(uint32_t input, uint16_t *keys, int numRounds);
    uint32_t flipLeftRight(uint32_t flipInput);
    uint32_t roundFunction(uint32_t roundInput, uint16_t keyInput);
    uint16_t fFunction(uint16_t wordInput, uint16_t keyInput);
    void findApprox(uint16_t **approxTable);

    EcuOperations *ecuOperations;
    Ui::EcuOperationsWindow *ui;

};

#endif // GETKEYOPERATIONSSUBARU_H
