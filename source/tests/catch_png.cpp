#include <catch2/catch.hpp>
#include <iostream>
#include <stdexcept>
#include "png_loader.h"
#include "pixel_buffer.h"

static PngLoader& loader = PngLoader::Instance();

TEST_CASE("Trying to load a png that isn't there.", "[png]")
{
    REQUIRE_THROWS_AS(loader.load_png("../res/test/notafile.png"), std::runtime_error);
}

TEST_CASE("Loading file that isn't a png image.", "[png]")
{
    REQUIRE_THROWS_AS(loader.load_png("../res/test/notapng.png"), std::runtime_error);
}

TEST_CASE("Loading a valid png file.", "[png]")
{
    PixelBuffer* px_buf = loader.load_png("../res/test/arthur.png");
    //std::cout << *px_buf << std::endl;
    REQUIRE(px_buf->get_width() == 840);
    REQUIRE(px_buf->get_height() == 94);
    REQUIRE(px_buf->get_bitdepth() == 8);
    REQUIRE(px_buf->get_channels() == 3);

    delete px_buf;
}

TEST_CASE("Loading a 9x9 png with known data.", "[png]")
{
    PixelBuffer* px_buf = loader.load_png("../res/test/9x9.png");

    REQUIRE(px_buf->get_width() == 3);
    REQUIRE(px_buf->get_height() == 3);

    // In png:
    // First row is  black, red, green
    // Second row is blue, white, gray(100)
    // Third row is  (1,2,3), (4,5,6), (7,8,9)
    // So we expect these in the REVERSE row order.
    unsigned char data[]{1, 2, 3, 4, 5, 6, 7, 8, 9,
                         0, 0, 255, 255, 255, 255, 100, 100, 100,
                         0, 0, 0, 255, 0, 0, 0, 255, 0};

    for (int ii = 0; ii < 27; ++ii)
        REQUIRE((*px_buf)[ii] == data[ii]);

    delete px_buf;
}

TEST_CASE("Loading a bad png file.", "[png]")
{
    REQUIRE_THROWS_AS(loader.load_png("../res/test/badpng.png"), std::runtime_error);
}
