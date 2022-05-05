
#include "con_manager.hpp"
#include <mutex>
#include <array>
#include "httplib.h"



// Pretty much All of this code comes from sys-hidplus
// Some of the code comes from hid-mitm

int FakeController::initialize()
{
    if (isInitialized) return 0;
    controllerDevice.deviceType = HidDeviceType_FullKey3; // Pro Controller
    controllerDevice.singleColorBody = RGBA8_MAXALPHA(255,153,204);
    controllerDevice.singleColorButtons = RGBA8_MAXALPHA(0,0,0);
    controllerDevice.colorLeftGrip = RGBA8_MAXALPHA(255,0,127);
    controllerDevice.colorRightGrip = RGBA8_MAXALPHA(255,0,127);
    controllerDevice.npadInterfaceType = HidNpadInterfaceType_Bluetooth;
    controllerState.battery_level = 4; // Set battery charge to full.
    controllerState.analog_stick_l.x = 0x0;
    controllerState.analog_stick_l.y = -0x0;
    controllerState.analog_stick_r.x = 0x0;
    controllerState.analog_stick_r.y = -0x0;
    Result myResult = hiddbgAttachHdlsVirtualDevice(&controllerHandle, &controllerDevice);
    if (R_FAILED(myResult)) {
        return -1;
    }
    isInitialized = true;
    return 0;
}

int FakeController::destruct()
{
    if (!isInitialized) return 0;
    controllerState = {0};
    hiddbgSetHdlsState(controllerHandle, &controllerState);
    hiddbgDetachHdlsVirtualDevice(controllerHandle);
    controllerHandle = {0};
    controllerDevice = {0};
    isInitialized = false;
    return 0;
}

FakeController fakeCon;
u64 buttonPresses;

Result apply_fake_con_state(httplib::Request message) {
    u64 keys = std::stoull(message.get_param_value("keys"));
    // If there is no controller connected, we have to initialize one
    if (!fakeCon.isInitialized) fakeCon.initialize();

    fakeCon.controllerState.buttons = keys;
    Result res = hiddbgSetHdlsState(fakeCon.controllerHandle, &fakeCon.controllerState);
    if R_SUCCEEDED(res) {
        svcSleepThread(100000000); // i want to rest eternally as well.
        fakeCon.controllerState.buttons = 0;
        return hiddbgSetHdlsState(fakeCon.controllerHandle, &fakeCon.controllerState);
    } else return res;
}
