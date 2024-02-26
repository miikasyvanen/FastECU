QT       += core gui xml serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += charts

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

win32 {
    LIBS += -LC:\Qt\5.9.9\mingw53_32\lib\libQt5OpenGL.a -lopengl32
    SOURCES += \
    J2534_win.cpp
    HEADERS += \
    J2534_win.h
    HEADERS += \
    J2534_tactrix_win.h
}
linux {
    SOURCES += \
    J2534_linux.cpp
    HEADERS += \
    J2534_linux.h
    HEADERS += \
    J2534_tatrix_linux.h \
}

SOURCES += \
    biu_operations_subaru.cpp \
    biu_ops_subaru_dtcs.cpp \
    biu_ops_subaru_data.cpp \
    biu_ops_subaru_input1.cpp \
    biu_ops_subaru_input2.cpp \
    biu_ops_subaru_switches.cpp \
    calibration_maps.cpp \
    calibration_treewidget.cpp \
    definition_file_convert.cpp \
    ecu_operations.cpp \
    ecu_operations_iso14230.cpp \
    ecu_operations_manual.cpp \
    ecu_operations_mercedes.cpp \
    ecu_operations_nissan.cpp \
    ecu_operations_subaru.cpp \
    file_actions.cpp \
    file_defs_ecuflash.cpp \
    file_defs_romraider.cpp \
    log_operations_ssm.cpp \
    logbox.cpp \
    logvalues.cpp \
    main.cpp \
    mainwindow.cpp \
    menu_actions.cpp \
    modules/flash_ecu_subaru_denso_mc68hc16y5_02.cpp \
    modules/flash_ecu_subaru_denso_sh7055_02.cpp \
    modules/flash_ecu_subaru_denso_sh7055_04.cpp \
    modules/flash_ecu_subaru_denso_sh7058_can.cpp \
    modules/flash_ecu_subaru_denso_sh7058_can_diesel.cpp \
    modules/flash_ecu_subaru_denso_sh705x_can.cpp \
    modules/flash_ecu_subaru_hitachi_m32r_02.cpp \
    modules/flash_ecu_subaru_hitachi_m32r_06.cpp \
    modules/flash_ecu_subaru_hitachi_m32r_can.cpp \
    modules/flash_ecu_subaru_uinisia_jecs_m32r.cpp \
    modules/flash_tcu_subaru_denso_sh705x_can.cpp \
    modules/flash_tcu_subaru_hitachi_m32r_can.cpp \
    modules/flash_tcu_subaru_hitachi_m32r_kline.cpp \
    protocol_select.cpp \
    serial_port_actions.cpp \
    settings.cpp \
    verticallabel.cpp

HEADERS += \
    biu_operations_subaru.h \
    biu_ops_subaru_dtcs.h \
    biu_ops_subaru_data.h \
    biu_ops_subaru_input1.h \
    biu_ops_subaru_input2.h \
    biu_ops_subaru_switches.h \
    calibration_maps.h \
    calibration_treewidget.h \
    definition_file_convert.h \
    ecu_operations.h \
    ecu_operations_iso14230.h \
    ecu_operations_manual.h \
    ecu_operations_manual.h \
    ecu_operations_mercedes.h \
    ecu_operations_nissan.h \
    ecu_operations_subaru.h \
    file_actions.h \
    kernelcomms.h \
    kernelmemorymodels.h \
    logbox.h \
    mainwindow.h \
    modules/flash_ecu_subaru_denso_mc68hc16y5_02.h \
    modules/flash_ecu_subaru_denso_sh7055_02.h \
    modules/flash_ecu_subaru_denso_sh7055_04.h \
    modules/flash_ecu_subaru_denso_sh7058_can.h \
    modules/flash_ecu_subaru_denso_sh7058_can_diesel.h \
    modules/flash_ecu_subaru_denso_sh705x_can.h \
    modules/flash_ecu_subaru_hitachi_m32r_02.h \
    modules/flash_ecu_subaru_hitachi_m32r_06.h \
    modules/flash_ecu_subaru_hitachi_m32r_can.h \
    modules/flash_ecu_subaru_uinisia_jecs_m32r.h \
    modules/flash_tcu_subaru_denso_sh705x_can.h \
    modules/flash_tcu_subaru_hitachi_m32r_can.h \
    modules/flash_tcu_subaru_hitachi_m32r_kline.h \
    protocol_select.h \
    serial_port_actions.h \
    settings.h \
    verticallabel.h

FORMS += \
    biu_operations_subaru.ui \
    biu_ops_subaru_dtcs.ui \
    biu_ops_subaru_data.ui \
    biu_ops_subaru_input1.ui \
    biu_ops_subaru_input2.ui \
    biu_ops_subaru_switches.ui \
    calibration_map_table.ui \
    definition_file_convert.ui \
    ecu_manual_operations.ui \
    ecu_operations.ui \
    logvalues.ui \
    mainwindow.ui \
    protocol_select.ui \
    settings.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    icons.qrc

DISTFILES += \
    LICENSE \
    README.md \
    USING.txt \
    config/fastecu.cfg \
    config/logger.cfg \
    config/menu.cfg \
    config/protocols.cfg \
    definitions/16BITBASE.xml \
    definitions/32BITBASE.xml \
    definitions/A2ZJ601A.xml \
    definitions/A4SE700D.xml \
    definitions/A8DH200Z.xml \
    definitions/ecu_defs.xml \
    definitions/logger_METRIC_EN_v370.xml \
    fastecu.rc \
    kernels/ssmk_CAN_SH7055.bin \
    kernels/ssmk_CAN_SH7058.bin \
    kernels/ssmk_HC16.bin \
    kernels/ssmk_HC16_decryp.bin \
    kernels/ssmk_SH7055.bin \
    kernels/ssmk_SH7058.bin \
    kernels/ssmk_SH7058_EEPSCI3.bin \
    kernels/ssmk_SH7058_EEPSCI4.bin \
    precompiled/FastECU-Linux.zip \
    precompiled/FastECU-Win7-64bit.zip \
    precompiled/FastECU-Windows.zip

RC_FILE = fastecu.rc
