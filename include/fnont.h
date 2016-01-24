#ifndef FNONT_H
#define FNONT_H

#include <stddef.h>
#include <stdint.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef FNONT_SHARED
#    ifdef _WIN32
#        define FNONT_LIB_IMPORT __declspec(dllimport)
#        define FNONT_LIB_EXPORT __declspec(dllexport)
#    else
#        define FNONT_LIB_IMPORT __attribute__((visibility("default")))
#        define FNONT_LIB_EXPORT __attribute__((visibility("default")))
#    endif
#    ifdef FNONT_INTERNAL
#        define FNONT_API FNONT_LIB_EXPORT
#    else
#        define FNONT_API FNONT_LIB_IMPORT
#    endif
#else
#    define FNONT_API
#endif

typedef uint32_t FnCharpoint;
typedef uint32_t FnTextureHandle;
typedef uint32_t FnError;

typedef FnError (*FnCreateFunc)(uint32_t width, uint32_t height,
                                uint32_t depth, FnTextureHandle *handle);

typedef FnError (*FnExpandFunc)(FnTextureHandle *handle, uint32_t index,
                                uint32_t width, uint32_t height);

typedef FnError (*FnUploadFunc)(FnTextureHandle *handle, uint32_t index,
                                uint32_t x, uint32_t y, uint32_t width,
                                uint32_t height, const void *data);

typedef FnError (*FnDeleteFunc)(FnTextureHandle *handle);

struct FnBackend {
    FnCreateFunc create;
    FnExpandFunc expand;
    FnUploadFunc upload;
    FnDeleteFunc destroy;
};

struct FnAtlasBinNode {
    uint32_t x, y;
    uint32_t width;
};

struct FnAtlasBin {
    struct FnAtlasBinNode *skylines;
    uint32_t capacity;
    uint32_t size;
    uint32_t width;
    uint32_t height;
};

struct FnAtlas {
    struct FnAtlasBin *bins;
    FnTextureHandle *handles;
    uint32_t depth;
};

struct FnGlyphMap {
    FnCharpoint *keys;
    struct FnGlyph *glyphs;
    int size, capacity;
};

struct FnFont {
    struct FnGlyphMap map;
    struct FnAtlas *atlas;

    FT_Face font_face;
    uint32_t font_size;
};

struct FnGlyph {
    FnTextureHandle handle;
    int x, y;
    int w, h;
    int advance;
};

struct FnRect {
    int x, y;
    int w, h;
};

FNONT_API extern FnError fn_initialize(struct FnBackend backend);

FNONT_API extern FnError fn_atlas_new(uint32_t width, uint32_t height,
                                      uint32_t depth, struct FnAtlas *atlas);


FNONT_API extern FnError fn_font_from_file(const char *path, uint32_t size,
                                           struct FnFont *font,
                                           struct FnAtlas *atlas);
FNONT_API extern FnError fn_font_get_glyph(struct FnFont *font,
                                           uint32_t charpoint,
                                           struct FnGlyph *glyph);

FNONT_API extern void fn_font_free(struct FnFont *font);


#endif // vim: ts=4 sw=4 et
