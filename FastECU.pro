QT       += core gui xml serialport remoteobjects websockets

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
    LIBS += -LC:\Qt\5.9.9\mingw53_32\lib\libQt5OpenGL.a -lopengl32 -lsetupapi
    SOURCES += \
    serial_port/J2534_win.cpp
    HEADERS += \
    serial_port/J2534_win.h
    HEADERS += \
    serial_port/J2534_tactrix_win.h
}
linux {
    SOURCES += \
    serial_port/J2534_linux.cpp
    HEADERS += \
    serial_port/J2534_linux.h
    HEADERS += \
    serial_port/J2534_tactrix_linux.h \
}

REPC_REPLICA = \
    serial_port/serial_port_actions.rep \
    remote_utility/remote_utility.rep

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
    file_actions.cpp \
    file_defs_ecuflash.cpp \
    file_defs_romraider.cpp \
    get_key_operations_subaru.cpp \
    log_operations_ssm.cpp \
    logbox.cpp \
    logvalues.cpp \
    main.cpp \
    mainwindow.cpp \
    menu_actions.cpp \
    modules/checksum_ecu_subaru_denso_sh705x_diesel.cpp \
    modules/checksum_ecu_subaru_denso_sh7xxx.cpp \
    modules/checksum_ecu_subaru_hitachi_m32r.cpp \
    modules/checksum_ecu_subaru_hitachi_sh7058.cpp \
    modules/checksum_ecu_subaru_hitachi_sh72543r.cpp \
    modules/checksum_tcu_mitsu_mh8104_can.cpp \
    modules/checksum_tcu_subaru_denso_sh7055.cpp \
    modules/checksum_tcu_subaru_hitachi_m32r_can.cpp \
    modules/eeprom_ecu_subaru_denso_sh705x_can.cpp \
    modules/eeprom_ecu_subaru_denso_sh705x_kline.cpp \
    modules/flash_ecu_subaru_denso_mc68hc16y5_02.cpp \
    modules/flash_ecu_subaru_denso_sh7055_02.cpp \
    modules/flash_ecu_subaru_denso_sh7058_can.cpp \
    modules/flash_ecu_subaru_denso_sh7058_can_diesel.cpp \
    modules/flash_ecu_subaru_denso_sh705x_densocan.cpp \
    modules/flash_ecu_subaru_denso_sh705x_kline.cpp \
    modules/flash_ecu_subaru_denso_sh72531_can.cpp \
    modules/flash_ecu_subaru_denso_sh7xxx_densocan.cpp \
    modules/flash_ecu_subaru_hitachi_m32r_can.cpp \
    modules/flash_ecu_subaru_hitachi_m32r_kline.cpp \
    modules/flash_ecu_subaru_hitachi_sh7058_can.cpp \
    modules/flash_ecu_subaru_hitachi_sh72543r_can.cpp \
    modules/flash_ecu_subaru_mitsu_m32r_kline.cpp \
    modules/flash_ecu_subaru_unisia_jecs.cpp \
    modules/flash_ecu_subaru_unisia_jecs_m32r.cpp \
    modules/flash_tcu_cvt_subaru_hitachi_m32r_can.cpp \
    modules/flash_tcu_cvt_subaru_mitsu_mh8104_can.cpp \
    modules/flash_tcu_cvt_subaru_mitsu_mh8111_can.cpp \
    modules/flash_tcu_subaru_denso_sh705x_can.cpp \
    modules/flash_tcu_subaru_hitachi_m32r_can.cpp \
    modules/flash_tcu_subaru_hitachi_m32r_kline.cpp \
    modules/unbrick/flash_ecu_subaru_unisia_jecs_m32r_boot_mode.cpp \
    modules/unbrick/flash_ecu_unbrick_subaru_denso_mc68hc16y5_02.cpp \
    protocol_select.cpp \
    remote_utility/remote_utility.cpp \
    serial_port/serial_port_actions.cpp \
    serial_port/serial_port_actions_direct.cpp \
    serial_port/websocketiodevice.cpp \
    settings.cpp \
    vehicle_select.cpp \
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
    file_actions.h \
    get_key_operations_subaru.h \
    kernelcomms.h \
    kernelmemorymodels.h \
    logbox.h \
    mainwindow.h \
    modules/checksum_ecu_subaru_denso_sh705x_diesel.h \
    modules/checksum_ecu_subaru_denso_sh7xxx.h \
    modules/checksum_ecu_subaru_hitachi_m32r.h \
    modules/checksum_ecu_subaru_hitachi_sh7058.h \
    modules/checksum_ecu_subaru_hitachi_sh72543r.h \
    modules/checksum_tcu_mitsu_mh8104_can.h \
    modules/checksum_tcu_subaru_denso_sh7055.h \
    modules/checksum_tcu_subaru_hitachi_m32r_can.h \
    modules/eeprom_ecu_subaru_denso_sh705x_can.h \
    modules/eeprom_ecu_subaru_denso_sh705x_kline.h \
    modules/flash_ecu_subaru_denso_mc68hc16y5_02.h \
    modules/flash_ecu_subaru_denso_sh7055_02.h \
    modules/flash_ecu_subaru_denso_sh7058_can.h \
    modules/flash_ecu_subaru_denso_sh7058_can_diesel.h \
    modules/flash_ecu_subaru_denso_sh705x_densocan.h \
    modules/flash_ecu_subaru_denso_sh705x_kline.h \
    modules/flash_ecu_subaru_denso_sh72531_can.h \
    modules/flash_ecu_subaru_denso_sh7xxx_densocan.h \
    modules/flash_ecu_subaru_hitachi_m32r_can.h \
    modules/flash_ecu_subaru_hitachi_m32r_kline.h \
    modules/flash_ecu_subaru_hitachi_sh7058_can.h \
    modules/flash_ecu_subaru_hitachi_sh72543r_can.h \
    modules/flash_ecu_subaru_mitsu_m32r_kline.h \
    modules/flash_ecu_subaru_unisia_jecs.h \
    modules/flash_ecu_subaru_unisia_jecs_m32r.h \
    modules/flash_tcu_cvt_subaru_hitachi_m32r_can.h \
    modules/flash_tcu_cvt_subaru_mitsu_mh8104_can.h \
    modules/flash_tcu_cvt_subaru_mitsu_mh8111_can.h \
    modules/flash_tcu_subaru_denso_sh705x_can.h \
    modules/flash_tcu_subaru_hitachi_m32r_can.h \
    modules/flash_tcu_subaru_hitachi_m32r_kline.h \
    modules/unbrick/flash_ecu_subaru_unisia_jecs_m32r_boot_mode.h \
    modules/unbrick/flash_ecu_unbrick_subaru_denso_mc68hc16y5_02.h \
    protocol_select.h \
    remote_utility/remote_utility.h \
    serial_port/qtrohelper.hpp \
    serial_port/serial_port_actions.h \
    serial_port/serial_port_actions_direct.h \
    serial_port/websocketiodevice.h \
    settings.h \
    vehicle_select.h \
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
#    ecu_manual_operations.ui \
    ecu_operations.ui \
    logvalues.ui \
    mainwindow.ui \
    protocol_select.ui \
    settings.ui \
    vehicle_select.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    icons.qrc \
    images.qrc

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
    external/FastECU-mc68hc16-flasher.zip \
    fastecu.rc \
    kernels/ssmk_CAN_SH7055.bin \
    kernels/ssmk_CAN_SH7058.bin \
    kernels/ssmk_HC16.bin \
    kernels/ssmk_HC16_decryp.bin \
    kernels/ssmk_SH7055.bin \
    kernels/ssmk_SH7058.bin \
    kernels/ssmk_SH7058_EEPSCI3.bin \
    kernels/ssmk_SH7058_EEPSCI4.bin \
    kernels/ssmk_can_sh7055.bin \
    kernels/ssmk_can_sh7058.bin \
    kernels/ssmk_can_sh7059d_euro5.bin \
    kernels/ssmk_can_tp_sh7058.bin \
    kernels/ssmk_can_tp_sh7058d_euro4.bin \
    kernels/ssmk_can_tp_sh7059d_euro5.bin \
    kernels/ssmk_hc16.bin \
    kernels/ssmk_hc16_decryp.bin \
    kernels/ssmk_kline_sh7055.bin \
    kernels/ssmk_kline_sh7058.bin \
    kernels/ssmk_mc68hc916y5.bin \
    kernels/ssmk_tcu_can_sh7055_35.bin \
    kernels/ssmk_tcu_can_sh7058.bin \
    precompiled/FastECU-Linux.zip \
    precompiled/FastECU-Win7-64bit.zip \
    precompiled/FastECU-Windows.zip

RC_FILE = fastecu.rc
