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

public:
    explicit GetKeyOperationsSubaru(QWidget *parent = nullptr);
    ~GetKeyOperationsSubaru();

private:
    void closeEvent(QCloseEvent *bar);

    bool kill_process = false;

    int linear_approx_test(void);
    int encrypt_and_apply_linear_approx();
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

public slots:

private slots:
    int send_log_window_message(QString message, bool timestamp, bool linefeed);

};

#endif // GETKEYOPERATIONSSUBARU_H
