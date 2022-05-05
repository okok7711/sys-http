#include "game-reader.hpp"
#include "httplib.h"
#include "utils.hpp"
#include <memory>
#include <switch.h>

extern "C" {
    #define HEAP_SIZE 0x000340000

    u32 __nx_applet_type = AppletType_None;
    char fake_heap[HEAP_SIZE];

    void __libnx_initheap(void) {
        extern char *fake_heap_start;
        extern char *fake_heap_end;

        fake_heap_start = fake_heap;
        fake_heap_end = fake_heap + HEAP_SIZE;
    }

void __appInit(void) {
    dmntchtInitialize();
    capsscInitialize();
    smInitialize();
    setsysInitialize();
    SetSysFirmwareVersion fw;
    setsysGetFirmwareVersion(&fw);
    hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
    setsysExit();
    setInitialize();
    pmdmntInitialize();
    nsInitialize();
    pminfoInitialize();
    fsInitialize();
    fsdevMountSdmc();
    timeInitialize();
    socketInitializeDefault();
}

void __appExit(void) {
    dmntchtExit();
    capsscExit();
    pminfoExit();
    pmdmntExit();
    setsysExit();
    fsdevUnmountAll();
    fsExit();
    smExit();
    timeExit();
    socketExit();
    nsExit();
    setExit();
    }
}

using namespace httplib;

#define SET_STATUS_FROM_RC(res, rc)     \
    if (R_FAILED(rc)) {                 \
        res.status = 500;               \
    } else {                            \
        res.status = 200;               \
    }

int main() {
    Server server;
    auto game = std::make_shared<GameReader>();

    server.Get("/refreshMetadata", [game](const Request &req, Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        SET_STATUS_FROM_RC(res, game->RefreshMetadata());
    });

    server.Get("/titleId", [](const Request &req, Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        u64 processId;
        u64 programId;
        std::string titleId;
        if (R_SUCCEEDED(pmdmntGetApplicationProcessId(&processId))){
            if (R_SUCCEEDED(pminfoGetProgramId(&programId, processId))) {
                titleId = convertNumToHexString(programId);
                res.set_content(titleId, "text/plain");
            } else {
                res.set_content("0", "text/plain");}
        } else {
            res.set_content("0", "text/plain");}
    });

    server.Get("/readHeap", [game](const Request &req, Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");

        u64 offset = 0;
        u64 size = 0;
        std::string offsetStr = "0";
        std::string sizeStr = "4";

        if (req.has_param("offset")) offsetStr = req.get_param_value("offset");

        if (req.has_param("size")) sizeStr = req.get_param_value("size");

        try {
            offset = stoull(offsetStr, 0, 16);
            size = stoull(sizeStr, 0, 16);
        } catch (const std::invalid_argument &ia) {
            res.set_content("Invalid parameters", "text/plain");
            res.status = 400;
        }

        if (res.status != 400) {
            u8 *buffer = new u8[size];
            auto rc = game->ReadHeap(offset, buffer, size);

            if (R_FAILED(rc)) {
                res.status = 500;
            } else {
                auto hexString = convertByteArrayToHex(buffer, size);
                res.set_content(hexString, "text/plain");
            }
            delete[] buffer;
        }
    });

    server.Get("/", [](const Request &req, Response &res) { res.set_content("Up and running!", "text/plain"); });

    while (appletMainLoop()) {
        server.listen("0.0.0.0", 8080);
        svcSleepThread(100000000L);
    }

    return 0;
}
