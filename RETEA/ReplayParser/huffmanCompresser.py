import bitarray, bitarray.util, heapq, copy, os


class HuffmanNode:
    def __init__(self, lson, rson, ch, fv):
        self.lson = lson
        self.rson = rson
        self.ch = ch
        self.fv = fv

    def __lt__(self, oth: "HuffmanNode"):
        return self.fv < oth.fv
    
    def __str__(self):
        return str(self.ch)


def __df(node: HuffmanNode, bt: bitarray, codes: dict):
    if node == None:
        bt.pop()
        return
    if node.ch != None:
        codes[node.ch] = copy.deepcopy(bt)
        bt.pop()
        return
    bt.append(0)
    __df(node.lson, bt, codes)   
    bt.append(1)
    __df(node.rson, bt, codes)    
    if len(bt): #daca sunt in radacina si vr sa ies <=> ultimul pas, nu mai am nimic de scos.
        bt.pop()

def encodeString(s: str, outputPath: str):
    s += "\x03" #"\x03" = ETX.
    t = sorted(s)
    heap = []
    i = 0
    while i < len(t):
        j = i
        while j < len(t) and t[j] == t[i]:
            j += 1
        heapq.heappush(heap, HuffmanNode(None, None, t[i], j-i))
        i = j

    while len(heap) > 1:
        nodeI, nodeII = heapq.heappop(heap), heapq.heappop(heap)
        heapq.heappush(heap, HuffmanNode(nodeI, nodeII, None, nodeI.fv + nodeII.fv))
    
    codes = {} #perechi tip <caracter, bitset>.
    root = heapq.heappop(heap)
    __df(root, bitarray.bitarray(endian = "big"), codes)

    #print(codes)

    bt = bitarray.bitarray(endian = "big")
    for ch in s:
        bt.extend(codes[ch])

    #print(bt)

    #fileCount = len([name for name in os.listdir(outputPath) if os.path.isfile(name)])
    #bin nu se pune? hmm
    fileCount = len(os.listdir(outputPath))
   
    fout = open(f"{outputPath}/{fileCount}.bin", "wb")

    #intai scriu perechile tip <caracter, bitset>.
    #am nevoie de len(orice bitset) <= 255.

    for ch in codes:
        chBt = codes[ch]
        fout.write(ch.encode()) #scriu un byte cu caracterul.
        fout.write(bitarray.util.int2ba(len(chBt), length = 8).tobytes()) #scriu un byte cu lungimea bitsetului.
        fout.write(chBt.tobytes()) #scriu bitstringul asociat, dau pad cu 0 la DREAPTA (deci dupa bitstring)

    fout.write(b"\x00") #bag un NULL ca sa semnific trecerea de la descriere perechi la textul compresat efectiv.

    fout.write(bt.tobytes()) #la fel, scriu textul bitstring asociat.
    #vine padding la dreapta; nu imi pasa ptc ma opresc imediat dupa ce am decodificat ETX.

    fout.close()