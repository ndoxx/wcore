#include <thread>

#include "chunk_manager.h"
#include "config.h"
#include "scene.h"
#include "scene_loader.h"
#include "camera.h"
#include "input_handler.h"
#include "debug_info.h"
#include "game_clock.h"

namespace wcore
{

using namespace math;

ChunkManager::ChunkManager(SceneLoader& loader):
loader_(loader),
active_(true),
view_radius_(2)
#ifdef __OPT_CHUNK_LOAD_DIRECTION_HINT__
,last_quadrant_(4)
#endif
#ifdef __OPT_CHUNK_LOAD_FULL_DISK__
,last_chunk_(0)
#endif
{
    // Get configuration
    uint32_t vr=2;
    if(CONFIG.get(H_("root.render.chunk.load_distance"), vr))
        view_radius_ = uint8_t(fmin(5,vr));

    // Register debug info fields
    DINFO.register_text_slot(H_("sdiNGeom"), vec3(0.5,0.0,1.0));
}

ChunkManager::~ChunkManager()
{

}

void ChunkManager::init()
{
    // * Load start chunk and some neighbors
    // compute current chunk coordinates
    const vec3& cam_pos = SCENE.get_camera()->get_position();
    uint32_t ck_size_m = loader_.get_chunk_size_meters();
    i32vec2 chunk_coords((uint32_t)floor(cam_pos.x()/ck_size_m),
                         (uint32_t)floor(cam_pos.z()/ck_size_m));
    loader_.load_chunk(chunk_coords);

#ifdef __OPT_CHUNK_LOAD_DIRECTION_HINT__
    // load neighbors
    for(int ii=-view_radius_; ii<=view_radius_; ++ii)
    {
        for(int jj=-view_radius_; jj<=view_radius_; ++jj)
        {
            if(int(chunk_coords.x())+ii<0 ||
               int(chunk_coords.y())+jj<0)
                continue;
            i32vec2 neighbor(chunk_coords.x()+ii,
                             chunk_coords.y()+jj);
            loader_.load_chunk(neighbor);
        }
    }
#endif
}


void ChunkManager::setup_user_inputs(InputHandler& handler)
{
    handler.register_action(H_("k_tg_chunk_mgr"), [&]()
    {
        toggle();
    });
}

#ifdef __OPT_CHUNK_LOAD_DIRECTION_HINT__
void ChunkManager::update(const GameClock& clock)
{
    if(!active_) return;

    //float dt = clock.get_scaled_frame_duration();

    // * Check if camera transitions through current chunk quadrants
    // Compute centered normalized local coordinates
    const vec3& cam_pos = SCENE.get_camera()->get_position();
    uint32_t ck_size_m = loader_.get_chunk_size_meters();
    vec2 lcp(fmod(cam_pos.x(), ck_size_m)/ck_size_m - 0.5f,
             fmod(cam_pos.z(), ck_size_m)/ck_size_m - 0.5f);
    // Compute current quadrant
    uint8_t cur_quadrant = quadrant(lcp);

    // Display local coords on slot 6
    if(DINFO.active())
    {
        std::stringstream ss;
        ss << "Local position: " << lcp;
        DINFO.display(6, ss.str(), vec3(0.2,0.9,1.0));
    }

    // * If so, check for loadable neighbor chunks using quadrant as a
    // direction hint and load if not already loaded.
    if(cur_quadrant!=last_quadrant_)
    {
        // compute current chunk coordinates
        i32vec2 chunk_coords((uint32_t)floor(cam_pos.x()/ck_size_m),
                             (uint32_t)floor(cam_pos.z()/ck_size_m));

        // compute lattice generators
        int hint_x = 2*(cur_quadrant&0x1)-1;
        int hint_z = 2*((cur_quadrant>>1)&0x1)-1;

        /*DLOGI(std::to_string(cur_quadrant) + " " +
              std::to_string(hint_x) + " " +
              std::to_string(hint_z));*/

        // just span these generators to produce chunk coords to load
        //BANG();
        for(int ii=0; ii<=view_radius_; ++ii)
        {
            for(int jj=0; jj<=view_radius_; ++jj)
            {
                if(int(chunk_coords.x())+ii*hint_x<0 ||
                   int(chunk_coords.y())+jj*hint_z<0)
                    continue;
                i32vec2 candidate(chunk_coords.x()+ii*hint_x,
                                  chunk_coords.y()+jj*hint_z);
                uint32_t c_index = std::hash<i32vec2>{}(candidate);
                if(SCENE.has_chunk(c_index))
                    continue;

                /*std::stringstream ss;
                ss << candidate;
                DLOGI(ss.str());*/
                loader_.load_chunk(candidate);
                /*std::thread th_load_ck([&]()
                {
                    loader_.load_chunk(candidate);
                });
                th_load_ck.detach();*/
            }
        }

        // Sort chunks
        SCENE.sort_chunks();

        last_quadrant_ = cur_quadrant;
    }
}
#endif
#ifdef __OPT_CHUNK_LOAD_FULL_DISK__
void ChunkManager::update(const GameClock& clock)
{
    if(!active_) return;

    //float dt = clock.get_scaled_frame_duration();

    // * Check if camera enters new chunk
    // get current chunk coordinates
    const i32vec2& chunk_coords = SCENE.get_current_chunk_coords();
    uint32_t current_chunk = SCENE.get_current_chunk_index();

    // * If so, check for loadable neighbors in full visibility disk
    if(current_chunk!=last_chunk_)
    {
        for(int ii=-view_radius_; ii<=view_radius_; ++ii)
        {
            for(int jj=-view_radius_; jj<=view_radius_; ++jj)
            {
                // Neighbor check & Euclidean distance check
                if((ii==0 && jj==0) || (ii*ii+jj*jj)>view_radius_*view_radius_)
                    continue;
                // Positive coordinates check
                int new_x = int(chunk_coords.x())+ii;
                int new_y = int(chunk_coords.y())+jj;
                if(new_x<0 || new_y<0)
                    continue;
                // Check if chunk already loaded
                i32vec2 candidate(new_x, new_y);
                uint32_t c_index = std::hash<i32vec2>{}(candidate);
                if(SCENE.has_chunk(c_index))
                    continue;
                // Load chunk
                loader_.load_chunk(candidate);
            }
        }

        // * And unload chunks that escaped the visibility disk
        std::vector<uint32_t> unload_candidates;
        SCENE.get_far_chunks(view_radius_+1, unload_candidates);
        for(uint32_t index: unload_candidates)
            SCENE.remove_chunk(index);

        // Sort chunks
        SCENE.sort_chunks();

        last_chunk_ = current_chunk;
    }

#ifdef __PROFILING_CHUNKS__
    // Display debug info
    if(DINFO.active())
    {
        std::stringstream ss;
        ss << "Vertex count: " << SCENE.get_vertex_count()
           << " Triangles count: " << SCENE.get_triangles_count();
        DINFO.display(H_("sdiNGeom"), ss.str());
    }
#endif
}
#endif

}
