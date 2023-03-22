#include "utilities.h"
#include "netinoutrotater.h"
#include "decompresser.h"
#include "network.h"

///returneaza suma erorilor pe o multime de antrenament.
double
performEpoch(Network &net, std::vector<FILE *> &trainBins)
{
  ///a0 = input pentru retea, A = expected output.
  Eigen::MatrixXd a0 = Eigen::MatrixXd::Zero(net.layers[0].N, 1),
                   A = Eigen::MatrixXd::Zero(net.layers.back().M, 1),
                Aold = Eigen::MatrixXd::Zero(5, 1);
  ///in binuri este cu 5 iesiri reteaua, trebuie trecuta pe 2.
  ///reteaua mea cu doua outputuri nu este STILL, NORTH, ci NOT NORTH, NORTH!!!

  ///deci dc primesc spre antrenare un exemplu tip STILL, pentru fiecare dintre cele 4 rotiri tb sa
  ///il antrenez sa zica NOT NORTH.
  ///dc primesc spre antrenare un exemplu tip EAST, pentru 0 rotiri tb sa zica NOT NORTH, pt 1 rotire
  ///tb sa zica NOT NORTH, pt 2 rotiri tb sa zica NOT NORTH, iar pt 3 rotiri tb sa zica NORTH.

  double overallCost = 0;

  for (int _ = 0; _ < util::TRAIN_MOVE_COUNT_PER_EPOCH; _++) {
    ///iau 1 input cu expected = STILL.
    decomp::parseVector(trainBins[util::INDEX_STILL], a0);
    decomp::parseVector(trainBins[util::INDEX_STILL], Aold);

    for (int _1 = 0; _1 < 4; _1++) {
      util::attemptEnqueueStay(a0);
      rotater::rotateNetInOut(a0, Aold);
    }

    ///iau 1 input cu expected = MOVE si il rotesc pana cand output-ul lui indica 1 pe NORTH.
    ///pentru toate celelalte rotatii, reteaua trebuie sa indice NOT NORTH.
    ///TODO scoate NOT NORTH daca sta pe loc intotdeauna etc.
    decomp::parseVector(trainBins[util::INDEX_MOVE], a0);
    decomp::parseVector(trainBins[util::INDEX_MOVE], Aold);

    for (int _1 = 0; _1 < 4; _1++) {
      if (Aold(1) > 1 - 0.0001) {
        A(0) = 0; A(1) = 1; ///<=> NORTH.
        overallCost += net.learnInput(a0, A);
      } else {
        ///incerc si asa? poate se misca mai putin, dar mai sigur pe el.
        util::attemptEnqueueStay(a0); ///TODO scoate linia asta dc nu merge.
        ///probabil daca o directie e buna, nu inseamna ca celelalte sunt praf, doar ca sunt mai putin bune.
      }

      rotater::rotateNetInOut(a0, Aold);
    }

    ///in final, iau 1 sample NOT NORTH.
    A(0) = 1; A(1) = 0; ///<=> NOT NORTH.

    overallCost += net.learnInput(util::quStay.front(), A);
    util::quStay.pop();
  }

  return overallCost;
}

std::pair<double, double>
performTest(Network &net, std::vector<FILE *> &testBins)
{
  ///a0 = input pentru retea, A = expected output.
  ///A(0) <=> NOT NORTH, A(1) <=> ma duc in NORTH.
  Eigen::MatrixXd a0 = Eigen::MatrixXd::Zero(net.layers[0].N, 1),
                   A = Eigen::MatrixXd::Zero(net.layers.back().M, 1),
                Aold = Eigen::MatrixXd::Zero(5, 1);

  int correctGuessesSTAY = 0, correctGuessesMOVE = 0;

  for (int _ = 0; _ < util::TEST_MOVE_COUNT_PER_EPOCH; _++) {
    ///iau 1 input cu expected = STILL.
    decomp::parseVector(testBins[util::INDEX_STILL], a0);
    decomp::parseVector(testBins[util::INDEX_STILL], Aold);

    A(0) = 1; A(1) = 0;
    for (int _1 = 0; _1 < 4; _1++) {
      ///pe oricare dintre cele 4 rotatii trebuie sa indice NOT NORTH.
      if (net.getHighestActivationNode(net.interpretInput(a0)).first ==
          net.getHighestActivationNode(A).first) {
        correctGuessesSTAY++;
      }

      rotater::rotateNetInOut(a0, Aold);
    }

    ///iau 1 input cu expected = MOVE.
    decomp::parseVector(testBins[util::INDEX_MOVE], a0);
    decomp::parseVector(testBins[util::INDEX_MOVE], Aold);

    for (int _1 = 0; _1 < 4; _1++) {
      if (Aold(1) > 1 - 0.0001) {
        A(0) = 0; A(1) = 1; ///tb sa dea NORTH.
        if (net.getHighestActivationNode(net.interpretInput(a0)).first ==
            net.getHighestActivationNode(A).first) {
          correctGuessesMOVE++;
        }
      } else {
//        A(0) = 1; A(1) = 0; ///tb sa dea NOT NORTH.
//        if (net.getHighestActivationNode(net.interpretInput(a0)).first ==
//            net.getHighestActivationNode(A).first) {
//          correctGuessesSTAY++;
//        }
        ///nu cred ca tb sa dea neaparat NOT NORTH.
      }

      rotater::rotateNetInOut(a0, Aold);
    }
  }

  return std::make_pair((double)correctGuessesSTAY / (4 * util::TEST_MOVE_COUNT_PER_EPOCH),
                        (double)correctGuessesMOVE / util::TEST_MOVE_COUNT_PER_EPOCH);
}

int
main()
{
  util::buildFastSig();

  std::vector<FILE *> trainBins(2), testBins(2);

  trainBins[util::INDEX_STILL] = fopen("../ReplayParser/Processed/STILL/train_CLOSE.bin", "rb");
  assert(trainBins[util::INDEX_STILL]);
  trainBins[util::INDEX_MOVE] = fopen("../ReplayParser/Processed/MOVE/train_CLOSE.bin", "rb");
  assert(trainBins[util::INDEX_MOVE]);

  testBins[util::INDEX_STILL] = fopen("../ReplayParser/Processed/STILL/test_CLOSE.bin", "rb");
  assert(testBins[util::INDEX_STILL]);
  testBins[util::INDEX_MOVE] = fopen("../ReplayParser/Processed/MOVE/test_CLOSE.bin", "rb");
  assert(testBins[util::INDEX_MOVE]);

  std::cerr << "Have opened 2 training bins and 2 testing bins!\n";

  Network net(2, {util::NETIN_SIZE, 100, util::NETOUT_SIZE}, 0.125, 0.1, 0.25);
  //0.25, 0.2, 0.5 -> 0.125 0.1 0.25
  //net.loadNetworkState("../Logs/taleNt_III/51_best_perc.txt");

  int epochCount = 0;
  double lastPrecision = 0, testPrecisionSTAY = 0, testPrecisionMOVE = 0, testPrecision = 0;
  while(1) {
    double epochCost = performEpoch(net, trainBins);
    std::cerr << "Performed epoch: " << epochCount+1 << ", with cost: " << epochCost << '\n';

    net.writeNetworkState(epochCount+1, util::NETOWRK_STATE_OUTPUT_PATH);
    epochCount++;

    std::tie(testPrecisionSTAY, testPrecisionMOVE) = performTest(net, testBins);
    testPrecision = (4 * testPrecisionSTAY + testPrecisionMOVE) / 5;

    std::cerr << std::fixed << std::setprecision(3);
    std::cerr << "Performed test with precision: " << testPrecisionSTAY << " (STAY), ";
    std::cerr << testPrecisionMOVE << " (MOVE), ";
    std::cerr << testPrecision << " (AVG)\n";

    if (epochCount >= util::LEAST_NUMBER_OF_EPOCHS &&
        testPrecision > lastPrecision &&
        testPrecision < lastPrecision + util::EPOCH_STOP_PRECISION) {
      break;
    }

    lastPrecision = testPrecision;
  }

  std::cerr << "Exited after " << epochCount << " epochs.\n";

  for (int _ = 0; _ < 2; _++) {
    fclose(trainBins[_]);
    fclose(testBins[_]);
  }

  return 0;
}
