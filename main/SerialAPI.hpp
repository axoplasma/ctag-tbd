#pragma once

#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

using namespace std;

namespace CTAG::SAPI{
    class SerialAPI {
    public:
        static void StartSerialAPI();
    private:
        static void processAPICommand(const string &cmd);
        static void sendString(const string &s);
        static void initUART();
        static void serialTask(void*);
        static TaskHandle_t hSerialTask;
        static const char stx;
        static const char etx;
    };
}

