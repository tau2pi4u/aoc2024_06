import numpy as np
from typing import List
from copy import deepcopy

from multiprocessing import Pool

class PathFinder():
    def __init__(self, obstructions, guardX, guardY, guardDir, debug=False):
        self.obstructions = deepcopy(obstructions)
        self.visits = np.zeros((self.obstructions.shape[0], self.obstructions.shape[1], 4), dtype=bool)
        self.guardX = guardX
        self.guardY = guardY
        self.guardDir = guardDir
        self.status = -1
        self.debug = debug

        self.visits[self.guardY, self.guardX, self.guardDir] = True

    def PrintBoard(self):
        print("")
        for y in range(self.obstructions.shape[0]):
            s = ""
            for x in range(self.obstructions.shape[1]):
                horizontal = self.visits[y, x, 1] or self.visits[y, x, 3]
                vertical = self.visits[y, x, 0] or self.visits[y, x, 2]
                if y == self.guardY and x == self.guardX:
                    s += ['^', '>', 'v', '<'][self.guardDir]
                elif self.obstructions[y, x]:
                    s += '#'
                elif horizontal and vertical:
                    s += '+'
                elif vertical:
                    s += '|'
                elif horizontal:
                    s += '-'
                else:
                    s += '.'
            print(s)
        print("")

    def Step(self):
        """
        Returns: 
        0 -> continue
        1 -> left
        2 -> loop

        """
        if self.debug:
            self.PrintBoard()

        dirX = dirToX[self.guardDir]
        dirY = dirToY[self.guardDir]

        assert(self.guardDir < 4)

        nextX = self.guardX + dirX
        nextY = self.guardY + dirY

        if nextX < 0 or nextX >= self.obstructions.shape[1]:
            return 1

        if nextY < 0 or nextY >= self.obstructions.shape[0]:
            return 1
        
        if self.obstructions[nextY, nextX]:
            self.guardDir = (self.guardDir + 1) % 4
            return 0
        
        if self.visits[nextY, nextX, self.guardDir]:
            return 2
        
        self.visits[nextY, nextX, self.guardDir] = True
        self.guardX = nextX
        self.guardY = nextY

        return 0

    def Solve(self):
        while True:
            self.status = self.Step()
            if self.status != 0:
                break
        return self.visits.any(axis=-1).sum()

def p2():
    p2 = 0
    
    with open("pf2py.txt", "w") as toWrite:

        for y in range(obstructions.shape[0]):
            for x in range(obstructions.shape[1]):
                if not pf1.visits[y, x, :].any(): continue
                if obstructions[y, x]: continue

                cpy = deepcopy(obstructions)
                cpy[y, x] = True
                pf2 = PathFinder(cpy, guardX, guardY, guardDir)
                pf2.Solve()
                if (pf2.status == 2):
                    p2 += 1
                    toWrite.write(f"({x}, {y})\n")
    return p2


if __name__ == "__main__":
    guardX = -1
    guardY = -1
    guardDir = 0

    dirToX = [0, 1, 0, -1]
    dirToY = [-1, 0, 1, 0]

    with open("aoc2024_06/input.txt") as inputFile:
        _obstructions : List[List[bool]] = []

        for y, line in enumerate(inputFile):
            _obstructions.append(np.asarray([c == '#' for c in line.strip()]))
            for x, c in enumerate(line.strip()):
                if c == '^':
                    guardX = x
                    guardY = y

        obstructions = np.asarray(_obstructions, dtype=bool)

    pf1 = PathFinder(obstructions, guardX, guardY, guardDir)

    import timeit
    print(timeit.timeit(pf1.Solve, number=10000) / 10000)
    p1 = pf1.Solve()

    print(p1)
    print(p2())
