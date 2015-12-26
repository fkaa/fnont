#include "fnont/fnont.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdbool.h>

struct FnAtlas {
    FnTextureHandle *handles;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
};

struct FnFont {
    FT_Face face;
    uint32_t size;

    struct FnAtlas atlas;
};

struct FnGlyph;

static FT_Library gLibrary;
static struct FnBackend gBackend;

static bool gInit = false;

/* TODO(fkaa): use error enums instead */
FnError fn_initialize(struct FnBackend backend)
{
    if (gInit) return 1;

    unsigned err = FT_Init_FreeType(&gLibrary);
    if (err) {
        return err;
    }

    gBackend = backend;
    gInit = true;
    return 0;
}

FnError fn_atlas_new(uint32_t width, uint32_t height, int32_t depth,
                     struct FnAtlas *atlas)
{
    gBackend.create(width, height, depth, atlas->handles);
    return 0;
}

FnError fn_font_from_file(const char *path, uint32_t size, struct FnFont *font)
{
    unsigned err = FT_New_Face(gLibrary, path, 0, &font->face);
    if (err) {
        return err;
    }

    err = FT_Set_Char_Size(font->face, size << 6, size << 6, 96, 96);
    if (err) {
        return err;
    }

//    err = fn_atlas_new(512, 512, 1);
    return 0;
}
