#include "fnont.h"

FnError create(uint32_t width, uint32_t height, uint32_t depth,
               FnTextureHandle *handle)
{
    return 0;
}

FnError expand(FnTextureHandle *handle, uint32_t index, uint32_t width,
               uint32_t height)
{
    return 0;
}

FnError upload(FnTextureHandle *handle, uint32_t index, uint32_t x, uint32_t y,
               uint32_t width, uint32_t height, const void *data)
{
    return 0;
}

FnError destroy(FnTextureHandle *handle)
{
    return 0;
}


int main() {
    struct FnBackend backend = { create, expand, upload, destroy };
    unsigned err = fn_initialize(backend);
    if (err) {
        printf("Failed to initialize backend\n");
        return 1;
    }
    
    struct FnAtlas atlas;
    err = fn_atlas_new(512, 512, 1, &atlas);
    if (err) {
        printf("Failed to create atlas\n");
        return 1;
    }
    
    printf("Loading font\n");
    
    struct FnFont font;
    err = fn_font_from_file("demo/VeraMono.ttf", 32, &font, &atlas);
    if (err) {
        printf("Failed to load from file\n");
        return 1;
    }
    
    printf("Loading glyph\n");
    
    struct FnGlyph glyph_A;
    err = fn_font_get_glyph(&font, 'A', &glyph_A);
    err = fn_font_get_glyph(&font, 'A', &glyph_A);
    if (err) {
        printf("Failed to load glyph\n");
        return 1;
    }
    
    printf("Glyph 'A': .handle = %d, .x = %d, .y = %d, .w = %d, .h = %d\n",
            glyph_A.handle, glyph_A.x, glyph_A.y ,glyph_A.w, glyph_A.h);
    
    return 0;
}
