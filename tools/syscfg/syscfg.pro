QT += core dbus
QT -= gui

CONFIG += c++11

TARGET = deepin-system-monitor-syscfg-tool
CONFIG += console
CONFIG -= app_bundle

CONFIG(release, debug|release) {
        DEFINES += QT_NO_DEBUG_OUTPUT
}

TEMPLATE = app

SOURCES += main.cpp

target.path = $${PREFIX}/bin

isEmpty(BINDIR):BINDIR=$${PREFIX}/bin
isEmpty(POLICYDIR):POLICYDIR=$${PREFIX}/share/polkit-1/actions
target.path = $$INSTROOT$$BINDIR
policy.path = $$INSTROOT$$POLICYDIR

# Automating generation .qm files from .ts files
CONFIG(release, debug|release) {
    # convert policy_{lang}.ts to .policy
    !system(deepin-policy-ts-convert init com.deepin.pkexec.$${TARGET}.policy translations): error("Failed policy to ts")
    !system(deepin-policy-ts-convert policy2ts com.deepin.pkexec.$${TARGET}.policy translations): error("Failed policy to ts")
    system(deepin-policy-ts-convert ts2policy com.deepin.pkexec.$${TARGET}.policy translations com.deepin.pkexec.$${TARGET}.policy.tmp) {
        system(mv com.deepin.pkexec.$${TARGET}.policy.tmp com.deepin.pkexec.$${TARGET}.policy)
    }
}

policy.files = com.deepin.pkexec.$${TARGET}.policy

INSTALLS += target policy
