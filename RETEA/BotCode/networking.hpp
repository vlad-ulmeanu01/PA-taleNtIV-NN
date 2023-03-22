#ifndef AI_NETWORKING_H
#define AI_NETWORKING_H

#include <iostream>
#include <time.h>
#include <set>
#include <cfloat>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <bitset>

#ifdef _WIN32
#include <sys/types.h>
#include <Winsock2.h>
#include <Ws2tcpip.h>
#define WINSOCKVERSION MAKEWORD(2,2)
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#endif

#include "hlt.hpp"

namespace detail{
    static std::vector< std::vector<unsigned char> > productions;
    static int width, height;

    static std::string serializeMoveSet(const std::set<hlt::Move> &moves) {
        //cumva are nevoie de comparatorul asta dubios..
        std::vector<hlt::Move> movesArr;
        for (auto x: moves) movesArr.push_back(x);
        std::sort(movesArr.begin(), movesArr.end(), [](const hlt::Move &m1, const hlt::Move &m2) {
            unsigned int l1Prod = ((m1.loc.x + m1.loc.y)*((unsigned int)m1.loc.x + m1.loc.y + 1) / 2) + m1.loc.y, l2Prod = ((m2.loc.x + m2.loc.y)*((unsigned int)m2.loc.x + m2.loc.y + 1) / 2) + m2.loc.y;
            return ((l1Prod + m1.dir)*(l1Prod + m1.dir + 1) / 2) + m1.dir < ((l2Prod + m2.dir)*(l2Prod + m2.dir + 1) / 2) + m2.dir;
        });

        std::ostringstream oss;
        for(auto a = movesArr.begin(); a != movesArr.end(); ++a) oss << a->loc.x << " " << a->loc.y << " " << (int)a->dir << " ";
        return oss.str();
    }

    static void deserializeMapSize(const std::string & inputString) {
        std::stringstream iss(inputString);
        iss >> width >> height;
    }

    static void deserializeProductions(const std::string & inputString) {
        std::stringstream iss(inputString);
        productions.resize(height);
        short temp;
        for(auto a = productions.begin(); a != productions.end(); a++) {
            a->resize(width);
            for(auto b = a->begin(); b != a->end(); b++) {
                iss >> temp;
                *b = temp;
            }
        }
    }

    static hlt::GameMap deserializeMap(const std::string & inputString) {
        std::stringstream iss(inputString);

        hlt::GameMap map(width, height);

        //Set productions
        for(int a = 0; a < map.height; a++) {
            for(int b = 0; b < map.width; b++) {
                map.contents[a][b].production = productions[a][b];
            }
        }

        //Run-length encode of owners
        unsigned short y = 0, x = 0;
        unsigned short counter = 0, owner = 0;
        while(y != map.height) {
            for(iss >> counter >> owner; counter; counter--) {
                map.contents[y][x].owner = owner;
                x++;
                if(x == map.width) {
                    x = 0;
                    y++;
                }
            }
        }

        for (int a = 0; a < (int)map.contents.size(); a++) {
            for (int b = 0; b < (int)map.contents[a].size(); b++) {
                short strengthShort;
                iss >> strengthShort;
                map.contents[a][b].strength = strengthShort;
            }
        }

        return map;
    }
    static void sendString(const std::string & sendString) {
        if(sendString.length() < 1) std::cout << ' ' << std::endl; //Automatically flushes.
        else std::cout << sendString << std::endl; //Automatically flushes.
    }

    static std::string getString() {
        std::string newString;
        std::getline(std::cin, newString);
        return newString;
    }
}

static void getInit(unsigned char& playerTag, hlt::GameMap& m) {
    playerTag = (unsigned char)std::stoi(detail::getString());
    detail::deserializeMapSize(detail::getString());
    detail::deserializeProductions(detail::getString());
    m = detail::deserializeMap(detail::getString());

}

static void sendInit(std::string name) {
    detail::sendString(name);
}

static void getFrame(hlt::GameMap& m) {
    m = detail::deserializeMap(detail::getString());
}
static void sendFrame(const std::set<hlt::Move> &moves) {
    detail::sendString(detail::serializeMoveSet(moves));
}

#endif
