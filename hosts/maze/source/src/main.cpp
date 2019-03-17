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

#include "model.h"
#include "lights.h"

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

    inline void remove_walls(int xx, int zz)
    {
        cells_[index(xx,zz)] &= 0x10;
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

    void make_hole(int xmin, int xmax, int zmin, int zmax)
    {
        assert(xmin<=xmax);
        assert(zmin<=zmax);
        for(int xx=xmin; xx<=xmax; ++xx)
            for(int zz=zmin; zz<=zmax; ++zz)
                remove_walls(xx,zz);
    }

    void traverse_cells(std::function<void(int xx, int zz, uint8_t state)> func)
    {
        for(int ii=0; ii<width_; ++ii)
            for(int jj=0; jj<height_; ++jj)
                func(ii, jj, cells_[index(jj,ii)]);
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

    // Remove redundant walls
    void simplify()
    {
        for(int xx=0; xx<width_; ++xx)
        {
            for(int zz=0; zz<height_; ++zz)
            {
                // Remove left neighbor's right wall if current cell has left wall
                if(xx-1 >= 0 && is_wall(0,xx,zz))
                {
                    if(is_wall(1,xx-1,zz))
                        remove_wall(xx-1, zz, CellState::WALL_RIGHT);
                }
                // Remove down neighbor's up wall if current cell has down wall
                if(zz-1 >= 0 && is_wall(3,xx,zz))
                {
                    if(is_wall(2,xx,zz-1))
                        remove_wall(xx, zz-1, CellState::WALL_UP);
                }
            }
        }
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

using namespace wcore;

hash_t make_reference(const char* base_name, int xx, int yy)
{
    return H_((base_name + std::string("_") + std::to_string(xx) + std::string("_") + std::to_string(yy)).c_str());
}

int main(int argc, char const *argv[])
{
    // * Generate maze
    MazeData maze(15,15);
    MazeRecursiveBacktracker generator;
    generator.make_maze(maze,82);
    // Make room for tree in the middle
    maze.make_hole(6,8,6,8);
    std::cout << maze << std::endl << std::endl;

    // * Start engine and load default map
    wcore::Engine engine;
    engine.Init(argc, argv, sandbox::parse_program_arguments);

    // * Load level and first chunk, but don't send geometry yet
    engine.scene->LoadLevel("maze");
    uint32_t chunk00 = engine.scene->LoadChunk(0, 0, false);

    std::default_random_engine rng;
    std::uniform_real_distribution<float> distribution(0.0,1.0);

    // * Add wall models to scene
    // Traverse maze cells to place lights
    maze.traverse_cells([&](int xx, int zz, uint8_t state)
    {
        uint8_t nwalls = 0;
        if(state & CellState::WALL_LEFT)
        {
            ++nwalls;
        }
        if(state & CellState::WALL_RIGHT)
        {
            ++nwalls;
        }
        if(state & CellState::WALL_UP)
        {
            ++nwalls;
        }
        if(state & CellState::WALL_DOWN)
        {
            ++nwalls;
        }

        if(nwalls>=3 && distribution(rng) > 0.5f)
        {
            wcore::math::vec3 cell_center(2.0f*xx+2.0f, 0.f, 2.0f*zz+1.0f);
            hash_t href = make_reference("point_light", xx, zz);
            engine.scene->LoadPointLight(chunk00, href);
            engine.scene->VisitLightRef(href, [&](Light& light)
            {
                light.set_position(cell_center+math::vec3(0.f,3.0f,1.f));
                light.set_color(math::vec3(0.25f+xx/20.f,distribution(rng),0.25f+zz/20.f));
                light.set_radius(5.f);
                light.set_brightness(10.f);
            });

            href = make_reference("teapot", xx, zz);
            engine.scene->LoadModel("teapot01"_h, chunk00, href);
            engine.scene->VisitModelRef(href, [&](Model& model)
            {
                model.set_scale(0.05f);
                model.set_position(cell_center+math::vec3(0,0.4f,1.0f));
                model.set_orientation(math::quat(0,90,0));
                model.update_bounding_boxes();
            });
        }
    });
    // Simplify geometry and place walls
    maze.simplify();
    maze.traverse_cells([&](int xx, int zz, uint8_t state)
    {
        uint8_t nwalls = 0;
        wcore::math::vec3 cell_center(2.0f*xx+2.0f, 0.f, 2.0f*zz+1.0f);
        if(state & CellState::WALL_LEFT)
        {
            hash_t href = make_reference("wall_left", xx, zz);
            engine.scene->LoadModel("brickWall01"_h, chunk00, href);
            engine.scene->VisitModelRef(href, [&](Model& model)
            {
                model.set_position(cell_center);
                model.set_orientation(math::quat(0,90,0));
                model.update_bounding_boxes();
            });
            ++nwalls;
        }
        if(state & CellState::WALL_RIGHT)
        {
            hash_t href = make_reference("wall_right", xx, zz);
            engine.scene->LoadModel("brickWall01"_h, chunk00, href);
            engine.scene->VisitModelRef(href, [&](Model& model)
            {
                model.set_position(cell_center+math::vec3(0.f,0.f,2.f));
                model.set_orientation(math::quat(0,90,0));
                model.update_bounding_boxes();
            });
            ++nwalls;
        }
        if(state & CellState::WALL_UP)
        {
            hash_t href = make_reference("wall_up", xx, zz);
            engine.scene->LoadModel("brickWall01"_h, chunk00, href);
            engine.scene->VisitModelRef(href, [&](Model& model)
            {
                model.set_position(cell_center+math::vec3(1.f,0.f,1.f));
                model.update_bounding_boxes();
            });
            ++nwalls;
        }
        if(state & CellState::WALL_DOWN)
        {
            hash_t href = make_reference("wall_down", xx, zz);
            engine.scene->LoadModel("brickWall01"_h, chunk00, href);
            engine.scene->VisitModelRef(href, [&](Model& model)
            {
                model.set_position(cell_center+math::vec3(-1.f-0.01f,-0.01f,1.f));
                model.update_bounding_boxes();
            });
            ++nwalls;
        }
    });

    // * Send chunk geometry to GL and run
    engine.scene->SendChunk(0, 0);
    return engine.Run();
}
