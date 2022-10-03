#ifndef ECU_OPERATIONS_NISSAN_H
#define ECU_OPERATIONS_NISSAN_H

#include <QCoreApplication>
#include <QByteArray>
#include <QDebug>
#include <QTime>
#include <QTimer>
#include <QWidget>

#include <kernelcomms.h>
#include <kernelmemorymodels.h>

class EcuOperationsNissan : public QWidget
{
    Q_OBJECT

public:
    explicit EcuOperationsNissan();
    ~EcuOperationsNissan();

private:

public slots:

private slots:
    void delay(int n);

};

#endif // ECU_OPERATIONS_NISSAN_H
