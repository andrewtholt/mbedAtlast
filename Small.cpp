/***********************************************************************
 * AUTHOR: andrewh <andrewh>
 *   FILE: ./Small.cpp
 *   DATE: Sat Jun 13 13:36:06 2020
 *  DESCR:
 ***********************************************************************/
// #include "Small.h"
#include "mbedSmall.h"
#include <iostream>

/***********************************************************************
 *  Method: Small::Small
 *  Params:
 * Effects:
 ***********************************************************************/
Small::Small() {
}


/***********************************************************************
 *  Method: Small::~Small
 *  Params:
 * Effects:
 ***********************************************************************/
Small::~Small() {
}


/***********************************************************************
 *  Method: Small::Get
 *  Params: std::string key
 * Returns: std::string
 * Effects:
 ***********************************************************************/
std::string Small::Get(std::string key) {

    std::string res="NON";

    int cnt = db.count(key);
    if (cnt > 0) {
        res = db[key]->value;
    } else {
        res="NON";
    }

    return res;
}

bool Small::GetBool(std::string key) {
    bool state = false;

    return state;
}

/***********************************************************************
 *  Method: Small::Set
 *  Params: std::string key, std::string value
 * Returns: void
 * Effects:
 ***********************************************************************/
void Small::Set(std::string key, std::string value) {
    int cnt = db.count(key);

    // TODO Smart pointer ?
    if( cnt > 0) {
        datum *p = db[key];
        p->value = value;

        for(auto sub : db[key]->subscriber ) {
            sendSet(taskId::INVALID,(taskId)sub, key, value );
        }
    } else {
        db[key] = new datum(value);
    }
}


/***********************************************************************
 *  Method: Small::sendSet
 *  Params: std::string key
 * Returns: void
 * Effects:
 ***********************************************************************/
void Small::sendSet(taskId source, taskId dest, std::string key, std::string value) {
//    std::cout << "========> Send set to " << to_string(id) << " Key "<< key << "=" << value << "\n";
}

void Small::Sub(std::string key, uint8_t id) {

    int cnt = db.count(key);
    if (cnt > 0) {
        (db[key]->subscriber).insert(id);

        sendSet(taskId::INVALID, (taskId)id, key, db[key]->value);
    }

}

void Small::dump() {
    std::cout << "======= Dump ======" << "\n";
    for(const auto &myPair : db ) {
        std::string key = myPair.first;
        std::string v   = db[key]->value;
        std::cout << "Key:" << key << " Value:" << v << "\n";
        std::cout << "Sub";
        for(auto sub : db[key]->subscriber ) {
            std::cout << "\t" << sub << "\n";
        }
    }
}



