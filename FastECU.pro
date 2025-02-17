QT       += core gui xml serialport remoteobjects websockets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += charts

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS #QT_SSL

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

win32 {
    #LIBS += -LC:\Qt\5.9.9\mingw53_32\lib\libQt5OpenGL.a -lopengl32 -lsetupapi
    #OpenSSL library must be stated first because in case
    #of static build it cannot be linked static, dynamic only
    QMAKE_LFLAGS += -lcrypto-3
    LIBS += -lopengl32 -lsetupapi
    SOURCES += \
    serial_port/J2534_win.cpp
    HEADERS += \
    serial_port/J2534_win.h
    HEADERS += \
    serial_port/J2534_tactrix_win.h
    INCLUDEPATH += "C:\Program Files (x86)\OpenSSL-Win32\include" "C:\Program Files\OpenSSL-Win32\include"
}
unix {
    LIBS += -lcrypto
    SOURCES += \
    serial_port/J2534_linux.cpp
    HEADERS += \
    serial_port/J2534_linux.h
    HEADERS += \
    serial_port/J2534_tactrix_linux.h \
}

# Do static build for Windows to have only on portable .exe file that
# includes everything. On Linux x86_64 and aarch64 use AppImage and/or Flatpack.
static {
    message(win32 static build)
    # For static build
    # -static enables static builg globally, for all libs
    #CONFIG += -static

    #-Wl,-Bstatic means that -Bstatic is passed to linker
    #and it affects library searching for -l options which follow it.
    QMAKE_LFLAGS += -Wl,-static -static-libgcc -static-libstdc++ -lstdc++
    win32:QMAKE_LFLAGS += \
        -Wl,--whole-archive \
        -lwinpthread \
        -Wl,--no-whole-archive

    DEFINES += STATIC
}

REPC_REPLICA = \
    serial_port/serial_port_actions.rep \
    remote_utility/remote_utility.rep

SOURCES += \
    calibration_maps.cpp \
    calibration_treewidget.cpp \
    cipher.cpp \
    definition_file_convert.cpp \
    ecu_operations.cpp \
    file_actions.cpp \
    file_defs_ecuflash.cpp \
    file_defs_romraider.cpp \
    get_key_operations_subaru.cpp \
    hexcommander.cpp \
    log_operations_ssm.cpp \
    logbox.cpp \
    logvalues.cpp \
    main.cpp \
    mainwindow.cpp \
    menu_actions.cpp \
    modules/bdm/flash_ecu_subaru_denso_mc68hc16y5_02_bdm.cpp \
    modules/biu/biu_operations_subaru.cpp \
    modules/biu/biu_ops_subaru_data.cpp \
    modules/biu/biu_ops_subaru_dtcs.cpp \
    modules/biu/biu_ops_subaru_input1.cpp \
    modules/biu/biu_ops_subaru_input2.cpp \
    modules/biu/biu_ops_subaru_switches.cpp \
    modules/bootmode/flash_ecu_subaru_unisia_jecs_m32r_bootmode.cpp \
    modules/checksum/checksum_ecu_subaru_denso_sh705x_diesel.cpp \
    modules/checksum/checksum_ecu_subaru_denso_sh7xxx.cpp \
    modules/checksum/checksum_ecu_subaru_hitachi_m32r_can.cpp \
    modules/checksum/checksum_ecu_subaru_hitachi_m32r_kline.cpp \
    modules/checksum/checksum_ecu_subaru_hitachi_sh7058.cpp \
    modules/checksum/checksum_ecu_subaru_hitachi_sh72543r.cpp \
    modules/checksum/checksum_tcu_mitsu_mh8104_can.cpp \
    modules/checksum/checksum_tcu_subaru_denso_sh7055.cpp \
    modules/checksum/checksum_tcu_subaru_hitachi_m32r_can.cpp \
    modules/ecu/flash_ecu_subaru_denso_1n83m_1_5m_can.cpp \
    modules/ecu/flash_ecu_subaru_denso_1n83m_4m_can.cpp \
    modules/ecu/flash_ecu_subaru_denso_mc68hc16y5_02.cpp \
    modules/ecu/flash_ecu_subaru_denso_sh7055_02.cpp \
    modules/ecu/flash_ecu_subaru_denso_sh7058_can.cpp \
    modules/ecu/flash_ecu_subaru_denso_sh7058_can_diesel.cpp \
    modules/ecu/flash_ecu_subaru_denso_sh705x_densocan.cpp \
    modules/ecu/flash_ecu_subaru_denso_sh705x_kline.cpp \
    modules/ecu/flash_ecu_subaru_denso_sh72531_can.cpp \
    modules/ecu/flash_ecu_subaru_denso_sh72543_can_diesel.cpp \
    modules/ecu/flash_ecu_subaru_hitachi_m32r_can.cpp \
    modules/ecu/flash_ecu_subaru_hitachi_m32r_kline.cpp \
    modules/ecu/flash_ecu_subaru_hitachi_sh7058_can.cpp \
    modules/ecu/flash_ecu_subaru_hitachi_sh72543r_can.cpp \
    modules/ecu/flash_ecu_subaru_mitsu_m32r_kline.cpp \
    modules/ecu/flash_ecu_subaru_unisia_jecs.cpp \
    modules/ecu/flash_ecu_subaru_unisia_jecs_m32r.cpp \
    modules/eeprom/eeprom_ecu_subaru_denso_sh705x_can.cpp \
    modules/eeprom/eeprom_ecu_subaru_denso_sh705x_kline.cpp \
    modules/jtag/flash_ecu_subaru_hitachi_m32r_jtag.cpp \
    modules/tcu/flash_tcu_cvt_subaru_hitachi_m32r_can.cpp \
    modules/tcu/flash_tcu_cvt_subaru_mitsu_mh8104_can.cpp \
    modules/tcu/flash_tcu_cvt_subaru_mitsu_mh8111_can.cpp \
    modules/tcu/flash_tcu_subaru_denso_sh705x_can.cpp \
    modules/tcu/flash_tcu_subaru_hitachi_m32r_can.cpp \
    modules/tcu/flash_tcu_subaru_hitachi_m32r_kline.cpp \
    protocol_select.cpp \
    remote_utility/remote_utility.cpp \
    serial_port/serial_port_actions.cpp \
    serial_port/serial_port_actions_direct.cpp \
    serial_port/websocketiodevice.cpp \
    settings.cpp \
    systemlogger.cpp \
    vehicle_select.cpp \
    verticallabel.cpp

HEADERS += \
    calibration_maps.h \
    calibration_treewidget.h \
    cipher.h \
    definition_file_convert.h \
    ecu_operations.h \
    file_actions.h \
    get_key_operations_subaru.h \
    hexcommander.h \
    kernelcomms.h \
    kernelmemorymodels.h \
    logbox.h \
    mainwindow.h \
    modules/bdm/flash_ecu_subaru_denso_mc68hc16y5_02_bdm.h \
    modules/biu/biu_operations_subaru.h \
    modules/biu/biu_ops_subaru_data.h \
    modules/biu/biu_ops_subaru_dtcs.h \
    modules/biu/biu_ops_subaru_input1.h \
    modules/biu/biu_ops_subaru_input2.h \
    modules/biu/biu_ops_subaru_switches.h \
    modules/bootmode/flash_ecu_subaru_unisia_jecs_m32r_bootmode.h \
    modules/checksum/checksum_ecu_subaru_denso_sh705x_diesel.h \
    modules/checksum/checksum_ecu_subaru_denso_sh7xxx.h \
    modules/checksum/checksum_ecu_subaru_hitachi_m32r_can.h \
    modules/checksum/checksum_ecu_subaru_hitachi_m32r_kline.h \
    modules/checksum/checksum_ecu_subaru_hitachi_sh7058.h \
    modules/checksum/checksum_ecu_subaru_hitachi_sh72543r.h \
    modules/checksum/checksum_tcu_mitsu_mh8104_can.h \
    modules/checksum/checksum_tcu_subaru_denso_sh7055.h \
    modules/checksum/checksum_tcu_subaru_hitachi_m32r_can.h \
    modules/ecu/flash_ecu_subaru_denso_1n83m_1_5m_can.h \
    modules/ecu/flash_ecu_subaru_denso_1n83m_4m_can.h \
    modules/ecu/flash_ecu_subaru_denso_mc68hc16y5_02.h \
    modules/ecu/flash_ecu_subaru_denso_sh7055_02.h \
    modules/ecu/flash_ecu_subaru_denso_sh7058_can.h \
    modules/ecu/flash_ecu_subaru_denso_sh7058_can_diesel.h \
    modules/ecu/flash_ecu_subaru_denso_sh705x_densocan.h \
    modules/ecu/flash_ecu_subaru_denso_sh705x_kline.h \
    modules/ecu/flash_ecu_subaru_denso_sh72531_can.h \
    modules/ecu/flash_ecu_subaru_denso_sh72543_can_diesel.h \
    modules/ecu/flash_ecu_subaru_hitachi_m32r_can.h \
    modules/ecu/flash_ecu_subaru_hitachi_m32r_kline.h \
    modules/ecu/flash_ecu_subaru_hitachi_sh7058_can.h \
    modules/ecu/flash_ecu_subaru_hitachi_sh72543r_can.h \
    modules/ecu/flash_ecu_subaru_mitsu_m32r_kline.h \
    modules/ecu/flash_ecu_subaru_unisia_jecs.h \
    modules/ecu/flash_ecu_subaru_unisia_jecs_m32r.h \
    modules/eeprom/eeprom_ecu_subaru_denso_sh705x_can.h \
    modules/eeprom/eeprom_ecu_subaru_denso_sh705x_kline.h \
    modules/jtag/flash_ecu_subaru_hitachi_m32r_jtag.h \
    modules/tcu/flash_tcu_cvt_subaru_hitachi_m32r_can.h \
    modules/tcu/flash_tcu_cvt_subaru_mitsu_mh8104_can.h \
    modules/tcu/flash_tcu_cvt_subaru_mitsu_mh8111_can.h \
    modules/tcu/flash_tcu_subaru_denso_sh705x_can.h \
    modules/tcu/flash_tcu_subaru_hitachi_m32r_can.h \
    modules/tcu/flash_tcu_subaru_hitachi_m32r_kline.h \
    protocol_select.h \
    remote_utility/remote_utility.h \
    serial_port/qtrohelper.hpp \
    serial_port/serial_port_actions.h \
    serial_port/serial_port_actions_direct.h \
    serial_port/websocketiodevice.h \
    settings.h \
    systemlogger.h \
    vehicle_select.h \
    verticallabel.h

FORMS += \
    calibration_map_table.ui \
    data_terminal.ui \
    definition_file_convert.ui \
#    ecu_manual_operations.ui \
    ecu_operations.ui \
    logvalues.ui \
    mainwindow.ui \
    modules/biu/biu_operations_subaru.ui \
    modules/biu/biu_ops_subaru_data.ui \
    modules/biu/biu_ops_subaru_dtcs.ui \
    modules/biu/biu_ops_subaru_input1.ui \
    modules/biu/biu_ops_subaru_input2.ui \
    modules/biu/biu_ops_subaru_switches.ui \
    protocol_select.ui \
    settings.ui \
    vehicle_select.ui

# Default rules for deployment.
flatpak {
    target.path = /app/bin
    !isEmpty(target.path): INSTALLS += target
} else {
    qnx: target.path = /tmp/$${TARGET}/bin
    else: unix:!android: target.path = /opt/$${TARGET}/bin
    !isEmpty(target.path): INSTALLS += target
}

RESOURCES += \
    config.qrc \
    icons.qrc \
    images.qrc \
    kernels.qrc

DISTFILES += \
    LICENSE \
    README.md \
    USING.txt \
    appdata/fastecu_128x128.png \
    appdata/fastecu_16x16.png \
    appdata/fastecu_256x256.png \
    appdata/fastecu_32x32.png \
    appdata/fastecu_64x64.png \
    appdata/fi.fastecu.fastecu.desktop \
    appdata/fi.fastecu.fastecu.metainfo.xml \
    config/fastecu.cfg \
    config/logger.cfg \
    config/menu.cfg \
    config/protocols.cfg \
    external/FastECU-mc68hc16-bdm.ino \
    fastecu.rc \
    flatpak/fi.fastecu.FastECU.desktop \
    flatpak/fi.fastecu.FastECU.ico \
    flatpak/fi.fastecu.FastECU.metainfo.xml \
    flatpak/fi.fastecu.FastECU.yml \
    flatpak/fi.fastecu.FastECU_128x128.png \
    flatpak/fi.fastecu.FastECU_16x16.png \
    flatpak/fi.fastecu.FastECU_256x256.png \
    flatpak/fi.fastecu.FastECU_32x32.png \
    flatpak/fi.fastecu.FastECU_64x64.png \
    kernels/ssmk_can_sh7055.bin \
    kernels/ssmk_can_sh7058.bin \
    kernels/ssmk_can_sh7059d_euro5.bin \
    kernels/ssmk_can_tp_sh7058.bin \
    kernels/ssmk_can_tp_sh7058d_euro4.bin \
    kernels/ssmk_can_tp_sh7059d_euro5.bin \
    kernels/ssmk_kline_sh7055.bin \
    kernels/ssmk_kline_sh7058.bin \
    kernels/ssmk_mc68hc916y5.bin \
    kernels/ssmk_tcu_can_sh7055_35.bin \
    kernels/ssmk_tcu_can_sh7058.bin

RC_FILE = fastecu.rc
