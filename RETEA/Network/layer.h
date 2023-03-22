#pragma once

#include "utilities.h"

struct Layer
{
  std::mt19937 mt;

  int N; ///N = numar noduri pentru input <=> a0 <=> nivelul 0.
  int M; ///M = numar noduri pentru output <=> a1 <=> nivelul 1.

  ///W = matricea greutatilor.
  ///W[i][j] = muchie care uneste nodul i din coloana initiala cu nodul j din coloana finala.
  Eigen::MatrixXd W;

  ///b = vectorul biasurilor pentru nivelul output. b are M linii.
  Eigen::MatrixXd b;

  Layer() {
    mt.seed(time(NULL));
  }

  Layer(int N_, int M_, double ct): Layer() {
    N = N_;
    M = M_;

    ///ct = valoarea maxima in modul a unei greutati initiale.

    W.resize(N, M);
    for (int i = 0; i < N; i++) {
      for (int j = 0; j < M; j++) {
        W(i, j) = std::uniform_real_distribution<double>(-ct, ct)(mt);
      }
    }

    b = Eigen::MatrixXd::Zero(M, 1);
  }

  ///a0 = vectorul initial. orice element din a0 \in (0, 1).
  ///returneaza vectorul final <=> a1.
  Eigen::MatrixXd
  interpretInput(Eigen::MatrixXd &a0)
  {
    assert(N == a0.rows());
    assert(a0.cols() == 1);

    return util::sigmoid(W.transpose() * a0 + b);
  }

  ///a1 = aproximarea nivelului pentru un input.
  ///A <=> label-ul inputului, valoarea corecta de iesire pentru input.
  ///ans = un vector cu erori pt fiecare linie. norm2(a1, a) = sqrt(sum(ans))
  Eigen::MatrixXd
  computeErrors(Eigen::MatrixXd &a1, Eigen::MatrixXd &A)
  {
    assert(a1.rows() == A.rows());
    assert(a1.cols() == 1);
    assert(A.cols() == 1);

    Eigen::MatrixXd C(a1.rows(), 1);
    for (int i = 0; i < a1.rows(); i++) {
      C(i) = (A(i) - a1(i)) * (A(i) - a1(i));
    }

    return C;
  }

  ///a0 = vectorul initial.
  ///A = rezultatul pe care il vreau de la nivel pentru a0.
  ///ans.fi = matricea de gradienti (W)
  ///ans.se = vectorul de gradienti (b).
  ///ans.th = vectorul de gradienti (a0). (chiar daca nu pot sa schimb valorile de activare,
  ///tot sunt interesat de cum s-ar putea schimba pentru a avea a1 mai aproape de A).
  ///ans.th folositor la propagare inversa intre nivele.
  std::tuple<Eigen::MatrixXd, Eigen::MatrixXd, Eigen::MatrixXd>
  computeGradientsWba(Eigen::MatrixXd &a0, Eigen::MatrixXd &A)
  {
    Eigen::MatrixXd NablaW(N, M), Nablab(M, 1), Nablaa = Eigen::MatrixXd::Zero(N, 1);
    Eigen::MatrixXd a1 = interpretInput(a0);
    Eigen::MatrixXd C = computeErrors(a1, A);

    ///NablaW(i, j) = dC(j) / dW(i, j).
    ///<=> cu cat se schimba costul nodului j \in [1, m] din output daca schimb putin greutatea
    ///muchiei din nodul i \in [1, n] din input catre nodul j din output.

    ///Nablab(j) = dC(j) / db(j).
    ///<=> cu cat se schimba costul nodului j din output daca schimb putin biasul aceluiasi nod.

    ///Nablaa(i) = avg(dC(1)/da0(i) + dC(2)/da0(i) + .. + dC(M)/da0(i))
    ///<=> ? cu cat se schimba in medie costurile daca schimb putin activarea unui nod din a0.

    for (int j = 0; j < M; j++) {
      Nablab(j) = 2 * (a1(j) - A(j)) * util::sigmoidDeriv(a0.col(0).dot(W.col(j)) + b(j));
    }

    for (int i = 0; i < N; i++) {
      for (int j = 0; j < M; j++) {
        ///NablaW(i, j) = 2 * (a1(j) - A(j)) * a0(i) * sigmoidDeriv(a0.col(0).dot(W.col(j)) + b(j));
        NablaW(i, j) = Nablab(j) * a0(i);
      }
    }

    for (int i = 0; i < N; i++) {
      for (int j = 0; j < M; j++) {
        ///dC(j)/da0(i) = 2 * (a1(j) - A(j)) * W(i, j) * sigmoidDeriv(a0.col(0).dot(W.col(j)) + b(j));
        Nablaa(i) += Nablab(j) * W(i, j);
      }
      Nablaa(i) /= (double)M;
    }

    return std::make_tuple(NablaW, Nablab, Nablaa);
  }
};

