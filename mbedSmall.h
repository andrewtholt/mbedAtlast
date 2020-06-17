#pragma once
#include <string>
#include <cstdint>

#include "Small.h"

class mbedSmall : public Small {
    public:
        void sendSet(taskId source, taskId dest,std::string key, std::string value);
};
