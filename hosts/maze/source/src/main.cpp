#include <cassert>
#include <functional>
#include <vector>
#include <stack>
#include <array>
#include <random>
#include <string>
#include <iostream>

#include "wcore.h"
#include "arguments.h"

enum CellState: uint8_t
{
    EMPTY = 0,
    WALL_LEFT = 1,
    WALL_RIGHT = 2,
    WALL_UP = 4,
    WALL_DOWN = 8,
    VISITED = 16
};

struct MazeData
{
    MazeData(int width, int height):
    width_(width),
    height_(height),
    size_(width*height),
    n_cells_visited_(0)
    {
        cells_ = new uint8_t[size_];
        for(int ii=0; ii<size_; ++ii)
            cells_[ii] = 0x0F; // Walls everywhere, no cell visited
    }

    MazeData(const MazeData& other):
    width_(other.width_),
    height_(other.height_),
    size_(other.size_),
    n_cells_visited_(other.n_cells_visited_)
    {
        cells_ = new uint8_t[size_];
        for(int ii=0; ii<size_; ++ii)
            cells_[ii] = other.cells_[ii];
    }

    ~MazeData()
    {
        delete[] cells_;
    }

    inline int index(int xx, int zz) const
    {
        assert(xx*width_ + zz < size_);
        return xx*width_ + zz;
    }

    inline void set_visited(int xx, int zz)
    {
        cells_[index(xx,zz)] |= CellState::VISITED;
        ++n_cells_visited_;
    }
    inline bool is_visited(int xx, int zz) const
    {
        return (cells_[index(xx,zz)] & CellState::VISITED);
    }
    inline bool is_complete()
    {
        return n_cells_visited_ == size_;
    }
    // direction = 0 -> LEFT
    // direction = 1 -> RIGHT
    // direction = 2 -> UP
    // direction = 3 -> DOWN
    inline bool is_wall(uint8_t direction, int xx, int zz) const
    {
        assert(direction<4);
        return (cells_[index(xx,zz)] & (1<<direction));
    }

    inline void remove_wall(int xx, int zz, uint8_t wall_state)
    {
        int ind = index(xx,zz);
        cells_[ind] ^= (0 ^ cells_[ind]) & (wall_state);
    }

    // Remove wall between cell (x1,z1) and cell (x2,z2)
    void remove_wall(int x1, int z1, int x2, int z2)
    {
        // Cell 2 is left neighbor
        if(x2-x1 == -1)
        {
            remove_wall(x1, z1, CellState::WALL_LEFT);
            remove_wall(x2, z2, CellState::WALL_RIGHT);
        }
        // Cell 2 is right neighbor
        else if(x2-x1 == 1)
        {
            remove_wall(x1, z1, CellState::WALL_RIGHT);
            remove_wall(x2, z2, CellState::WALL_LEFT);
        }
        // Cell 2 is down neighbor
        else if(z2-z1 == -1)
        {
            remove_wall(x1, z1, CellState::WALL_DOWN);
            remove_wall(x2, z2, CellState::WALL_UP);
        }
        // Cell 2 is up neighbor
        else if(z2-z1 == 1)
        {
            remove_wall(x1, z1, CellState::WALL_UP);
            remove_wall(x2, z2, CellState::WALL_DOWN);
        }
    }

    void traverse_cells(std::function<void(int xx, int zz, uint8_t state)> func)
    {
        for(int ii=0; ii<width_; ++ii)
            for(int jj=0; jj<height_; ++jj)
                func(ii, jj, cells_[index(ii,jj)]);
    }

    void visit_neighbors(int xx, int zz, std::function<void(int xx, int zz)> func)
    {
        // Left cell
        if(xx-1 >= 0)
            func(xx-1, zz);
        // Right cell
        if(xx+1 < width_)
            func(xx+1, zz);
        // Up cell
        if(zz+1 < height_)
            func(xx, zz+1);
        // Down cell
        if(zz-1 >= 0)
            func(xx, zz-1);
    }

    std::vector<std::pair<int,int>> get_unvisited_neighbors(int xx, int zz)
    {
        std::vector<std::pair<int,int>> neighbors;
        visit_neighbors(xx,zz,[&](int nx, int nz)
        {
            if(!is_visited(nx,nz))
                neighbors.push_back(std::pair(nx,nz));
        });
        return neighbors;
    }

    friend std::ostream& operator <<(std::ostream& stream, const MazeData& maze);

    int width_;
    int height_;
    int size_;
    int n_cells_visited_;
    uint8_t* cells_;
};

typedef std::array<std::string,3> PatternT;
static PatternT get_pattern(uint8_t cell_state)
{
    PatternT pattern;
    pattern[0] = "      ";
    pattern[1] = "      ";
    pattern[2] = "      ";

    if(cell_state & CellState::WALL_LEFT)
    {
        pattern[0][0] = '|';
        pattern[1][0] = '|';
        pattern[2][0] = '|';
    }
    if(cell_state & CellState::WALL_RIGHT)
    {
        pattern[0][5] = '|';
        pattern[1][5] = '|';
        pattern[2][5] = '|';
    }
    if(cell_state & CellState::WALL_UP)
    {
        if(pattern[0][0] == ' ')
            pattern[0][0] = '-';

        pattern[0][1] = '-';
        pattern[0][2] = '-';
        pattern[0][3] = '-';
        pattern[0][4] = '-';

        if(pattern[0][5] == ' ')
            pattern[0][5] = '-';
    }
    if(cell_state & CellState::WALL_DOWN)
    {
        if(pattern[2][0] == ' ')
            pattern[2][0] = '-';

        pattern[2][1] = '-';
        pattern[2][2] = '-';
        pattern[2][3] = '-';
        pattern[2][4] = '-';

        if(pattern[2][5] == ' ')
            pattern[2][5] = '-';
    }

    return pattern;
}

std::ostream& operator <<(std::ostream& stream, const MazeData& maze)
{
    for(int zz=maze.height_-1; zz>=0; --zz)
    {
        // Save patterns for whole line
        std::vector<PatternT> line;
        for(int xx=0; xx<maze.width_; ++xx)
        {
            line.push_back(get_pattern(maze.cells_[maze.index(xx,zz)]));
        }
        for(int ii=0; ii<3; ++ii) // For each row in pattern
        {
            for(auto&& pattern: line) // For each pattern in line
            {
                stream << pattern[ii];
            }
            stream << std::endl;

        }
        //stream << std::endl;
    }
    return stream;
}

class MazeRecursiveBacktracker
{
public:
    MazeRecursiveBacktracker():
    cur_x_(0),
    cur_z_(0)
    {}

    void make_maze(MazeData& maze, int seed=0, int xx_start=0, int zz_start=0)
    {
        std::default_random_engine generator(seed);

        // * Make the initial cell the current cell and mark it as visited
        cur_x_ = xx_start;
        cur_z_ = zz_start;
        maze.set_visited(cur_x_, cur_z_);

        // Stack for backtracking
        std::stack<std::pair<int,int>> cell_stack;

        // * While there are unvisited cells
        while(!maze.is_complete())
        {
            // * If the current cell has any neighbours which have not been visited
            std::vector<std::pair<int,int>> unvisited = maze.get_unvisited_neighbors(cur_x_, cur_z_);
            if(unvisited.size()!=0)
            {
                // * Choose randomly one of the unvisited neighbours
                std::uniform_int_distribution<int> distribution(0,unvisited.size()-1);
                int rind = distribution(generator);
                std::pair<int,int> chosen = unvisited[rind];

                // * Push the current cell to the stack
                cell_stack.push(std::pair(cur_x_, cur_z_));
                // * Remove the wall between the current cell and the chosen cell
                maze.remove_wall(cur_x_, cur_z_, chosen.first, chosen.second);
                // * Make the chosen cell the current cell and mark it as visited
                cur_x_ = chosen.first;
                cur_z_ = chosen.second;
                maze.set_visited(cur_x_, cur_z_);
            }
            // * Else if stack is not empty
            else if(cell_stack.size()!=0)
            {
                // * Pop a cell from the stack
                std::pair<int,int> next = cell_stack.top();
                cell_stack.pop();
                // * Make it the current cell
                cur_x_ = next.first;
                cur_z_ = next.second;
            }
        }
    }

private:
    int cur_x_;
    int cur_z_;
};

int main(int argc, char const *argv[])
{
    // * Generate maze
    MazeData maze(8,8);
    MazeRecursiveBacktracker generator;
    generator.make_maze(maze,4);
    std::cout << maze << std::endl << std::endl;

    // * Start engine and load default map
    wcore::Engine engine;
    engine.Init(argc, argv, sandbox::parse_program_arguments);
    wcore::GlobalsSet(H_("START_LEVEL"), "maze");

    // * Load level and first chunk, but don't send geometry yet
    engine.LoadLevel();
    engine.LoadChunk(0, 0, false);

    // * Add wall models to scene

    /* TODO
        [x] Decouple Mesh from Model
            [x] Meshes are cashed, Model has weak ptr to Mesh
            [x] In level XML, ability to declare Mesh instances
                -> Instances are global in level
                -> Model node can refer to a Mesh Instance instead of
                   declaring the Mesh
        [x] Same for Material
            [x] Materials are cashed...
        -> Make API handle scene insertion of models using declared instances
    */


    maze.traverse_cells([&](int xx, int zz, uint8_t state)
    {
        wcore::math::vec3 cell_center(xx+0.5f, 0.f, zz+0.5f);
        if(state & CellState::WALL_LEFT)
        {

        }
        if(state & CellState::WALL_RIGHT)
        {

        }
        if(state & CellState::WALL_UP)
        {

        }
        if(state & CellState::WALL_DOWN)
        {

        }
    });

    // * Send chunk geometry to GL and run
    engine.SendChunk(0, 0);
    return engine.Run();
}
