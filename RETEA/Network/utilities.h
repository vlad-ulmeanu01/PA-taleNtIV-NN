#pragma once

#pragma GCC optimize("Ofast,no-stack-protector,unroll-loops,fast-math,O3")
//#pragma GCC target("sse,sse2,sse3,ssse3,sse4,popcnt,abm,mmx,avx,tune=native")

#include <algorithm>
#include <iostream>
#include <fstream>
#include <utility>
#include <iomanip>
#include <vector>
#include <chrono>
#include <string>
#include <random>
#include <queue>
#include <set>
#include <cassert>
#include <cstdio>
#include <ctime>
#include <cmath>

#include "../Eigen/Dense"


namespace util
{
  const int INTEREST_LEN = 5;
  const int INTEREST_SIDE = INTEREST_LEN * 2 + 1; ///latura ariei de interes.
  const int AFFILIATIONS = 3; ///ALLY, NEUTRAL, ENEMY.
  const int STATES = 3; ///strength, production, cost cucerire patratel.
  const int NETIN_SIZE = INTEREST_SIDE * INTEREST_SIDE * STATES * AFFILIATIONS; ///11*11*3*3 = 1089.
  const int NETOUT_SIZE = 2;
  const int NETOUTOLD_SIZE = 5;

  const int TRAIN_MOVE_COUNT_PER_EPOCH = 150000; ///practic dai train la t * (1+4) pe epoca.
  const int TEST_MOVE_COUNT_PER_EPOCH = 50000; ///practic tot 5X ai.
  const int RUN_TEST_ONCE_EVERY_X_EPOCHS = 3;
  const int MAX_STAY_QUEUE_SIZE = 200000;

  const double TALENT_MIN_CONFIDENCE = 0.8;
  const int TALENT_GREEDY_BORDER_WIDTH = 1;
  const int TALENT_GREEDY_MOVE_TIMER = 5; ///<=> 3 timpi stau, al 4-lea timp mut.
  const int TALENT_GREEDY_MOVE_BONUS = 2; ///pt anumite patratele prefer sa stau mai mult (valuri).
  const int TALENT_GREEDY_WAVE_MIN_DISTANCE = 5; ///cate patratele distanta de baza vr ai incep sa fac valuri.
  const int TALENT_GREEDY_NAP_WIDTH = 2; ///dc distanta fata de un inamic este <= 2, fac Non Agression Pact.
  const int TALENT_GREEDY_SUPPLY_CONSCRIPTION_WIDTH = 5; ///vr sa trimit mai degraba catre granita cu inamic langa ea.

  const int LEAST_NUMBER_OF_EPOCHS = 50;
  const double EPOCH_STOP_PRECISION = 0.0025; ///daca castigul dintre 2 runde este mai mic decat ??, ma opresc.
  const std::string NETOWRK_STATE_OUTPUT_PATH = "../NetworkStates";

  ///in netOut care are lungimea 2, pe care pozitie gasesc NOT NORTH/NORTH?
  const int INDEX_STILL = 0;
  const int INDEX_MOVE = 1;
  ///in vectorul nets care are 2 retele neurale, pe care pozitie gasesc departe de dusman/aproape de dusman?
  const int INDEX_NET_FAR_ENEMY = 0;
  const int INDEX_NET_CLOSE_ENEMY = 1;

  ///cat de aproape trebuie sa fie dusmanul ca sa folosesc reteaua <aproape de dusman>.
  const int MAX_DIST_ENEMY = 4;

  std::vector<double> expTable, sigTable, sigDTable;
  int fastMargin;

  std::queue<Eigen::MatrixXd> quStay; ///pt ca rata NOT NORTH/NORTH este ?:1, tb sa tin in coada tot ce e NOT NORTH
                                      ///in plus.


  void attemptEnqueueStay(Eigen::MatrixXd &netIn) {
    ///TODO daca nu merge, da pop dc sz(qu) == MAX_SIZE ca sa poti da push de fiecare data.
    if ((int)quStay.size() < MAX_STAY_QUEUE_SIZE) {
      quStay.push(netIn);
    }
  }


  void buildFastSig() {
    fastMargin = 160;
    expTable.resize(2 * fastMargin * 1000 + 1);
    sigTable.resize(2 * fastMargin * 1000 + 1);
    sigDTable.resize(2 * fastMargin * 1000 + 1);

    int ind = 0;
    for (double x = -fastMargin; !(x > fastMargin); x += 0.001, ind++) {
      expTable[ind] = exp(x / 8);
      sigTable[ind] = expTable[ind] / (1 + expTable[ind]);
      sigDTable[ind] = sigTable[ind] / (1 + expTable[ind]) / 8;
    }
  }

  inline int getIndex(double x) {
    if (x < -fastMargin) x = -fastMargin;
    if (x > fastMargin) x = fastMargin;

    int ind = round((fastMargin + x) * 1000) + 1;
    if (ind < 0) ind = 0;
    if (ind >= (int)expTable.size()) ind = (int)expTable.size() - 1;

    return ind;
  }

  inline double sigmoid(double x) {
    return sigTable[getIndex(x)];
  }

  inline double sigmoidDeriv(double x) {
    return sigDTable[getIndex(x)];
  }

//  double sigmoid(double x) {
//    double ex = exp(x);
//    return ex / (1 + ex);
//  }
//
//  double sigmoidDeriv(double x) {
//    double ex = exp(x);
//    return ex / ((1 + ex) * (1 + ex));
//  }

  Eigen::MatrixXd sigmoid(Eigen::MatrixXd X) {
    for (int i = 0; i < X.rows(); i++) {
      for (int j = 0; j < X.cols(); j++) {
        X(i, j) = sigmoid(X(i, j));
      }
    }
    return X;
  }
}
