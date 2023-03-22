#pragma once

#include "utilities.h"
#include "layer.h"

///o retea este formata din mai multe nivele.
///un nivel de forma: a0 -> a1.
///trei nivele de forma: a0 -> a1 -> a2 -> a3.

struct Network {
  int layerCount;
  std::vector<Layer> layers;
  double ctW, ctb, cta;

  Network() {
    layerCount = 0;
    ctW = ctb = cta = 0;
  }

  Network(int layerCount_, std::vector<int> layerSizes, double ctW_, double ctb_, double cta_) {
    assert(layerCount_ >= 1);
    assert(layerCount_ + 1 == (int)layerSizes.size());

    layerCount = layerCount_;
    for (int i = 0; i < layerCount; i++) {
      layers.push_back(Layer(layerSizes[i], layerSizes[i+1], 1.0));
    }

    ctW = ctW_;
    ctb = ctb_;
    cta = cta_;
  }

  Eigen::MatrixXd
  interpretInput(Eigen::MatrixXd a0)
  {
    assert(a0.rows() == layers[0].N);
    assert(a0.cols() == 1);

    Eigen::MatrixXd a1;
    for (int i = 0; i < layerCount; i++) {
      a1 = layers[i].interpretInput(a0);
      a0 = a1;
    }

    return a0;
  }

  ///returneaza costul pentru a0 pe retea inainte de a face propagare inversa.
  ///A este vectorul pe care il astept sa iasa din ultimul nivel al retelei.
  double
  learnInput(Eigen::MatrixXd &a0, Eigen::MatrixXd A)
  {
    ///a contine toti vectorii de activare din retea.
    std::vector<Eigen::MatrixXd> a;
    a.push_back(a0);
    for (int i = 0; i < layerCount; i++) {
      a.push_back(layers[i].interpretInput(a.back()));
    }

    double cost = layers.back().computeErrors(a.back(), A).sum(); ///cost va fi returnat.

    ///acum trebuie sa fac propagare inversa.
    Eigen::MatrixXd NablaW, Nablab, Nablaa;
    for (int i = (int)layers.size() - 1; i >= 0; i--) {
      std::tie(NablaW, Nablab, Nablaa) = layers[i].computeGradientsWba(a[i], A);
      layers[i].W -= ctW * NablaW;
      layers[i].b -= ctb * Nablab;

      ///trebuie sa il schimb pe A ai pare ca este expected out pentru nivelul anterior.
      A = a[i] - cta * Nablaa;
    }

    return cost;
  }

  ///.first = indicele ([0 ..] care are valoarea maxima din vector), .second = valoarea maxima din vector.
  std::pair<int, double>
  getHighestActivationNode(Eigen::MatrixXd a1)
  {
    int ans = 0;
    assert(a1.cols() == 1);
    for (int i = 0; i < a1.rows(); i++) {
      if (a1(i) > a1(ans)) {
        ans = i;
      }
    }
    return std::make_pair(ans, a1(ans));
  }

  ///afiseaza toate greutatile/biasurile retelei intr-un fisier.
  void
  writeNetworkState(int epochCount, std::string outputPath)
  {
    assert(outputPath.back() != '/');

    char s[outputPath.size() + 100] = {0};
    sprintf(s, "%s/%d.txt", outputPath.c_str(), epochCount);

    std::ofstream fout(s);

    ///numarul de straturi <=> numarul de nivele - 1.
    fout << layerCount << '\n';

    for (int _ = 0; _ < layerCount; _++) {
      fout << layers[_].N << ' ' << layers[_].M << '\n'; ///marime input layer, marime output layer.
      fout << layers[_].W << '\n'; ///N x M.
      fout << layers[_].b << '\n'; ///M x 1.
    }

    fout.close();
  }

  void
  loadNetworkState(std::string inputPath)
  {
    std::ifstream fin(inputPath);

    int layerCount_; fin >> layerCount_;
    assert(layerCount == layerCount_);

    ///NU lasa cerr in cod ca se pune ca bad signal sau ceva de genul

    int i, j, z, N_, M_;
    for (z = 0; z < layerCount; z++) {
      fin >> N_ >> M_;
      assert(layers[z].N == N_);
      assert(layers[z].M == M_);

      for (i = 0; i < N_; i++) {
        for (j = 0; j < M_; j++) {
          fin >> layers[z].W(i, j);
        }
      }

      for (j = 0; j < M_; j++) {
        fin >> layers[z].b(j);
      }
    }

    fin.close();
  }
};
