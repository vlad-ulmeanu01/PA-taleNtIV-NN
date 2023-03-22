from collections import deque
import heapq

STILL, NORTH, EAST, SOUTH, WEST = range(5)
ALLY, NEUTRAL, ENEMY = range(3)
dirNames = ["STILL", "MOVE"]
MAX_DIST_ENEMY = 4
MAX_FAR_STILLS_PER_GAME = 22500 #pt reteaua cu dusmani la departare.

#indicii unui vecin. get(.., P, STILL) = P.
def getNeighbour(N, M, P, dir):
    dy, dx = ((0, 0), (-1, 0), (0, 1), (1, 0), (0, -1))[dir]
    return ((P[0] + dy) % N, (P[1] + dx) % M) #linie, coloana.

#pe o tabla circulara N linii x M coloane, cat de departate sunt P1 si P2? (distanta Manhattan).
def getDistance(N, M, P1, P2):
    dy = min(abs(P1[0] - P2[0]), P1[0] + N - P2[0], P2[0] + N - P1[0])
    dx = min(abs(P1[1] - P2[1]), P1[1] + M - P2[1], P2[1] + M - P1[1])
    return dx + dy

#se foloseste pentru a scadea exponential interesul pentru patratele aflate la o distanta mai mare de
#patratelul analizat acum.
def deflationFunction(x):
    return 1 if x == 0 else 2 ** ((1-x) / 3)

#folosit pentru a transforma distanta minima catre un patratel intr-o valoare de activare a unui neuron.
#vreau f(0) = 1, f(255*10) ~= 0, f(255*2) = 0.5; pp ca majoritatea costurilor sunt relativ mici fata
#de capatul maxim posibil.
def deflationDistance(x):
    #x \in [0, 2550].
    #daca fac aproximare liniara, ramane ft putin sensitiv intervalul care chiar ma intereseaza pe mine
    #in general.
    return 2 ** (-x / 512)

#pentru fiecare patratel care nu este al meu, vreau sa stiu suma minima a costurilor necesara pentru
#a captura acel patratel.
#frame este o matrice 30 pe 30 (in general), iar fiecare casuta are o pereche tip (cine detine casuta, 
#[0, 255] = strengthul patraticii respective.)
def dijkstraOnFrame(frame, myID):
    pq = []
    #pq este minHeap si tine perechi de forma (distanta minima bagata in heap (poate s-a modificat in timp)
    #                                          , punctul aferent ei)
    #consider ca intre doua casute <=> noduri A, B, costul muchiei A -> B este valoarea din nodul B.

    N, M = len(frame), len(frame[0])
    costFrame = [[(1<<30) for j in range(M)] for i in range(N)]

    
    for i in range(N): #ce este deja controlat are costul 0.
        for j in range(M):
            if frame[i][j][0] == myID:
                costFrame[i][j] = 0
                heapq.heappush(pq, (0, (i, j)))

    while len(pq):
        cost, P = heapq.heappop(pq)
        i, j = P

        if costFrame[i][j] < cost: #nu mai este de actualitate varful heapului.
            continue

        for dir in range(1, 5):
            Pnou = getNeighbour(N, M, P, dir)

            if costFrame[Pnou[0]][Pnou[1]] > costFrame[i][j] + frame[Pnou[0]][Pnou[1]][1]:
                costFrame[Pnou[0]][Pnou[1]] = costFrame[i][j] + frame[Pnou[0]][Pnou[1]][1]
                heapq.heappush(pq, (costFrame[Pnou[0]][Pnou[1]], Pnou))

    return costFrame

def distanceToEnemiesOnFrame(frame, myID):
    N, M = len(frame), len(frame[0])
    costFrame = [[(1<<30) for j in range(M)] for i in range(N)]

    qu = deque()
    for i in range(N):
        for j in range(M):
            if frame[i][j][0] != myID and frame[i][j][0] != 0:
                costFrame[i][j] = 0
                qu.append((i, j))

    while len(qu):
        P = qu.popleft()
        for dir in range(1, 5):
            Pnou = getNeighbour(N, M, P, dir)
            if costFrame[Pnou[0]][Pnou[1]] > costFrame[P[0]][P[1]] + 1:
                costFrame[Pnou[0]][Pnou[1]] = costFrame[P[0]][P[1]] + 1
                qu.append(Pnou)

    return costFrame

def arrayToString(arr: list) -> str:
    s = ""
    for x in arr:
        s += str(round(x, 5))
        s += "\n"
    return s
