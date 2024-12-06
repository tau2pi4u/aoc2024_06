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

enum class MoveType
{
    Normal,
    Rotate,
    Leave,
    Cycle,
    Count
};

struct Guard
{
    int x = 0;
    int y = 0;
    Direction dir = Direction::Count;
};

struct Board
{
    Board(std::ifstream& inputFile)
    {
        std::string line;
        while (std::getline(inputFile, line))
        {
            obstructions.push_back(std::vector<bool>());
            for (char c : line)
            {
                obstructions.back().push_back(c == '#');
                if (c == '^')
                {
                    guard.dir = Direction::Up;
                    guard.x = obstructions.back().size() - 1;
                    guard.y = obstructions.size() - 1;
                }
            }
            boardX = obstructions.back().size();
            visited.push_back(std::vector<char>(boardX));
        }
        boardY = obstructions.size();
        visited[guard.y][guard.x] |= DirToMask();
    }

    void Reset(Guard const& resetGuard)
    {
        for (auto & row : visited)
        {
            for (auto & cell : row)
            {
                cell = 0;
            }
        }
        guard = resetGuard;
    }

    int DirectionToX()
    {
       /* switch (guard.dir)
        {
        case Direction::Up:
        case Direction::Down:
            return 0;
        case Direction::Left:
            return -1;
        case Direction::Right:
            return 1;
        }*/

        //Up,
        //Right,
        //Down,
        //Left,

        static const int dirs[] = { 0, 1, 0, -1 };
        return dirs[static_cast<int>(guard.dir)];
    }

    int DirectionToY()
    {
       /* switch (guard.dir)
        {
        case Direction::Left:
        case Direction::Right:
            return 0;
        case Direction::Up:
            return -1;
        case Direction::Down:
            return 1;
        }*/
        static const int dirs[] = { -1, 0, 1, 0 };
        return dirs[static_cast<int>(guard.dir)];
    }

    char DirToMask()
    {
        return 1 << static_cast<char>(guard.dir);
    }

    bool Leaves(int dirX, int dirY)
    {
        int nextX = guard.x + dirX;
        int nextY = guard.y + dirY;

        if (nextX < 0 || nextX >= boardX) return true;
        if (nextY < 0 || nextY >= boardY) return true;
        return false;
    }

    bool Obstructed(int dirX, int dirY)
    {
        int nextX = guard.x + dirX;
        int nextY = guard.y + dirY;

        if (Leaves(dirX, dirY)) return false;

        return obstructions[nextY][nextX];
    }

    MoveType FindGuardStepType(int dirX, int dirY)
    {
        if (Leaves(dirX, dirY))
        {
            return MoveType::Leave;
        }

        if (Obstructed(dirX, dirY))
        {
            return MoveType::Rotate;
        }

        return MoveType::Normal;
    }

    MoveType GuardStep()
    {
        int dirX = DirectionToX();
        int dirY = DirectionToY();

        MoveType moveType = FindGuardStepType(dirX, dirY);

        switch (moveType)
        {
        case MoveType::Normal:
        {
            guard.x += dirX;
            guard.y += dirY;

            char dirMask = DirToMask();
            if (visited[guard.y][guard.x] & dirMask)
            {
                return MoveType::Cycle; // cycle 
            }
            visited[guard.y][guard.x] |= dirMask;
            break;
        }
        case MoveType::Rotate:
            guard.dir = static_cast<Direction>((static_cast<int>(guard.dir) + 1) % static_cast<int>(Direction::Count));
            break;
        case MoveType::Leave:
            break;
        default:
            __debugbreak(); 
        }

        return moveType;
    }

    int part1()
    {
        while (GuardStep() != MoveType::Leave);
        int count = 0;
        for (auto const& row : visited)
        {
            for (auto const& cell : row)
            {
                count += !!cell;
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

    int part2(Board const& p1Board)
    {
        auto gCopy = guard;
        int count = 0;
        for (size_t y = 0; y < obstructions.size(); ++y)
        {
            for (size_t x = 0; x < obstructions[y].size(); ++x)
            {
                if (obstructions[y][x]) continue;
                if (!p1Board.visited[y][x]) { continue; }
                obstructions[y][x] = true;
                count += HasCycle();
                obstructions[y][x] = false;
                Reset(gCopy);
            }
        }
        return count;
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
    std::vector<std::vector<char>> visited;

    int boardX;
    int boardY;

    Guard guard;
};

int main()
{
    std::ifstream inputFile;
    inputFile.open("input.txt", std::ios::in);

    Board board(inputFile);

    Board p1Board = board;

    int p1 = p1Board.part1();
    

    printf("p1: %d\n", p1);

    auto durationSum = 0;
    int iters = 16;
    int p2;
    for (int i = 0; i < iters; ++i)
    {
        auto start = std::chrono::high_resolution_clock::now();
        p2 = board.part2(p1Board);
        auto stop = std::chrono::high_resolution_clock::now();
        durationSum += std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    }

    printf("p2: %d\n%llu ms\n", p2, durationSum / iters);
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