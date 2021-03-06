#pragma once
#include <string>
#include <map>
#include <set>
#include "tasks.h"

class datum {
    public:
    std::string value;
    std::set<int>subscriber;

    datum(std::string v) {
        value=v;
        subscriber={};
    }
} ;

class Small {
    friend class mbedSmall;
    public:
        Small();
        ~Small();

        std::string Get(std::string key);
        bool GetBool(std::string key);

        void Set(std::string key, std::string value);
        void Sub(std::string key, uint8_t id);
        void Unsub(std::string key, uint8_t id);

        uint8_t getSubCount(std::string key);

        void dump();
        bool haveSubscribers();

        virtual void sendSet(taskId source, taskId dest,std::string key, std::string value);
    private:
        bool haveSubscribersFlag;
        std::map<std::string, datum *> db;

        std::set<std::string>True  = {"TRUE","ON","YES"};
        std::set<std::string>False = {"FALSE","OFF","NO"};
};

