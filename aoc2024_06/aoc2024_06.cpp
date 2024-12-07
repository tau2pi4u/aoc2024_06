// aco2024_06.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <chrono>
#include <array>
#include <unordered_map>
#include <unordered_set>

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


// Functions from http://graphics.stanford.edu/%7Eseander/bithacks.html#InterleaveTableObvious
template <size_t dim>
struct MortonOrder
{
    MortonOrder()
    {
        _lookup = new std::array<std::array<uint32_t, dim>, dim>();
        for (int y = 0; y < dim; ++y)
        {
            for (int x = 0; x < dim; ++x)
            {
                (*_lookup)[y][x] = Generate(y, x);
            }
        }
    }

    ~MortonOrder()
    {
        delete _lookup;
    }

    uint32_t Generate(uint8_t y, uint8_t x)
    {
        unsigned int z = 0; // z gets the resulting Morton Number.

        for (int i = 0; i < sizeof(x) * 8; i++) // unroll for more speed...
        {
            z |= (x & 1U << i) << i | (y & 1U << i) << (i + 1);
        }
        return z;
    }

    uint32_t operator()(uint8_t y, uint8_t x) const
    {
        return (*_lookup)[y][x];
    }

    uint32_t operator()(size_t y, size_t x) const
    {
        return (*_lookup)[y][x];
    }

    std::array<std::array<uint32_t, dim>, dim> * _lookup;
};

static MortonOrder<256> g_mortonOrder;

uint32_t RoundUpPow2(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

uint32_t RoundUpPow2(size_t v)
{
    return RoundUpPow2(static_cast<uint32_t>(v));
}

template <typename T>
struct OneDVector
{
    OneDVector() : innerDim(0) {}

    template<typename T2>
    OneDVector(std::vector<std::vector<T2>> const& basis) : 
        innerDim(basis.front().size())
    {
        _vec.resize(RoundUpPow2(innerDim) * RoundUpPow2(basis.size()));
        for (size_t y = 0; y < basis.size(); ++y)
        {
            for (size_t x = 0; x < innerDim; ++x)
            {
                set(y, x, static_cast<T>(basis[y][x]));
            }
        }
    }

    void set(size_t y, size_t x, T const& val)
    {
        //_vec[y * innerDim + x] = val;
        uint32_t mo = g_mortonOrder(y, x);
        _vec[mo] = val;
    }

    T& get(size_t y, size_t x)
        requires(!std::is_same<T, bool>::value)
    {
        return _vec[g_mortonOrder(y, x)];
    }

    T const& get(size_t y, size_t x) const
    {
        return _vec[g_mortonOrder(y, x)];
    }

    size_t innerDim;
    std::vector<T> _vec;
};

int DirectionToX(Direction dir)
{
    static const int dirs[] = { 0, 1, 0, -1 };
    return dirs[static_cast<int>(dir)];
}

int DirectionToY(Direction dir)
{
    static const int dirs[] = { -1, 0, 1, 0 };
    return dirs[static_cast<int>(dir)];
}

Direction Rotate(Direction startDir, int count = 1)
{
    return static_cast<Direction>((DirTo<int>(startDir) + count) % DirTo<int>(Direction::Count));
}

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
                obstructionCount += _obstructions.back()[i];
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

        cells = OneDVector<Cell>(_cells);
        obstructions = OneDVector<char>(_obstructions);

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

    int _DirectionToX()
    {
        return DirectionToX(guard.dir);
    }

    int _DirectionToY()
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

    bool NextCellIsEmpty(int x, int y)
    {
        int dirX = _DirectionToX();
        int dirY = _DirectionToY();

        x += dirX;
        y += dirY;

        if (x < 0 || y < 0 || x >= boardX || y >= boardY) return false;

        return obstructions.get(y, x);
    }

    MoveType GuardStep()
    {
        int dirX = _DirectionToX();
        int dirY = _DirectionToY();

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
            auto& cell = cells.get(guard.y, guard.x);
            if (cell.dirMask & dirMask)
            {
                return MoveType::Cycle; // cycle 
            }
            cell.dirMask |= dirMask;
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
        guard.x = x - _DirectionToX();
        guard.y = y - _DirectionToY();

        return HasCycle();
    }

    std::vector<std::vector<bool>> _obstructions;
    std::vector<std::vector<Cell>> _cells;

    OneDVector<char> obstructions;
    OneDVector<Cell> cells;

    int boardX;
    int boardY;
    int obstructionCount = 0;

    Guard guard;
};

int Part2(Board & p1Board, int y)
{
    Board &board = p1Board;
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
    Board board = p1Board;
//#pragma omp parallel for
    for (int y = 0; y < p1Board.boardY; ++y)
    {
        count += Part2(board, y);
    }
    return count;
}

int GetNextTag()
{
    static int tag = 0;
    return ++tag;
}

struct StateNode
{
    Direction hitByGuardMovingInDir;
    int x;
    int y;
    bool valid;
    int tag = 0;
    StateNode* next = nullptr;
};

struct StateHasher
{
    uint32_t operator()(std::tuple<int, int, Direction> const& toHash) const
    {
        return (std::get<0>(toHash) << 16) | (std::get<1>(toHash) << 8) | static_cast<int>(std::get<2>(toHash));
    }
};

struct StateGraph
{
    StateGraph(Board const& board) :
        board(board),
        nodeCount((board.obstructionCount + 1) * DirTo<size_t>(Direction::Count) + 1)
    {
        nodes = new StateNode[nodeCount];

        nodes[0] = StateNode{ .hitByGuardMovingInDir = Direction::Count, .x = board.guard.x, .y = board.guard.y, .next = nullptr };

        size_t count = 1;
        for (int y = 0; y < board.boardY; ++y)
        {
            for (int x = 0; x < board.boardX; ++x)
            {
                if (!board.obstructions.get(y, x)) continue;

                for (int i = 0; i < DirTo<int>(Direction::Count); ++i)
                {
                    Direction dir = static_cast<Direction>(i);
                    if (count < nodeCount)
                    {
                        nodes[count] = StateNode{ .hitByGuardMovingInDir = dir, .x = x, .y = y, .next=nullptr };
                        stateToNode.insert({ std::make_tuple(x, y, dir), &nodes[count] });
                        ++count;
                    }
                }
            }
        }

        for (size_t i = 0; i < count; ++i)
        {
            ConnectNodeToGraph(nodes[i]);
        }
    }

    ~StateGraph()
    {
        delete nodes;
    }

    bool OutOfBounds(int x, int y)
    {
        return x < 0 || y < 0 || x >= board.boardX || y >= board.boardY;
    }

    void ConnectNodeToGraph(StateNode & node)
    {
        bool isFirst = node.hitByGuardMovingInDir == Direction::Count;

        Direction newDir;
        int x, y;

        // if (node.x == 3 && node.y == 17 && node.hitFrom == Direction::Left)__debugbreak();

        if (isFirst)
        {
            newDir = board.guard.dir;
            x = node.x;
            y = node.y;
        }
        else
        {
            // figure out where collision happened
            auto offsetX = DirectionToX(node.hitByGuardMovingInDir);
            auto offsetY = DirectionToY(node.hitByGuardMovingInDir);

            x = node.x - offsetX;
            y = node.y - offsetY;

            if (OutOfBounds(x, y)) { return; }

            if (board.obstructions.get(y, x))
            {
                return;
            }

            // figure out where to go next
            newDir = Rotate(node.hitByGuardMovingInDir, 1);
        }

        do
        {
            x += DirectionToX(newDir);
            y += DirectionToY(newDir);
            
            if (OutOfBounds(x, y)) break;
            if (!board.obstructions.get(y, x)) continue;
            if (stateToNode.count(std::make_tuple(x, y, newDir)) == 0) continue;

            node.next = stateToNode.at(std::make_tuple(x, y, newDir));
            break;
        } while (true);

        node.valid = true;
    }

    void AddObstruction(int x, int y)
    {
        size_t additionBase = board.obstructionCount * DirTo<size_t>(Direction::Count) + 1;
        for (int i = 0; i < DirTo<int>(Direction::Count); ++i)
        {
            auto& newNode = nodes[additionBase + i];

            newNode.valid = (board.cells.get(y, x).dirMask & (1 << i));
            if (!newNode.valid) continue;

            Direction hitFrom = static_cast<Direction>(i);
            newNode = StateNode{ .hitByGuardMovingInDir = hitFrom, .x = x, .y = y, .valid = true, .next = nullptr };

            int localX = x;
            int localY = y;

            Direction prevDir = Rotate(hitFrom, 3);
            int lookOffsetX = DirectionToX(prevDir);
            int lookOffsetY = DirectionToY(prevDir);

            Direction backwards = Rotate(hitFrom, 2);
            int dirX = DirectionToX(backwards);
            int dirY = DirectionToY(backwards);

            bool hit = false;

            while (!OutOfBounds(localX, localY) && !board.obstructions.get(localY, localX))
            {
                int searchX = localX + lookOffsetX;
                int searchY = localY + lookOffsetY;

                if (!OutOfBounds(searchX, searchY) &&
                    board.obstructions.get(searchY, searchX))
                {
                    auto& prevNode = stateToNode.at(std::make_tuple(searchX, searchY, prevDir));
                    savedNodeNextCache[prevNode] = prevNode->next;

                    prevNode->next = &newNode;
                    hit = true;
                }

                if (hitFrom == board.guard.dir && localX == board.guard.x && localY == board.guard.y)
                {
                    auto& startNode = nodes[0];
                    savedNodeNextCache[&startNode] = startNode.next;

                    startNode.next = &newNode;
                    hit = true;
                }

                localX += dirX;
                localY += dirY;
            }
           
            if (!hit) continue;
            ConnectNodeToGraph(newNode);
        }
    }

    void ResetObstruction()
    {
        for (auto& [ptr, prevNext] : savedNodeNextCache)
        {
            ptr->next = prevNext;
        }
        savedNodeNextCache.clear();
    }

    bool HasCycle(size_t startIdx)
    {
        int tag = GetNextTag();
        StateNode* node = &nodes[startIdx];

        while (node)
        {
            if (node->tag == tag) return true;

            node->tag = tag;

            node = node->next;
        }
        return false;
    }

    int PathLength()
    {
        std::unordered_set<int> seen;
        StateNode* node = &nodes[0];
        while (node)
        {
            auto const& next = node->next;
            if (!next) break;
            node = next;
        }
        return 0;
    }

    const size_t nodeCount;
    StateNode* nodes;

    const Board& board;

    std::unordered_map<std::tuple<int, int, Direction>, StateNode*, StateHasher> stateToNode;
    std::unordered_map<StateNode*, StateNode*> savedNodeNextCache;
};

int Part2YGraph(Board const& board, int y)
{
    StateGraph graph(board);
    int count = 0;
    for (int x = 0; x < board.boardX; ++x)
    {
        if (board.obstructions.get(y, x)) continue;
        if (!board.cells.get(y, x).dirMask) continue;
        graph.AddObstruction(x, y);
        bool hasCycle = graph.HasCycle(0);
        count += hasCycle;
        graph.ResetObstruction();
    }
    return count;
}

int Part2ParallelGraph(Board const& board)
{
    int count = 0;
//#pragma omp parallel for
    for (int y = 0; y < board.boardY; ++y)
    {
        count += Part2YGraph(board, y);
    }       
    return count;
}

int Part2SingleGraph(Board const& board, StateGraph & graph)
{
    int count = 0;
    for (int y = 0; y < board.boardY; ++y)
    {
        for (int x = 0; x < board.boardX; ++x)
        {
            if (board.obstructions.get(y, x)) continue;
            if (!board.cells.get(y, x).dirMask) continue;
            graph.AddObstruction(x, y);
            bool hasCycle = 0;
            for (size_t i = graph.nodeCount - 4; i < graph.nodeCount && !hasCycle; ++i)
            {
                if (!graph.nodes[i].valid) continue;
                hasCycle |= graph.HasCycle(0);
            }
            count += hasCycle;
            graph.ResetObstruction();
        }
    }
    return count;
}


struct Timer
{
    using time_t = std::chrono::time_point<std::chrono::high_resolution_clock>;

    void Begin()
    {
        start = std::chrono::high_resolution_clock::now();
    }

    void End(const char * s)
    {
        time_t stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
        total += duration;
        printf("%s: %lu us\n", s, duration);
    }

    void PrintTotal()
    {
        printf("Total: %lu us\n", total);
    }

    time_t start;
    size_t total = 0;
};

int main()
{
    Timer timer;

    timer.Begin();
    std::ifstream inputFile;
    inputFile.open("input.txt", std::ios::in);

    Board board(inputFile);
    Board p1Board = board;

    int p1 = p1Board.part1();
    p1Board.guard = board.guard;

    timer.End("Part1");
    timer.Begin();
    
    StateGraph graph(p1Board);
    int p2 = Part2SingleGraph(p1Board, graph);
    printf("p2: %d\n", p2);   

    timer.End("Part2");
    timer.PrintTotal();
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
