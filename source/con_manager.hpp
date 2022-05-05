#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <httplib.h>
#include <switch.h>

int printToFile(const char* myString);

class FakeController {
public:
    HiddbgHdlsHandle controllerHandle = {0};
    HiddbgHdlsDeviceInfo controllerDevice = {0};
    HiddbgHdlsState controllerState = {0};
    int initialize();
    int destruct();
    bool isInitialized = false;
    
};

Result apply_fake_con_state(httplib::Request message);