#ifndef DBUS_COMMON_H
#define DBUS_COMMON_H

#include <QMap>
#include <QVariant>
#include <QtDBus>
#include <QtGlobal>

namespace DBus {
namespace Common {

constexpr const char* UnitTypeServiceSuffix = ".service";

enum DBusSignatureType {
    kDBusByteType,
    kDBusBooleanType,
    kDBusInt16Type,
    kDBusUInt16Type,
    kDBusInt32Type,
    kDBusUInt32Type,
    kDBusInt64Type,
    kDBusUInt64Type,
    kDBusDoubleType,
    kDBusStringType,
    kDBusObjectType,
    kDBusSignatureType,
    kDBusArrayType,
    kDBusStructStartType,
    kDBusStructEndType,
    kDBusVariantType,
    kDBusMapStartType,
    kDBusMapEndType,
    kDBusFDType,

    kDBusInvalidType
};

// Not a valid type code, used to terminate signatures
const char kDBusInvalidTypeSignature = '\0';
// 8-bit unsigned integer
const char kDBusByteTypeSignature = 'y';
// Boolean value, 0 is FALSE and 1 is TRUE. Everything else is invalid.
const char kDBusBooleanTypeSignature = 'b';
// 16-bit signed integer
const char kDBusInt16TypeSignature = 'n';
// 16-bit unsigned integer
const char kDBusUInt16TypeSignature = 'q';
// 32-bit signed integer
const char kDBusInt32TypeSignature = 'i';
// 32-bit unsigned integer
const char kDBusUInt32TypeSignature = 'u';
// 64-bit signed integer
const char kDBusInt64TypeSignature = 'x';
// 64-bit unsigned integer
const char kDBusUInt64TypeSignature = 't';
// IEEE 754 double
const char kDBusDoubleTypeSignature = 'd';
// UTF-8 string (must be valid UTF-8). Must be nul terminated and contain no other nul bytes.
const char kDBusStringTypeSignature = 's';
// Name of an object instance
const char kDBusObjectTypeSignature = 'o';
// A type signature
const char kDBusSignatureTypeSignature = 'g';
// Array
const char kDBusArrayTypeSignature = 'a';
// Struct
const char kDBusStructStartTypeSignature = '(';
const char kDBusStructEndTypeSignature = ')';
// Variant type (the type of the value is part of the value itself)
const char kDBusVariantTypeSignature = 'v';
// Entry in a dict or map (array of key-value pairs)
const char kDBusMapStartTypeSignature = '{';
const char kDBusMapEndTypeSignature = '}';
// Unix file descriptor
const char kDBusFDTypeSignature = 'h';

const QMap<char, DBusSignatureType> DBusSignatureToTypeMap = {
    {kDBusInvalidTypeSignature, kDBusInvalidType},
    {kDBusByteTypeSignature, kDBusByteType},
    {kDBusBooleanTypeSignature, kDBusBooleanType},
    {kDBusInt16TypeSignature, kDBusInt16Type},
    {kDBusUInt16TypeSignature, kDBusUInt16Type},
    {kDBusInt32TypeSignature, kDBusInt32Type},
    {kDBusUInt32TypeSignature, kDBusUInt32Type},
    {kDBusInt64TypeSignature, kDBusInt64Type},
    {kDBusUInt64TypeSignature, kDBusUInt64Type},
    {kDBusDoubleTypeSignature, kDBusDoubleType},
    {kDBusStringTypeSignature, kDBusStringType},
    {kDBusObjectTypeSignature, kDBusObjectType},
    {kDBusSignatureTypeSignature, kDBusSignatureType},
    {kDBusArrayTypeSignature, kDBusArrayType},
    {kDBusStructStartTypeSignature, kDBusStructStartType},
    {kDBusStructEndTypeSignature, kDBusStructEndType},
    {kDBusVariantTypeSignature, kDBusVariantType},
    {kDBusMapStartTypeSignature, kDBusMapStartType},
    {kDBusMapEndTypeSignature, kDBusMapEndType},
    {kDBusFDTypeSignature, kDBusFDType}};

const QMap<DBusSignatureType, char> DBusTypeToSignatureMap = {
    {kDBusInvalidType, kDBusInvalidTypeSignature},
    {kDBusByteType, kDBusByteTypeSignature},
    {kDBusBooleanType, kDBusBooleanTypeSignature},
    {kDBusInt16Type, kDBusInt16TypeSignature},
    {kDBusUInt16Type, kDBusUInt16TypeSignature},
    {kDBusInt32Type, kDBusInt32TypeSignature},
    {kDBusUInt32Type, kDBusUInt32TypeSignature},
    {kDBusInt64Type, kDBusInt64TypeSignature},
    {kDBusUInt64Type, kDBusUInt64TypeSignature},
    {kDBusDoubleType, kDBusDoubleTypeSignature},
    {kDBusStringType, kDBusStringTypeSignature},
    {kDBusObjectType, kDBusObjectTypeSignature},
    {kDBusSignatureType, kDBusSignatureTypeSignature},
    {kDBusArrayType, kDBusArrayTypeSignature},
    {kDBusStructStartType, kDBusStructStartTypeSignature},
    {kDBusStructEndType, kDBusStructEndTypeSignature},
    {kDBusVariantType, kDBusVariantTypeSignature},
    {kDBusMapStartType, kDBusMapStartTypeSignature},
    {kDBusMapEndType, kDBusMapEndTypeSignature},
    {kDBusFDType, kDBusFDTypeSignature}};

// unit load states
enum UnitLoadState {
    kUnitStubState,
    kUnitLoadedState,
    kUnitNotFoundState,
    kUnitBadSettingState,
    kUnitErrorState,
    kUnitMergedState,
    kUnitMaskedState
};
constexpr const char* kUnitStubStateText = QT_TRANSLATE_NOOP("DBus.Unit.Load.State", "stub");
constexpr const char* kUnitLoadedStateText = QT_TRANSLATE_NOOP("DBus.Unit.Load.State", "Loaded");
constexpr const char* kUnitNotFoundStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Load.State", "not-found");
constexpr const char* kUnitBadSettingStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Load.State", "bad-setting");
constexpr const char* kUnitErrorStateText = QT_TRANSLATE_NOOP("DBus.Unit.Load.State", "error");
constexpr const char* kUnitMergedStateText = QT_TRANSLATE_NOOP("DBus.Unit.Load.State", "merged");
constexpr const char* kUnitMaskedStateText = QT_TRANSLATE_NOOP("DBus.Unit.Load.State", "masked");

// unit active states
enum UnitActiveState {
    kUnitActiveState,
    kUnitReloadingState,
    kUnitInactiveState,
    kUnitFailedState,
    kUnitActivatingState,
    kUnitDeactivatingState
};
constexpr const char* kUnitActiveStateText = QT_TRANSLATE_NOOP("DBus.Unit.Active.State", "active");
constexpr const char* kUnitReloadingStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Active.State", "reloading");
constexpr const char* kUnitInactiveStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Active.State", "inactive");
constexpr const char* kUnitFailedStateText = QT_TRANSLATE_NOOP("DBus.Unit.Active.State", "failed");
constexpr const char* kUnitActivatingStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Active.State", "activating");
constexpr const char* kUnitDeactivatingStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Active.State", "deactivating");

// service unit substates
enum ServiceUnitSubState {
    kServiceUnitDeadSubState,
    kServiceUnitStartPreSubState,
    kServiceUnitStartSubState,
    kServiceUnitStartPostSubState,
    kServiceUnitRunningSubState,
    kServiceUnitExitedSubState,
    kServiceUnitReloadSubState,
    kServiceUnitStopSubState,
    kServiceUnitStopWatchdogSubState,
    kServiceUnitStopSigTermSubState,
    kServiceUnitStopSigKillSubState,
    kServiceUnitStopPostSubState,
    kServiceUnitFinalSigTermSubState,
    kServiceUnitFinalSigKillSubState,
    kServiceUnitFailedSubState,
    kServiceUnitAutoRestartSubState
};
constexpr const char* kServiceUnitDeadSubStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Sub.State", "dead");
constexpr const char* kServiceUnitStartPreSubStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Sub.State", "start-pre");
constexpr const char* kServiceUnitStartSubStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Sub.State", "start");
constexpr const char* kServiceUnitStartPostSubStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Sub.State", "start-post");
constexpr const char* kServiceUnitRunningSubStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Sub.State", "running");
constexpr const char* kServiceUnitExitedSubStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Sub.State", "exited");
constexpr const char* kServiceUnitReloadSubStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Sub.State", "reload");
constexpr const char* kServiceUnitStopSubStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Sub.State", "stop");
constexpr const char* kServiceUnitStopWatchdogSubStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Sub.State", "stop-watchdog");
constexpr const char* kServiceUnitStopSigTermSubStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Sub.State", "stop-sigterm");
constexpr const char* kServiceUnitStopSigKillSubStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Sub.State", "stop-sigkill");
constexpr const char* kServiceUnitStopPostSubStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Sub.State", "stop-post");
constexpr const char* kServiceUnitFinalSigTermSubStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Sub.State", "final-sigterm");
constexpr const char* kServiceUnitFinalSigKillSubStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Sub.State", "final-sigkill");
constexpr const char* kServiceUnitFailedSubStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Sub.State", "failed");
constexpr const char* kServiceUnitAutoRestartSubStateText =
    QT_TRANSLATE_NOOP("DBus.Unit.Sub.State", "auto-restart");

enum ServiceStatus {
    kServiceEnabled,
    kServiceDisabled,
    kServiceStatic,
    kServiceTransient,
    kServiceIndirect,
    kServiceEnabledRuntime
};
constexpr const char* kServiceEnabledText = QT_TRANSLATE_NOOP("DBus.Unit.State", "enabled");
constexpr const char* kServiceDisabledText = QT_TRANSLATE_NOOP("DBus.Unit.State", "disabled");
constexpr const char* kServiceStaticText = QT_TRANSLATE_NOOP("DBus.Unit.State", "static");
constexpr const char* kServiceTransientText = QT_TRANSLATE_NOOP("DBus.Unit.State", "transient");
constexpr const char* kServiceIndirectText = QT_TRANSLATE_NOOP("DBus.Unit.State", "indirect");
constexpr const char* kServiceEnabledRuntimeText =
    QT_TRANSLATE_NOOP("DBus.Unit.State", "enabled-runtime");

enum UnitControlJobMode {
    kJobModeFail,
    kJobModeReplace,
    kJobModeReplaceIrreversibly,
    kJobModeIsolate,
    kJobModeIgnoreDependencies,
    kJobModeIgnoreRequirements,
    kJobModeFlush
};
constexpr const char* kJobModeFailCMD = "fail";
constexpr const char* kJobModeReplaceCMD = "replace";
constexpr const char* kJobModeReplaceIrreversiblyCMD = "replace-irreversibly";
constexpr const char* kJobModeIsolateCMD = "isolate";
constexpr const char* kJobModeIgnoreDependenciesCMD = "ignore-dependencies";
constexpr const char* kJobModeIgnoreRequirementsCMD = "ignore-requirements";
constexpr const char* kJobModeFlushCMD = "flush";

const QMap<int, const char*> UnitControlJobModeMap = {
    {kJobModeFail, kJobModeFailCMD},
    {kJobModeReplace, kJobModeReplaceCMD},
    {kJobModeReplaceIrreversibly, kJobModeReplaceIrreversiblyCMD},
    {kJobModeIsolate, kJobModeIsolateCMD},
    {kJobModeIgnoreDependencies, kJobModeIgnoreDependenciesCMD},
    {kJobModeIgnoreRequirements, kJobModeIgnoreRequirementsCMD},
    {kJobModeFlush, kJobModeFlushCMD}};

}  // namespace Common
}  // namespace DBus

#endif  // DBUS_COMMON_H
