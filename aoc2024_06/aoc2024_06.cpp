// aco2024_06.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <chrono>

enum class Direction
{
    Up,
    Right,
    Down, 
    Left,
    Count
};

template <typename T>
T DirTo(Direction dir)
{
    return static_cast<T>(dir);
}

enum class MoveType
{
    Normal = 0,
    Rotate = 1,
    Leave = 2,
    Cycle = 3,
    Count
};

struct Guard
{
    int x = 0;
    int y = 0;
    Direction dir = Direction::Count;
};

struct Cell
{
    //uint8_t obstructedInDir;
    uint8_t dirMask;
};

template <typename T>
struct OneDVector
{
    T& operator[](size_t y, size_t x)
    {
        return _vec[y * innerDim + x];
    }

    T const& operator[](size_t y, size_t x)
    {
        return _vec[y * innerDim + x];
    }

    size_t innerDim;
    std::vector<T> _vec;
};

struct Board
{
    Board(std::ifstream& inputFile)
    {
        std::string line;
        while (std::getline(inputFile, line))
        {
            cells.push_back(std::vector<Cell>(line.size()));
            obstructedInDir.push_back(std::vector<uint8_t>(line.size()));
            obstructions.push_back(std::vector<bool>(line.size()));
            for (size_t i = 0; i < line.size(); ++i)
            {
                auto const& c = line[i];
                obstructions.back()[i] = (c == '#');
                if (c == '^')
                {
                    guard.dir = Direction::Up;
                    guard.x = static_cast<int>(i);
                    guard.y = static_cast<int>(cells.size() - 1);
                }
            }
            boardX = static_cast<int>(cells.back().size());
        }

        boardY = static_cast<int>(cells.size());
        cells[guard.y][guard.x].dirMask |= DirToMask();

        for (int y = 0; y < boardY; ++y)
        {
            for (int x = 0; x < boardX; ++x)
            {
                if (!obstructions[y][x]) continue;
                ObstructAdjacent(x, y);
            }
        }
    }
    
    bool GuardOutOfBounds()
    {
        return guard.x < 0 || guard.y < 0 || guard.x >= boardX || guard.y >= boardY;
    }

    void SaveAdjacent(int x, int y)
    {
        for (int i = 0; i < DirTo<int>(Direction::Count); ++i)
        {
            Direction dir = static_cast<Direction>(i);
            int newY = y + DirectionToY(dir);
            int newX = x + DirectionToX(dir);
            if (newY < 0 || newX < 0 || newY >= boardY || newX >= boardX) continue;
            adjactentSaved[i] = obstructedInDir[newY][newX];
        }
    }

    void ResetAdjacent(int x, int y)
    {
        for (int i = 0; i < DirTo<int>(Direction::Count); ++i)
        {
            Direction dir = static_cast<Direction>(i);
            int newY = y + DirectionToY(dir);
            int newX = x + DirectionToX(dir);
            if (newY < 0 || newX < 0 || newY >= boardY || newX >= boardX) continue;
            obstructedInDir[newY][newX] = adjactentSaved[i];
        }
    }

    void Obstruct(int x, int y, Direction fromDir)
    {
        Direction rotated = static_cast<Direction>((DirTo<int>(fromDir) + 2) % DirTo<int>(Direction::Count));
        x += DirectionToX(rotated);
        y += DirectionToY(rotated);

        if (x < 0 || y < 0 || x >= boardX || y >= boardY) return;

        obstructedInDir[y][x] |= 1 << DirTo<int>(fromDir);
    }

    void ObstructAdjacent(int x, int y)
    {
        for (int i = 0; i < DirTo<int>(Direction::Count); ++i)
        {
            Direction dir = static_cast<Direction>(i);
            Obstruct(x, y, dir);
        }
    }

    void Reset(Guard const& resetGuard)
    {
        for (auto& row : cells)
        {
            for (auto& cell : row)
            {
                cell.dirMask = 0;
            }
        }
        guard = resetGuard;
    }

    int DirectionToX(Direction dir)
    {
        static const int dirs[] = { 0, 1, 0, -1 };
        return dirs[static_cast<int>(dir)];
    }

    int DirectionToX()
    {
        return DirectionToX(guard.dir);
    }

    int DirectionToY(Direction dir)
    {
        static const int dirs[] = { -1, 0, 1, 0 };
        return dirs[static_cast<int>(dir)];
    }

    int DirectionToY()
    {
        return DirectionToY(guard.dir);
    }

    char DirToMask()
    {
        return 1 << static_cast<char>(guard.dir);
    }

    bool Leaves(int dirX, int dirY)
    {
        int nextX = guard.x + dirX;

        if (nextX < 0 || nextX >= boardX) return true;

        int nextY = guard.y + dirY;
        if (nextY < 0 || nextY >= boardY) return true;
        return false;
    }

    MoveType GuardStep()
    {
        int dirX = DirectionToX();
        int dirY = DirectionToY();

        guard.x += dirX;
        guard.y += dirY;

        if (GuardOutOfBounds())return MoveType::Leave;

        auto const& obstructed = obstructions[guard.y][guard.x];

        if (obstructed)
        {
            guard.x -= dirX;
            guard.y -= dirY;
            guard.dir = static_cast<Direction>((static_cast<int>(guard.dir) + 1) % static_cast<int>(Direction::Count));
            return MoveType::Rotate;
        }
        else
        {
            auto dirMask = DirToMask();
            if (cells[guard.y][guard.x].dirMask & dirMask)
            {
                return MoveType::Cycle; // cycle 
            }
            cells[guard.y][guard.x].dirMask |= dirMask;
            return MoveType::Normal;
        }
    }

    bool HasVisited(int x, int y) const
    {
        return cells[y][x].dirMask;
    }

    int part1()
    {
        while (GuardStep() != MoveType::Leave);
        int count = 0;

        for (int y = 0; y < boardY; ++y)
        {
            for (int x = 0; x < boardX; ++x)
            {
                count += HasVisited(x, y);
            }
        }
        return count;
    }

    bool HasCycle()
    {
        while (true)
        {
            auto mt = GuardStep();
            if (mt == MoveType::Leave) break;
            if (mt == MoveType::Cycle)
            {
                return true;
                break;
            }
        }
        return false;
    }

    bool part2(Board const& p1Board, int x, int y)
    {
        auto gCopy = guard;
        int count = 0;

        if (obstructions[y][x]) return false;
        if (!p1Board.HasVisited(x, y)) { return false; }

        obstructions[y][x] = true;
        bool cycle = HasCycle();
        obstructions[y][x] = false;
        return cycle;
    }

    bool TryFromDir(int x, int y, int dirInt)
    {
        Direction dir = static_cast<Direction>(dirInt);
        guard.dir = dir;
        guard.x = x - DirectionToX();
        guard.y = y - DirectionToY();

        return HasCycle();
    }

    std::vector<std::vector<bool>> obstructions;
    std::vector<std::vector<uint8_t>> obstructedInDir;
    ////std::vector<std::vector<char>> visited;
    //std::vector<std::vector<bool>> visited[static_cast<size_t>(Direction::Count)];

    std::vector<std::vector<Cell>> cells;

    int boardX;
    int boardY;

    char adjactentSaved[4] = { 0 };

    Guard guard;
};

int Part2(Board const& p1Board, int y)
{
    Board board = p1Board;
    int count = 0;
    for (int x = 0; x < p1Board.boardX; ++x)
    {
        count += board.part2(p1Board, x, y);
        board.Reset(p1Board.guard);
    }
    return count;
}

int Part2(Board const& p1Board)
{
    int count = 0;
#pragma omp parallel for
    for (int y = 0; y < p1Board.boardY; ++y)
    {
        count += Part2(p1Board, y);
    }
    return count;
}


int main()
{
    std::ifstream inputFile;
    inputFile.open("input.txt", std::ios::in);

    Board board(inputFile);

    Board p1Board = board;

    int p1 = p1Board.part1();
    p1Board.guard = board.guard;
    

    printf("p1: %d\n", p1);

    auto durationSum = 0;
    int iters = 1 << 4;
    int p2;
    for (int i = 0; i < iters; ++i)
    {
        auto start = std::chrono::high_resolution_clock::now();
        p2 = Part2(p1Board);
        auto stop = std::chrono::high_resolution_clock::now();
        durationSum += std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
    }

    printf("p2: %d\n%llu us\n", p2, durationSum / iters);
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
