#pragma once

#include "utilities.h"

namespace rotater
{
  ///roteste cu 90 de grade la dreapta echivalentul celor 3 matrici dintr-un input de retea.
  ///returneaza tot un input de retea valabil.

  const int submatSize = util::INTEREST_SIDE * util::INTEREST_SIDE * util::STATES;
  std::tuple<double, double, double> mat[util::INTEREST_SIDE][util::INTEREST_SIDE],
                                     matR[util::INTEREST_SIDE][util::INTEREST_SIDE];

  void
  rotateNetInOut(Eigen::MatrixXd &netIn, Eigen::MatrixXd &netOut)
  {
    assert(netIn.rows() == util::NETIN_SIZE);
    assert(netIn.cols() == 1);
    assert(netOut.rows() == util::NETOUTOLD_SIZE);
    assert(netOut.cols() == 1);

    ///STILL, NORTH, EAST, SOUTH, WEST
    double netOut4 = netOut(4);
    netOut(4) = netOut(3); ///S -> W
    netOut(3) = netOut(2); ///E -> S
    netOut(2) = netOut(1); ///N -> E
    netOut(1) = netOut4; ///W -> N

    int i, j, z;
    for (int submatInd = 0; submatInd < util::AFFILIATIONS; submatInd++) {
      ///rotesc pentru fiecare dintre ALLY, NEUTRAL si ENEMY.
      for (i = j = 0, z = submatSize * submatInd; z < submatSize * (submatInd+1); z += util::STATES) {
        mat[i][j] = std::make_tuple(netIn(z), netIn(z+1), netIn(z+2));
        j++;
        if (j >= util::INTEREST_SIDE) {
          j = 0;
          i++;
        }
      }

      for (i = 0; i < util::INTEREST_SIDE; i++) {
        for (j = 0; j < util::INTEREST_SIDE; j++) {
          matR[j][util::INTEREST_SIDE-1-i] = mat[i][j];
        }
      }

      for (i = j = 0, z = submatSize * submatInd; z < submatSize * (submatInd+1); z += util::STATES) {
        std::tie(netIn(z), netIn(z+1), netIn(z+2)) = matR[i][j];
        j++;
        if (j >= util::INTEREST_SIDE) {
          j = 0;
          i++;
        }
      }
    }
  }

//  void testRotation() {
//    FILE *fin = fopen("0.bin", "rb");
//
//    Eigen::MatrixXd netIn(util::NETIN_SIZE, 1);
//    Eigen::MatrixXd netOut(util::NETOUT_SIZE, 1);
//
//    decomp::parseVector(fin, netIn);
//    decomp::parseVector(fin, netOut);
//
//    Eigen::MatrixXd cpyIn = netIn, cpyOut = netOut;
//
//    rotater::rotateNetInOut(netIn, netOut);
//    rotater::rotateNetInOut(netIn, netOut);
//    rotater::rotateNetInOut(netIn, netOut);
//    rotater::rotateNetInOut(netIn, netOut);
//
//    assert(netIn == cpyIn && netOut == cpyOut);
//
//    fclose(fin);
//  }
}
