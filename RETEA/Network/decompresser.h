#pragma once

#include "utilities.h"

///decompreseaza date dintr-un bin. folosit pentru a obtine in/out pentru retea.
namespace decomp {
  const int inPrecision = 6; ///cate zecimale am in input.
  double p10[inPrecision] = {0.1, 0.01, 0.001, 0.0001, 0.00001, 0.000001};

  /**
  NULL = 0000 (SKIP)
     0 = 0001
     1 = 0010
     2 = 0011
     3 = 0100
     4 = 0101
     5 = 0110
     6 = 0111
     7 = 1000
     8 = 1001
     9 = 1010 A
     . = 1011 B
     x = 1100 C
    \n = 1101 D
  **/

  ///citeste un byte din fin in y.
  void __readByte(FILE *fin, uint8_t &y) {
    while (1) {
      if (fread(&y, sizeof(uint8_t), 1, fin) == 1) {
        return;
      } else { ///ma duc la inceputul fisierului.
        fseek(fin, 0, SEEK_SET);
      }
    }
  }

  ///baga un nibble din fin in x. (pe cele mai nesemnificative 4 pozitii).
  void __readNibble(FILE *fin, uint8_t &x) {
    static uint8_t y = 0;

    while (y == 0) { ///daca tb sa citesc un byte intreg/sa sar peste NULL == 0000.
      __readByte(fin, y);
    }

    if ((y & 0xF0) != 0) { ///returnez primii 4 biti, y pastreaza ultimii 4 biti.
      x = y >> 4;
      y = y & 0xF;
    } else { ///returnez ultimii 4 biti. marchez y ai data viitoare sa mai citesc.
      x = y;
      y = 0;
    }
  }

  double __readNumber(FILE *fin) {
    static int remainingStreak = 0; ///streak = cati de 0 mai am de parcurs.

    if (remainingStreak > 0) {
      remainingStreak--;
      return 0;
    }

    uint8_t x = 0;

    __readNibble(fin, x);

    if (x == 0x2) { ///<=> am citit '1'
      __readNibble(fin, x); ///citit '\n'.
      return 1;
    }

    if (x == 0xC) { ///<=> am citit 'x'.
      __readNibble(fin, x);

      while (x != 0xD) { ///cat timp nu am '\n'.
        remainingStreak = remainingStreak * 10 + (x - 1);
        __readNibble(fin, x);
      }

      remainingStreak--;
      return 0;
    }

    ///am citit '.'.
    assert(x == 0xB);
    double ans = 0;

    for (int _ = 0; _ < inPrecision; _++) {
      __readNibble(fin, x);
      ans += p10[_] * (x - 1);
    }

    __readNibble(fin, x); ///citit '\n'.

    return ans;
  }

  void parseVector(FILE *fin, Eigen::MatrixXd &m) {
    assert(m.cols() == 1);

    for (int i = 0; i < m.rows(); i++) {
      m(i) = __readNumber(fin);
    }
  }

//int main() {
//  FILE *fin = fopen("Processed/MOVE/0.bin", "rb");
//
//  uint8_t x = 0;
//
//  Eigen::MatrixXd netIn(1089, 1), netOut(5, 1);
//
//  parseVector(fin, netIn);
//  parseVector(fin, netOut);
//
//  std::cout << netIn << "\n---\n" << netOut << '\n';
//
//  fclose(fin);
//
//  return 0;
//}
}
