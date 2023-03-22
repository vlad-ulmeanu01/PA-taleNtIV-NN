#ifndef HLT_H
#define HLT_H

#include <list>
#include <vector>
#include <random>

#define STILL 0
#define NORTH 1
#define EAST 2
#define SOUTH 3
#define WEST 4

const int DIRECTIONS[] = {STILL, NORTH, EAST, SOUTH, WEST};
const int CARDINALS[] = {NORTH, EAST, SOUTH, WEST};

namespace hlt {
    struct Location {
        unsigned short x, y;
    };

    static bool operator == (const Location &l1, const Location &l2) {
        return l1.x == l2.x && l1.y == l2.y;
    }

    static bool operator != (const Location &l1, const Location &l2) {
        return !(l1 == l2);
    }

    // static bool operator < (const Location& l1, const Location& l2) {
    //     return ((l1.x + l1.y)*((unsigned int)l1.x + l1.y + 1) / 2) + l1.y <
    //            ((l2.x + l2.y)*((unsigned int)l2.x + l2.y + 1) / 2) + l2.y;
    // }

    static bool operator < (const Location &l1, const Location &l2) {
        if (l1.x != l2.x) return l1.x < l2.x;
        return l1.y < l2.y;
    }

    struct Site {
        unsigned char owner;
        unsigned char strength;
        unsigned char production;
    };

    class GameMap {
    public:
        std::vector< std::vector<Site> > contents;
        unsigned short width, height; //Number of rows & columns, NOT maximum index.

        GameMap() {
            width = 0;
            height = 0;
            contents = std::vector< std::vector<Site> >(height, std::vector<Site>(width, { 0, 0, 0 }));
        }

        GameMap(const GameMap &otherMap) {
            width = otherMap.width;
            height = otherMap.height;
            contents = otherMap.contents;
        }

        GameMap(int w, int h) {
            width = w;
            height = h;
            contents = std::vector< std::vector<Site> >(height, std::vector<Site>(width, { 0, 0, 0 }));
        }

        bool inBounds(Location l) {
            return l.x < width && l.y < height;
        }

        short getDistance(Location l1, Location l2) { ///TODO dc crapa pune float aici.
            short dx = abs(l1.x - l2.x), dy = abs(l1.y - l2.y);
            if(dx > width / 2) dx = width - dx;
            if(dy > height / 2) dy = height - dy;
            return dx + dy;
        }

        float getAngle(Location l1, Location l2) {
            short dx = l2.x - l1.x, dy = l2.y - l1.y;
            if(dx > width - dx) dx -= width;
            else if(-dx > width + dx) dx += width;
            if(dy > height - dy) dy -= height;
            else if(-dy > height + dy) dy += height;
            return atan2(dy, dx);
        }

        Location getLocation(Location l, unsigned char direction) {
            if(direction != STILL) {
                if(direction == NORTH) {
                    if(l.y == 0) l.y = height - 1;
                    else l.y--;
                }
                else if(direction == EAST) {
                    if(l.x == width - 1) l.x = 0;
                    else l.x++;
                }
                else if(direction == SOUTH) {
                    if(l.y == height - 1) l.y = 0;
                    else l.y++;
                }
                else if(direction == WEST) {
                    if(l.x == 0) l.x = width - 1;
                    else l.x--;
                }
            }
            return l;
        }

        Site& getSite(Location l, unsigned char direction = STILL) {
            l = getLocation(l, direction);
            return contents[l.y][l.x];
        }
    };

    struct Move {
        Location loc;
        unsigned char dir;
    };

    static bool operator < (const Move& m1, const Move& m2) {
        if (m1.loc != m2.loc) return m1.loc < m2.loc;
        return m1.dir < m2.dir;
    }
}

#endif
