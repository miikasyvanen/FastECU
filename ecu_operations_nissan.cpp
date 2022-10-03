#include "ecu_operations_nissan.h"

EcuOperationsNissan::EcuOperationsNissan()
{

}

EcuOperationsNissan::~EcuOperationsNissan()
{

}

void EcuOperationsNissan::delay(int n)
{
    QTime dieTime = QTime::currentTime().addMSecs(n);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}
