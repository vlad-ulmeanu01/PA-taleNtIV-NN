#python main.py 0/1 ./Replays ./Processed
#0 = departe de dusmani, 1 = aproape de dusmani.
#OLD:
#python main.py 0 100 ./Replays ./Processed
#python main.py 100 301 ./Replays ./Processed

import random
import json
import sys
import os

import fastCompresser
import helpFuncs

def getWinnerID(hltGame):
    id = 1
    for playerName in hltGame["player_names"]:
        if playerName == hltGame["winner"]:
            return id
        id += 1
    return -1
    
def processGame(fin, netEnemyType, fouts, offloadFrequencies):
    hltGame = json.load(fin)
    print(f"Names: {hltGame['player_names']}, Winner ID: {getWinnerID(hltGame)}")

    numFrames = hltGame["num_frames"] #numFrames cadre => numFrames-1 tranzitii.
    myID = getWinnerID(hltGame)
    maxProduction = max([max(line) for line in hltGame["productions"]])

    N, M = hltGame["height"], hltGame["width"]
    netInSize, netOutSize = (5+5+1) * (5+5+1) * 3 * 3, 5
    #pentru un patratel aliat, trebuie sa determin ce actiune ii aplic => netOutSize = 5.
    #vreau sa monitorizez o zona la 5 patratele distanta in oricare parte <=> zona 11x11.
    #pentru fiecare patratel am 3 tipuri: aliat, neutru, dusman
    #pentru fiecare tip am 3 variabile de mentinut: strength/production/distanta fata de patratel.
    #in general, modific datele ai 0 = prost 1 = bun.    
    
    #primele 11x11x3 patratele pt aliat, urm 11x11x3 pt neutru, ultimele 11x11x3 pentru dusman.
    #primele 3 din 11x11x3 pentru (-5, -5).
    
    remainingFarStills = helpFuncs.MAX_FAR_STILLS_PER_GAME
    frameIndexes = [(i, j) for i in range(N) for j in range(M)]

    for frameCnt in range(numFrames - 1):
        costFrame = helpFuncs.dijkstraOnFrame(hltGame["frames"][frameCnt], myID)
        distToEnemiesFrame = helpFuncs.distanceToEnemiesOnFrame(hltGame["frames"][frameCnt], myID)

        remainingFarStillsInFrame = remainingFarStills // (numFrames-1 - frameCnt)

        random.shuffle(frameIndexes)
        #dc iau un nr limitat de STILL pt dusmani departati, vr sa amestec indicii ai ii aleg uniform.
        for i, j in frameIndexes:
            if hltGame["frames"][frameCnt][i][j][0] == myID and\
               ((netEnemyType == 0 and distToEnemiesFrame[i][j] > helpFuncs.MAX_DIST_ENEMY) or\
               (netEnemyType == 1 and distToEnemiesFrame[i][j] <= helpFuncs.MAX_DIST_ENEMY)):


                netIn, netOut = [0] * netInSize, [0] * netOutSize
                
                move = hltGame["moves"][frameCnt][i][j]
                moveInd = 0 if move == 0 else 1


                if netEnemyType != 0 or moveInd != 0 or remainingFarStillsInFrame > 0:
                    if netEnemyType == 0 and moveInd == 0: #dusmani la distanta, verdict = STILL.
                        remainingFarStillsInFrame -= 1 #am o norma max de chestii la distanta
                        remainingFarStills -= 1 # cu STILL pe care vr sa le pun.
                    
                    #pun mutarea asteptata pe 1. (STILL, NORTH, EAST, SOUTH, WEST)
                    netOut[move] = 1
                    
                    #completez netIn.
                    #sunt la dy linii dx coloane fata de centru.
                    for dy in range(-5, 6):
                        for dx in range(-5, 6):
                            y, x = (i + dy) % N, (j + dx) % M
                            if hltGame["frames"][frameCnt][y][x][0] == myID: #ALLY
                                baseIndex = 11*11*3 * 0 + 3 * ((dy+5)*11 + dx+5)
                                
                                #strength.
                                netIn[baseIndex + 0] = helpFuncs.deflationFunction(
                                    abs(dy) + abs(dx)
                                ) * (hltGame["frames"][frameCnt][y][x][1] / 255)
                                
                                #production.
                                netIn[baseIndex + 1] = helpFuncs.deflationFunction(
                                    abs(dy) + abs(dx)
                                ) * (hltGame["productions"][y][x] / maxProduction)
                                
                                #cost cucerire patratel.
                                netIn[baseIndex + 2] = 0 #<=> nu imi pasa de el

                            elif hltGame["frames"][frameCnt][y][x][0] == 0: #NEUTRAL
                                baseIndex = 11*11*3 * 1 + 3 * ((dy+5)*11 + dx+5)
                                
                                #strength.
                                netIn[baseIndex + 0] = helpFuncs.deflationFunction(
                                    abs(dy) + abs(dx)
                                ) * (hltGame["frames"][frameCnt][y][x][1] / 255)
                                #(NU) cu cat are strength mai mic, cu atat mai bine.
                                #cred ca faceam o prostie cu 1 - ...
                                
                                #production.
                                netIn[baseIndex + 1] = helpFuncs.deflationFunction(
                                    abs(dy) + abs(dx)
                                ) * (hltGame["productions"][y][x] / maxProduction)
                                
                                #cost cucerire patratel.
                                netIn[baseIndex + 2] = helpFuncs.deflationDistance(
                                    costFrame[y][x]
                                )
                                
                            else: #ENEMY
                                baseIndex = 11*11*3 * 2 + 3 * ((dy+5)*11 + dx+5)
                                
                                #strength.
                                netIn[baseIndex + 0] = helpFuncs.deflationFunction(
                                    abs(dy) + abs(dx)
                                ) * (hltGame["frames"][frameCnt][y][x][1] / 255)
                                
                                #production.
                                netIn[baseIndex + 1] = helpFuncs.deflationFunction(
                                    abs(dy) + abs(dx)
                                ) * (hltGame["productions"][y][x] / maxProduction)
                                
                                #cost cucerire patratel.
                                netIn[baseIndex + 2] = helpFuncs.deflationDistance(
                                    costFrame[y][x] * 2
                                ) #l-am pus sa ia <=> 2x costul echivalent pentru
                                #un patratel neutru.

                    fastCompresser.offloadArray(fouts[moveInd], netIn)
                    fastCompresser.offloadArray(fouts[moveInd], netOut)
                    offloadFrequencies[moveInd] += 1


#deschid cate un .bin pentru STILL si MOVE.
fouts = []
offloadFrequencies = [0] * 2 #= cati vectori InOut bag in fiecare bin.
for dirName in helpFuncs.dirNames:
    fileCount = len(os.listdir(f"{sys.argv[3]}/{dirName}"))
    fouts.append(open(f"{sys.argv[3]}/{dirName}/{fileCount}.bin", "wb"))

path = sys.argv[2]
if os.path.isdir(path):
    for _1, _2, files in os.walk(path):
        for filename in files:
            if filename.endswith(".hlt"):
                with open(path + "/" + filename, "r") as fin:
                    processGame(fin, int(sys.argv[1]), fouts, offloadFrequencies)

for fout in fouts:
    fout.close()

print(f"Offload Frequencies = (STILL) {offloadFrequencies[0]}, (MOVE) {offloadFrequencies[1]}")

"""
fin = open("Replays/30x30-2-2047969.hlt")
fin.close()

print(hltGame["winner"])
print(hltGame["frames"][0][15][7])
print(hltGame["player_names"])

print(getWinnerID(hltGame))
"""
