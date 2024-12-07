import numpy as np
from typing import List
from copy import deepcopy

# Directions:
# Up = 0, Right = 1, Down = 2, Left = 3
# Two arrays for the change in dimension (+ve y is down, +ve x is right)
dirToX = [0, 1, 0, -1]
dirToY = [-1, 0, 1, 0]

class PathFinder():
    """
    Class to check the path of the guard on a board
    Obstructions are tracked as a 2D numpy array of booleans - true if obstructed, false if not
    Visits are tracked as a 3D array - the extra dimension is for each direction of visit
    """

    def __init__(self, obstructions, guardX, guardY, guardDir, debug=False):
        # Track obstructions and visits
        self.obstructions = deepcopy(obstructions)
        self.visits = np.zeros((self.obstructions.shape[0], self.obstructions.shape[1], 4), dtype=bool)

        # Keep track of the guard
        self.guardX = guardX
        self.guardY = guardY
        self.guardDir = guardDir

        # keep track of our status (see Step for what the values mean)
        self.status = -1
        self.debug = debug

        # Mark our initial visit
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
        Have the guard a single step 
        Returns: 
        0 -> keep walking
        1 -> guard left
        2 -> found loop
        """
        
        # just for debugging
        if self.debug:
            self.PrintBoard()

        # see which direction to move in
        dirX = dirToX[self.guardDir]
        dirY = dirToY[self.guardDir]

        assert(self.guardDir < 4)

        # find next cell
        nextX = self.guardX + dirX
        nextY = self.guardY + dirY

        # Check we don't go out of bounds
        if nextX < 0 or nextX >= self.obstructions.shape[1]:
            return 1 # Guard left

        if nextY < 0 or nextY >= self.obstructions.shape[0]:
            return 1 # Guard left
        
        # Check if we've hit an obstruction
        if self.obstructions[nextY, nextX]:
            # Rotate the guard (directions ordered so that + 1 mod 4 is a 90 degree rotation)
            print(f"\t ({nextX}, {nextY}) ({self.guardX}, {self.guardY})")
            self.guardDir = (self.guardDir + 1) % 4
            return 0 # Rotate, continue

        # Check if we've been in this cell before while facing the same direction
        if self.visits[nextY, nextX, self.guardDir]:
            return 2 # Loop
        
        # Mark that we've been here
        self.visits[nextY, nextX, self.guardDir] = True

        # Update our position
        self.guardX = nextX
        self.guardY = nextY

        return 0

    def Solve(self):
        """
        Walks until we hit a loop or exit the grid
        """
        while True:
            self.status = self.Step()
            if self.status != 0:
                break
        
        # Counts cells for which any of the direction bits are set
        return self.visits.any(axis=-1).sum()

def p2():
    p2 = 0
    
    with open("pf2py.txt", "w") as toWrite:
        # For every cell
        for y in range(7, obstructions.shape[0]):
            for x in range(34, obstructions.shape[1]):
                # skip cells which aren't visited in p1 - we can never get there
                if not pf1.visits[y, x, :].any(): continue

                # Copy the obstructions to avoid interfering with ourselves
                cpy = deepcopy(obstructions)
                # Obstruct our cell
                cpy[y, x] = True

                # Solve
                pf2 = PathFinder(cpy, guardX, guardY, guardDir)
                pf2.Solve()

                # Check for 
                if (pf2.status == 2):
                    p2 += 1
                    toWrite.write(f"({x}, {y})\n")
    return p2

def p1():
    PathFinder(obstructions, guardX, guardY, guardDir)
    return pf1.Solve()

if __name__ == "__main__":
    guardX = -1
    guardY = -1
    guardDir = 0

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

    print(p1())
    print(p2())
