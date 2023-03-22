///biblioteci retea.
#include "../Network/network.h"
#include "../Network/netinoutrotater.h"

///biblioteci cod.
#include "hlt.hpp"
#include "networking.hpp"

#include "taleNt_helper.h"

///am o retea cu 2 iesiri, trebuie sa spun pe care dintre cele 5 directii ar tb sa mearga.
int
getNetDirection52(hlt::GameMap &presentMap,
                  std::set<hlt::Move> &moves,
                  hlt::Location l,
                  Network &net,
                  Eigen::MatrixXd &netIn,
                  Eigen::MatrixXd &netOut)
{
  double outcomes[5] = {0};
  int redir[4] = {1, 4, 3, 2}; ///0 = N. 1 = W. 2 = S. 3 = E. (dc dupa o rotire voiam N <=> initial voiam W).

  outcomes[0] = 1;
  for (int _ = 0; _ < 4; _++) {
    netOut = net.interpretInput(netIn);

    ///normalizez outputul din retea.
    double rp = 1 / (netOut(0) + netOut(1));
    netOut(0) *= rp;
    netOut(1) *= rp;

    outcomes[0] = std::min(outcomes[0], netOut(0));
    outcomes[redir[_]] = netOut(1);

    rotater::rotateNetInOut(netIn, help::matBogus);
  }

  ///TODO si altele aici.
  for (unsigned char dir = 1; dir < 5; dir++) {
    hlt::Location lWant = presentMap.getLocation(l, dir);
    if (presentMap.getSite(lWant).owner == presentMap.getSite(l).owner &&
        help::distToBorder[lWant.y][lWant.x] > help::distToBorder[l.y][l.x]) {
      ///daca consider mutarea din granita in interior, ignor.
      outcomes[dir] = 0;
    }

    auto it = moves.lower_bound(hlt::Move{lWant, STILL});
    if (it != moves.end() && it->loc == lWant) { ///unde vrea sa se duca lWant?
      hlt::Location lWW = presentMap.getLocation(lWant, it->dir);
      if (lWW == l) {
        outcomes[dir] = 0;
        ///daca am un vecin care deja intentioneaza sa se mute in mine, nu consider mutarea asta inca de aici.
      }
    }
  }

  if (*std::max_element(outcomes+1, outcomes+5) > util::TALENT_MIN_CONFIDENCE) {
    return std::max_element(outcomes+1, outcomes+5) - outcomes;
  }

  if (outcomes[0] > util::TALENT_MIN_CONFIDENCE) {
    return 0;
  }

  return -1;
}

unsigned char
testBotSuggest(hlt::GameMap &presentMap,
               hlt::Location &initialLocation,
               unsigned short y,
               unsigned short x,
               unsigned char myID,
               bool napCond = false)
{
  hlt::Site present = presentMap.getSite({x, y});

  if (present.strength == 0) {
    return STILL;
  }

  bool ownAllNeighs = true;
  unsigned char dirs[4] = {1, 2, 3, 4};
  std::random_shuffle(dirs, dirs+4);

  if (napCond) {
    for (unsigned char dirNeigh: dirs) {
      hlt::Site siteDir = presentMap.getSite({x, y}, dirNeigh);
      if (siteDir.owner != myID && siteDir.owner != 0 && siteDir.strength < present.strength) {
        return dirNeigh;
      }
    }
  }

  for (unsigned char dirNeigh: dirs) {
    hlt::Site siteDir = presentMap.getSite({x, y}, dirNeigh);
    ownAllNeighs &= (siteDir.owner == myID);

    if (siteDir.owner != myID && siteDir.strength < present.strength) {
      return dirNeigh;
    }
  }

  if (ownAllNeighs) {
    unsigned char weakestNeigh = STILL;

    for (unsigned char dirNeigh: dirs) {
      if (presentMap.getDistance(initialLocation, {x, y}) <
          presentMap.getDistance(initialLocation, presentMap.getLocation({x, y}, dirNeigh))
          &&
          presentMap.getSite({x, y}, dirNeigh).strength <
          presentMap.getSite({x, y}, weakestNeigh).strength) {
        weakestNeigh = dirNeigh;
      }
    }

    return weakestNeigh;
  }

  return STILL;
}

void
testBot(hlt::GameMap &presentMap,
        std::set<hlt::Move> &moves,
        hlt::Location &initialLocation,
        unsigned char myID)
{
  ///daca orice patrat din vecinatatea mea e neocupat si am strength-ul mai mare decat el, il iau.
  ///daca am cucerit toti vecinii mei, dau strength-ul meu celui mai slab vecin.
  ///am grija sa nu fac ciclu de donatii, ex dau unui vecin tura asta si el imi da inapoi tura
  ///urmatoare

  for (unsigned short y = 0; y < presentMap.height; y++) {
    for (unsigned short x = 0; x < presentMap.width; x++) {
      hlt::Site present = presentMap.getSite({x, y});

      if (present.owner == myID) {
        unsigned char TBSuggest = testBotSuggest(presentMap, initialLocation, y, x, myID);
        moves.insert({{x, y}, TBSuggest});
      }
    }
  }
}

void
taleNt(hlt::GameMap &presentMap,
       hlt::Location &initialLocation,
       std::set<hlt::Move> &moves,
       std::vector<Network> &nets,
       Eigen::MatrixXd &netIn,
       Eigen::MatrixXd &netOut,
       unsigned char myID)
{
  for (unsigned short y = 0; y < presentMap.height; y++) {
    for (unsigned short x = 0; x < presentMap.width; x++) {
      hlt::Site present = presentMap.getSite({x, y});

      auto it = moves.lower_bound(hlt::Move{hlt::Location{x, y}, STILL});
      ///ma ocup de patratel doar daca nu l-am rezolvat inainte altundeva.
      if (present.owner == myID && (it == moves.end() || it->loc != hlt::Location{x, y})) {
        help::composeNetIn(presentMap, netIn, y, x, myID);

        int netIndex = util::INDEX_NET_FAR_ENEMY;
        if (help::distToEnemy[y][x] <= util::MAX_DIST_ENEMY) {
          netIndex = util::INDEX_NET_CLOSE_ENEMY;
        }

        int talentSuggest = getNetDirection52(presentMap, moves, {x, y}, nets[netIndex], netIn, netOut);

        hlt::Location Ls = {x, y};
        if (talentSuggest != -1) {
          Ls = presentMap.getLocation({x, y}, (unsigned char)talentSuggest);
        }

        ///TODO opreste punctele de pe granita sa se intoarca in interior.
        if (talentSuggest == -1
            ||
            (presentMap.getSite(Ls).owner != myID &&
             presentMap.getSite(Ls).strength >= present.strength)
            ||
            (presentMap.getSite(Ls).owner == myID &&
            (int)present.strength +
            (int)presentMap.getSite(Ls).strength > 255)
            ||
            (presentMap.getSite(Ls).owner == myID &&
            presentMap.getSite(Ls).strength == 0)
            ||
            present.strength == 0) {
          moves.insert({{x, y}, help::correctIntentByOtherLocation(
            presentMap,
            moves,
            (hlt::Location){x, y},
            (unsigned char)testBotSuggest(presentMap, initialLocation, y, x, myID)
          )});
        } else {
          moves.insert({{x, y}, help::correctIntentByOtherLocation(
            presentMap,
            moves,
            (hlt::Location){x, y},
            (unsigned char)talentSuggest
          )});
        }
      }
    }
  }
}

void
greedyMainland(hlt::GameMap &presentMap,
               std::set<hlt::Move> &moves,
               unsigned char myID)
{
  unsigned char dirs[4] = {1, 2, 3, 4};

  for (unsigned short y = 0; y < presentMap.height; y++) {
    for (unsigned short x = 0; x < presentMap.width; x++) {

      auto it = moves.lower_bound(hlt::Move{hlt::Location{x, y}, STILL});
      if (presentMap.getSite({x, y}).owner == myID &&
          help::distToBorder[y][x] >= util::TALENT_GREEDY_BORDER_WIDTH &&
          (it == moves.end() || it->loc != hlt::Location{x, y})) {

        ///controlez patratul {x, y} si nu sunt pe granita. fac greedy. cred ca aprovizionarea patratelor din
        ///margine e prost/inconsistent facuta de retea.
        if ((int)presentMap.getSite({x, y}).strength == 0 ||
            (int)presentMap.getSite({x, y}).strength <
            (int)presentMap.getSite({x, y}).production * util::TALENT_GREEDY_MOVE_TIMER) {
          moves.insert({{x, y}, STILL});
        } else {
          ///mut ce am in patratul {x, y} intr-un vecin.
          ///dintre cei mai apropriati de granita, il aleg pe cel mai slab.

          std::random_shuffle(dirs, dirs+4);

          std::vector<unsigned char> okDirs;
          for (int i = 0; i < 4; i++) {
            hlt::Location Ln = presentMap.getLocation({x, y}, dirs[i]);
            if (1/*(int)presentMap.getSite({x, y}).strength + (int)presentMap.getSite(Ln).strength <= 255*/) {
              if (help::distToEnemy[y][x] <= util::TALENT_GREEDY_SUPPLY_CONSCRIPTION_WIDTH) {
                ///TODO poate adaugi in iful de deasupra si min(help::distToEnemy) <= 2 sau cv de genul.
                if (help::distToEnemy[Ln.y][Ln.x] < help::distToEnemy[y][x]) {
                  okDirs.push_back(dirs[i]);
                }
              } else {
                if (help::distToBorder[Ln.y][Ln.x] < help::distToBorder[y][x]) {
                  okDirs.push_back(dirs[i]);
                }
              }
            }
          }

          if (okDirs.empty()) { ///nu ar tb sa ajung niciodata in iful asta.
            moves.insert({{x, y}, dirs[0]});
          } else {
            unsigned char bestDir = okDirs[0];
            for (int i = 1; i < (int)okDirs.size(); i++) {
              if (presentMap.getSite({x, y}, okDirs[i]).strength <
                  presentMap.getSite({x, y}, bestDir).strength) {
                bestDir = okDirs[i];
              }
            }
            moves.insert({{x, y}, bestDir});
          }
        }
      }
    }
  }
}

void
greedyNAP(hlt::GameMap &presentMap,
          hlt::Location &initialLocation,
          std::set<hlt::Move> &moves,
          unsigned char myID,
          unsigned short turnCount)
{
  for (unsigned short y = 0; y < presentMap.height; y++) {
    for (unsigned short x = 0; x < presentMap.width; x++) {
      auto it = moves.lower_bound(hlt::Move{hlt::Location{x, y}, STILL});
      if (presentMap.getSite({x, y}).owner == myID &&
          help::distToEnemy[y][x] <= util::TALENT_GREEDY_NAP_WIDTH &&
          (it == moves.end() || it->loc != hlt::Location{x, y})) {
        moves.insert({{x, y}, help::correctIntentByOtherLocation(
          presentMap,
          moves,
          (hlt::Location){x, y},
          (unsigned char)testBotSuggest(presentMap, initialLocation, y, x, myID)
        )});
      }
    }
  }
}

int
main()
{
  srand(time(NULL));

  unsigned char myID;
  hlt::GameMap presentMap;
  getInit(myID, presentMap);
  sendInit("taleNt IV G',H,B (SP rank)");

  std::set<hlt::Move> moves;
  help::init(presentMap);
  hlt::Location initialLocation = help::getInitialLocation(presentMap, myID);

  util::buildFastSig();

  std::vector<Network> nets; ///departe / aproape de dusman.
  for (int _ = 0; _ < 2; _++) {
    nets.push_back(Network(2, {util::NETIN_SIZE, 100, util::NETOUT_SIZE}, 0.125, 0.1, 0.25));
  }

  help::loadNetworkStateH(nets[0], taleNt_IV_FAR_8);
  help::loadNetworkStateH(nets[1], taleNt_IV_CLOSE_7);

  Eigen::MatrixXd netIn(util::NETIN_SIZE, 1), netOut(util::NETOUT_SIZE, 1);

  unsigned short turnCount = 0;
  while(true) {

    moves.clear();
    ///moves.insert({x, y}, 0/1/2/3/4 <=> STILL/NORTH/EAST/SOUTH/WEST).
    
    getFrame(presentMap);

    help::computeDistanceToBorder(presentMap, myID);
    help::computeDijkstra(presentMap, myID);
    help::computeDistanceToEnemies(presentMap, myID);

    greedyNAP(presentMap, initialLocation, moves, myID, turnCount);
    greedyMainland(presentMap, moves, myID);
    taleNt(presentMap, initialLocation, moves, nets, netIn, netOut, myID);

    sendFrame(moves);

    turnCount++;
  }

  return 0;
}
