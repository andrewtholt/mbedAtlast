#pragma once
#include <string>
#include <cstdint>

#include "Small.h"

class mbedSmall : public Small {
    public:
        void sendSet(uint8_t id,std::string key, std::string value);
};
