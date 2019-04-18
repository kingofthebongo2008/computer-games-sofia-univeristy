#pragma once

namespace sample
{
    //Mipmap faces
    //face = 0 +X
    //face = 1 -X
    //face = 2 +Y
    //face = 3 -Y
    //face = 5 +Z
    //face = 6 -Z

    // A decoded sample from a sampling render pass.
    struct DecodedSample
    {
        float u;
        float v;
        short mip;
        short face;
    };
}
