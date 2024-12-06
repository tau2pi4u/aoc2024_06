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
    int16_t x = 0;
    int16_t y = 0;
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
    OneDVector() : innerDim(0) {}
    OneDVector(std::vector<std::vector<T>> const& basis) : 
        innerDim(basis.front().size())
    {
        _vec.resize(innerDim * basis.size());
        for (size_t y = 0; y < basis.size(); ++y)
        {
            for (size_t x = 0; x < innerDim; ++x)
            {
                set(y, x, basis[y][x]);
            }
        }
    }

    void set(size_t y, size_t x, T const& val)
    {
        _vec[y * innerDim + x] = val;
    }

    T& get(size_t y, size_t x)
        requires(!std::is_same<T, bool>::value)
    {
        return _vec[y * innerDim + x];
    }

    T const& get(size_t y, size_t x) const
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
            _cells.push_back(std::vector<Cell>(line.size()));
            _obstructions.push_back(std::vector<bool>(line.size()));
            for (size_t i = 0; i < line.size(); ++i)
            {
                auto const& c = line[i];
                _obstructions.back()[i] = (c == '#');
                if (c == '^')
                {
                    guard.dir = Direction::Up;
                    guard.x = static_cast<int>(i);
                    guard.y = static_cast<int>(_cells.size() - 1);
                }
            }
            boardX = static_cast<int>(_cells.back().size());
        }

        boardY = static_cast<int>(_cells.size());
        _cells[guard.y][guard.x].dirMask |= DirToMask();

        cells = OneDVector(_cells);
        obstructions = OneDVector(_obstructions);

        _cells.clear();
        _obstructions.clear();
    }
    
    bool GuardOutOfBounds()
    {
        return guard.x < 0 || guard.y < 0 || guard.x >= boardX || guard.y >= boardY;
    }

    void Reset(Guard const& resetGuard)
    {
        /*for (auto& row : cells)
        {
            for (auto& cell : row)
            {
                cell.dirMask = 0;
            }
        }*/

        std::fill(cells._vec.begin(), cells._vec.end(), Cell());
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

        if (GuardOutOfBounds()) return MoveType::Leave;

        //auto const& obstructed = obstructions.get(guard.y, guard.x);

        if (obstructions.get(guard.y, guard.x))
        {
            guard.x -= dirX;
            guard.y -= dirY;
            guard.dir = static_cast<Direction>((static_cast<int>(guard.dir) + 1) % static_cast<int>(Direction::Count));
            return MoveType::Rotate;
        }
        else
        {
            auto dirMask = DirToMask();
            if (cells.get(guard.y, guard.x).dirMask & dirMask)
            {
                return MoveType::Cycle; // cycle 
            }
            cells.get(guard.y, guard.x).dirMask |= dirMask;
            return MoveType::Normal;
        }
    }

    bool HasVisited(int x, int y) const
    {
        return cells.get(y, x).dirMask;
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

        if (obstructions.get(y, x)) return false;
        if (!p1Board.HasVisited(x, y)) { return false; }

        obstructions.set(y, x, true);
        bool cycle = HasCycle();
        obstructions.set(y, x, false);
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

    std::vector<std::vector<bool>> _obstructions;
    std::vector<std::vector<Cell>> _cells;

    OneDVector<bool> obstructions;
    OneDVector<Cell> cells;

    int boardX;
    int boardY;

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

    size_t durationSum = 0;
    int iters = 1 << 10;
    int p2;
    for (int i = 0; i < iters; ++i)
    {
        auto start = std::chrono::high_resolution_clock::now();
        p2 = Part2(p1Board);
        auto stop = std::chrono::high_resolution_clock::now();
        durationSum += std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
    }

    printf("p2: %d\n%.2f ms\n", p2, static_cast<float>(durationSum) / (1000 * iters));
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
