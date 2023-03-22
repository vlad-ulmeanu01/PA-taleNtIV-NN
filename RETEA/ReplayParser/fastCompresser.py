import bitarray, bitarray.util

#scrie in fout = format bin.
#caractere = {NULL, '0', '1', .., '9', '.', 'x', '\n'}.
#NULL == 0000, se ignora
#'0'  == 0001, '9' == 1010, '.' == 1011, 'x' == 1100, '\n' == 1101.
#cand am 3489 de 0, scriu "x3489\n"
#practic 1 ch <=> 1 nibble.

def __scrieX0(fout, streak):
    bt = bitarray.bitarray(endian = "big")
    bt.extend("1100") #'x'.
    for cifra in str(streak):
        bt.extend(bitarray.util.int2ba(int(cifra) + 1, length = 4))
    bt.extend("1101") #'\n'.
    fout.write(bt.tobytes()) #daca nr biti e M8 + 4, mai adauga 0000 <=> NULL, care se ignora la citire.

def __scrieFloat(fout, x):
    bt = bitarray.bitarray(endian = "big")
    #nu mai scriu 0 de dinaintea punctului.
    bt.extend("1011") #"."
    for cifra in "{0:.6f}".format(x).split('.')[1]: #trebuie precizie fixa...
        bt.extend(bitarray.util.int2ba(int(cifra) + 1, length = 4))
    bt.extend("1101") #'\n'.
    fout.write(bt.tobytes())
   

#afiseaza un vector de double in fout, cate un numar pe linie.
#daca exista o secventa de ? 0-uri, in loc de ele se va afisa "x ?" pe o linie.
def offloadArray(fout, arr):
    streak = 0
    for x in arr:
        if x == 0:
            streak += 1
        else:
            if streak:
                __scrieX0(fout, streak)
                streak = 0
            if x == 1:
                fout.write(b"\x2D") # 0010 cu 1101. 1 + 4 + 8 + 32 = 45
            else:
                __scrieFloat(fout, x)
    if streak:
        __scrieX0(fout, streak)
