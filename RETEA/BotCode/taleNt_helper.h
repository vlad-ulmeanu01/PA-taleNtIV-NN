#pragma once

#include "../Network/network.h"

#include "hlt.hpp"
#include "networking.hpp"
#include "taleNt_net_txt.h"

namespace help {
  ///pentru Dijkstra.
  std::priority_queue<std::pair<int, hlt::Location>,
                      std::vector<std::pair<int, hlt::Location>>,
                      std::greater<std::pair<int,hlt::Location>>> pq;
  std::vector<std::vector<int>> costs;
  unsigned char maxProduction;

  ///pentru distanta fata de dusmani.
  std::vector<std::vector<int>> distToEnemy;

  ///pentru distanta fata de granita mea. un patratel aliat este pe granita daca nu are toti cei 4
  ///vecini aliati.
  std::vector<std::vector<int>> distToBorder;

  ///folosit de supply. majoritatea patratelelor au util::TALENT_GREEDY_MOVE_TIMER, dar pentru unele
  ///vreau sa pun util::TALENT_GREEDY_MOVE_TIMER + 2 sau 3.
  // std::vector<std::vector<int>> matGreedyTimer;

  Eigen::MatrixXd matBogus = Eigen::MatrixXd::Zero(5, 1); ///folosita pentru rotatii in getNetDirection52.

  ///se foloseste pentru a scadea exponential interesul pentru patratele aflate la o distanta mai mare de
  ///patratelul analizat acum.
  double deflationFunction(int x) {
    assert(x >= 0);
    assert(x < util::INTEREST_SIDE);
    switch(x) {
      case 0: return 1;
      case 1: return 1.0;
      case 2: return 0.7937005259840998;
      case 3: return 0.6299605249474366;
      case 4: return 0.5;
      case 5: return 0.3968502629920499;
      case 6: return 0.3149802624737183;
      case 7: return 0.25;
      case 8: return 0.19842513149602492;
      case 9: return 0.15749013123685915;
      default: return 0.125;
    }
  }

  ///folosit pentru a transforma distanta minima catre un patratel intr-o valoare de activare a unui neuron.
  ///vreau f(0) = 1, f(255*10) ~= 0, f(255*2) = 0.5; pp ca majoritatea costurilor sunt relativ mici fata
  ///de capatul maxim posibil.
  double deflationDistance(int x) {
    ///x \in [0, 2550].
    ///daca fac aproximare liniara, ramane ft putin sensitiv intervalul care chiar ma intereseaza pe mine
    ///in general.
    return pow(2, ((double)(-x) / 512));
  }

  void
  init(hlt::GameMap &presentMap)
  {
    costs.resize(presentMap.height);
    
    for (unsigned short y = 0; y < presentMap.height; y++) {
      costs[y].resize(presentMap.width);
    }

    distToEnemy.resize(presentMap.height);
    for (unsigned short y = 0; y < presentMap.height; y++) {
      distToEnemy[y].resize(presentMap.width);
    }

    distToBorder.resize(presentMap.height);
    for (unsigned short y = 0; y < presentMap.height; y++) {
      distToBorder[y].resize(presentMap.width);
    }

    maxProduction = 0;
    for (unsigned short y = 0; y < presentMap.height; y++) {
      for (unsigned short x = 0; x < presentMap.width; x++) {
        maxProduction = std::max(maxProduction, presentMap.getSite({x, y}).production);
      }
    }

    // matGreedyTimer.resize(presentMap.height);
    // for (unsigned short y = 0; y < presentMap.height; y++) {
    //   matGreedyTimer[y].resize(presentMap.width);
    //   for (unsigned short x = 0; x < presentMap.width; x++) {
    //     matGreedyTimer[y][x] = util::TALENT_GREEDY_MOVE_TIMER;
    //     // if (presentMap.getDistance({x, y}, initialLocation) >= util::TALENT_GREEDY_WAVE_MIN_DISTANCE &&
    //     //     (x+y) % 8 == 3) {
    //     //   matGreedyTimer[y][x] = util::TALENT_GREEDY_MOVE_TIMER + util::TALENT_GREEDY_MOVE_BONUS;
    //     // }
    //   }
    // }
  }

  ///iau locatia initiala.
  hlt::Location
  getInitialLocation(hlt::GameMap &presentMap,
                     unsigned char myID)
  {
    hlt::Location initialLocation = {presentMap.width, presentMap.height};

    for(unsigned short y = 0; y < presentMap.height; y++) {
      for(unsigned short x = 0; x < presentMap.width; x++) {
        if (presentMap.getSite({x, y}).owner == myID) {
          initialLocation = {x, y};
        }
      }
    }

    return initialLocation;
  }

  ///cauta intentia de miscare a lui l. daca nu exista, returneaza 0 <=> STILL.
  ///daca exista in set, returneaza {0,1,2,3,4}, intentia de directie pe care o are.
  unsigned char
  findMoveIntention(std::set<hlt::Move> &moves,
                    hlt::Location l)
  {
    hlt::Move m = {l, 0};

    auto it = moves.lower_bound(m);

    if (it == moves.end() || it->loc != l) {
      return 0;
    }

    return it->dir;
  }

  ///pentru patratelul controlat de mine "l", vreau mutarea suggest. l X suggest => lWant.
  ///exista o problema. poate lWant e controlat de mine si vrea sa se duca in l. astfel, doar
  ///interschimb niste patratele (l, lWant) si pierd o tura degeaba. functia returneaza o
  ///corectie pentru suggest: daca lWant x sugestia lui == l, atunci suggest devine STILL.
  ///altfel, suggest ramane la fel.
  unsigned char
  correctIntentByOtherLocation (hlt::GameMap &presentMap,
                                std::set<hlt::Move> &moves,
                                hlt::Location l,
                                unsigned char suggest)
  {
    hlt::Location lWant = presentMap.getLocation(l, suggest);
    hlt::Location lWW = presentMap.getLocation(lWant, findMoveIntention(moves, lWant));

    if (lWW == l) {
      ///am 2 patrate care incearca sa se ajute reciproc. eu stau pe loc si
      ///il las pe celalalt sa ma ajute.
      return STILL;
    }

    return suggest;
  }

  void
  computeDijkstra(hlt::GameMap &presentMap,
                  unsigned char myID)
  {
    while (!pq.empty()) {
      pq.pop();
    }

    for (unsigned short y = 0; y < presentMap.height; y++) {
      for (unsigned short x = 0; x < presentMap.width; x++) {
        if (presentMap.getSite({x, y}).owner != myID) {
          costs[y][x] = (1<<30);
        } else {
          costs[y][x] = 0;
          pq.push(std::make_pair(0, hlt::Location{x, y}));
        }
      }
    }

    int cost;
    hlt::Location L;
    while (!pq.empty()) {
      std::tie(cost, L) = pq.top();
      pq.pop();

      if (costs[L.y][L.x] < cost) {
        continue;
      }

      for (unsigned char dir = 1; dir < 5; dir++) {
        hlt::Location Ln = presentMap.getLocation(L, dir);
        if (costs[Ln.y][Ln.x] > costs[L.y][L.x] + presentMap.getSite(Ln).strength) {
          costs[Ln.y][Ln.x] = costs[L.y][L.x] + presentMap.getSite(Ln).strength;
          pq.push(std::make_pair(costs[Ln.y][Ln.x], Ln));
        }
      }
    }
  }

  void
  computeDistanceToEnemies(hlt::GameMap &presentMap,
                           unsigned char myID)
  {
    std::queue<hlt::Location> qu;
    bool inq[presentMap.height][presentMap.width] = {false};

    for (unsigned short y = 0; y < presentMap.height; y++) {
      for (unsigned short x = 0; x < presentMap.width; x++) {
        unsigned char owner = presentMap.getSite({x, y}).owner;
        if (owner != myID && owner != 0) {
          distToEnemy[y][x] = 0;
          qu.push(hlt::Location{x, y});
          inq[y][x] = true;
        } else {
          distToEnemy[y][x] = (1<<30);
          inq[y][x] = false;
        }
      }
    }

    while (!qu.empty()) {
      hlt::Location L = qu.front();
      qu.pop();
      inq[L.y][L.x] = false;

      for (unsigned char dir = 1; dir < 5; dir++) {
        hlt::Location Ln = presentMap.getLocation(L, dir);
        if (distToEnemy[Ln.y][Ln.x] > distToEnemy[L.y][L.x] + 1) {
          distToEnemy[Ln.y][Ln.x] = distToEnemy[L.y][L.x] + 1;
          if (!inq[Ln.y][Ln.x]) {
            qu.push(Ln);
            inq[Ln.y][Ln.x] = true;
          }
        }
      }
    }
  }

  void
  computeDistanceToBorder(hlt::GameMap &presentMap,
                          unsigned char myID)
  {
    std::queue<hlt::Location> qu;
    bool inq[presentMap.height][presentMap.width] = {false};

    for (unsigned short y = 0; y < presentMap.height; y++) {
      for (unsigned short x = 0; x < presentMap.width; x++) {
        unsigned char owner = presentMap.getSite({x, y}).owner;
        if (owner != myID) {
          distToBorder[y][x] = (1<<30);
          inq[y][x] = false;
        } else {
          int alliedNeighsCnt = 0;
          for (unsigned char dir = 1; dir < 5; dir++) {
            if (presentMap.getSite({x, y}, dir).owner == myID) {
              alliedNeighsCnt++;
            }
          }

          if (alliedNeighsCnt < 4) {
            distToBorder[y][x] = 0;
            qu.push(hlt::Location{x, y});
            inq[y][x] = true;
          } else {
            distToBorder[y][x] = (1<<30);
            inq[y][x] = false;
          }
        }
      }
    }

    while (!qu.empty()) {
      hlt::Location L = qu.front();
      qu.pop();
      inq[L.y][L.x] = false;

      for (unsigned char dir = 1; dir < 5; dir++) {
        hlt::Location Ln = presentMap.getLocation(L, dir);
        if (distToBorder[Ln.y][Ln.x] > distToBorder[L.y][L.x] + 1) {
          distToBorder[Ln.y][Ln.x] = distToBorder[L.y][L.x] + 1;
          if (!inq[Ln.y][Ln.x]) {
            qu.push(Ln);
            inq[Ln.y][Ln.x] = true;
          }
        }
      }
    }
  }

  ///compune inputul pentru retea considerand punctul de pe linia i, coloana j.
  void
  composeNetIn(hlt::GameMap &presentMap,
               Eigen::MatrixXd &netIn,
               unsigned short i,
               unsigned short j,
               unsigned char myID)
  {
    netIn = Eigen::MatrixXd::Zero(util::NETIN_SIZE, 1);

    for (int dy = -util::INTEREST_LEN; dy <= util::INTEREST_LEN; dy++) {
      for (int dx = -util::INTEREST_LEN; dx <= util::INTEREST_LEN; dx++) {
        int y = (i + dy + presentMap.height) % presentMap.height;
        int x = (j + dx + presentMap.width) % presentMap.width;
        int baseIndex = 0;

        hlt::Site presentSite = presentMap.getSite(hlt::Location{(unsigned short)x, (unsigned short)y});

        if (presentSite.owner == myID) { ///ALLY
          baseIndex = 11*11*3 * 0 + 3 * ((dy+5)*11 + dx+5);

          ///strength.
          netIn(baseIndex + 0) = deflationFunction(
            abs(dy) + abs(dx)
          ) * ((double)presentSite.strength / 255);

          ///production.
          netIn(baseIndex + 1) = deflationFunction(
            abs(dy) + abs(dx)
          ) * ((double)presentSite.production / maxProduction);

          ///cost cucerire patratel.
          netIn(baseIndex + 2) = 0; ///<=> nu imi pasa de el

        } else if (presentSite.owner == 0) { ///NEUTRAL
          baseIndex = 11*11*3 * 1 + 3 * ((dy+5)*11 + dx+5);

          ///strength.
          netIn(baseIndex + 0) = deflationFunction(
            abs(dy) + abs(dx)
          ) * ((double)presentSite.strength / 255);
          ///(NU) cu cat are strength mai mic, cu atat mai bine.
          ///cred ca faceam o prostie cu 1 - ...

          ///production.
          netIn(baseIndex + 1) = deflationFunction(
            abs(dy) + abs(dx)
          ) * ((double)presentSite.production / maxProduction);

          ///cost cucerire patratel.
          netIn(baseIndex + 2) = deflationDistance(
            costs[y][x]
          );

        } else { ///ENEMY
          baseIndex = 11*11*3 * 2 + 3 * ((dy+5)*11 + dx+5);

          ///strength.
          netIn(baseIndex + 0) = deflationFunction(
            abs(dy) + abs(dx)
          ) * ((double)presentSite.strength / 255);

          ///production.
          netIn(baseIndex + 1) = deflationFunction(
            abs(dy) + abs(dx)
          ) * ((double)presentSite.production / maxProduction);

          ///cost cucerire patratel.
          netIn(baseIndex + 2) = deflationDistance(
            costs[y][x] * 2
          );
          ///l-am pus sa ia <=> 2x costul echivalent pentru un patratel neutru.
        }
      }
    }
  }

  void
  loadNetworkStateH(Network &net, NetTxt &netTxt)
  {
    int layerCount_; netTxt.read(layerCount_);
    assert(net.layerCount == layerCount_);

    int i, j, z, N_, M_;
    for (z = 0; z < layerCount_; z++) {
      netTxt.read(N_);
      netTxt.read(M_);

      assert(net.layers[z].N == N_);
      assert(net.layers[z].M == M_);

      for (i = 0; i < N_; i++) {
        for (j = 0; j < M_; j++) {
          netTxt.read(net.layers[z].W(i, j));
        }
      }

      for (j = 0; j < M_; j++) {
        netTxt.read(net.layers[z].b(j));
      }
    }
  }
}
