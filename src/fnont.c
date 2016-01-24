#include "fnont.h"

#include <stdbool.h>
#include <limits.h>

#include FT_GLYPH_H

static FT_Library g_library;
static struct FnBackend g_backend;
static bool g_init = false;

static void map_new(struct FnGlyphMap *map, int size);
static void map_free(struct FnGlyphMap *map);
static bool map_get(struct FnGlyphMap *map, uint32_t charpoint,
                    struct FnGlyph *glyph);
static void map_put(struct FnGlyphMap *map, uint32_t charpoint,
                    struct FnGlyph glyph);

static FnError atlas_pack(struct FnAtlas *atlas, uint32_t width,
                          uint32_t height, struct FnGlyph *glyph,
                          void *data);
static void atlas_free(struct FnAtlas *atlas);

static FnError bin_init(struct FnAtlasBin *bin, uint32_t width,
                        uint32_t height);
static bool bin_fits(struct FnAtlasBin *bin, uint32_t idx, uint32_t width,
                     uint32_t height, uint32_t *y);
static void bin_skyline(struct FnAtlasBin *bin, uint32_t idx,
                        struct FnRect rect);
static struct FnRect bin_position(struct FnAtlasBin *bin, int width, int height,
                                  int *best_height, int *best_width,
                                  int *best_idx);
static bool bin_insert(struct FnAtlasBin *bin, int width, int height,
                       struct FnRect *rect);
static void bin_merge(struct FnAtlasBin *bin);
static void bin_free(struct FnAtlasBin *bin);

static FnError font_load_glyph(struct FnFont *font, uint32_t charpoint,
                               struct FnGlyph *glyph);
static FnError font_load_glyphs(struct FnFont *font, uint32_t *charpoints,
                                size_t len);

FnError fn_initialize(struct FnBackend backend)
{
    if (g_init)
        return 1;

    unsigned err = FT_Init_FreeType(&g_library);
    if (err) {
        return err;
    }

    g_backend = backend;
    g_init = true;
    return 0;
}

FnError fn_atlas_new(uint32_t width, uint32_t height, uint32_t depth,
                     struct FnAtlas *atlas)
{
    unsigned err = g_backend.create(width, height, depth, atlas->handles);
    if (err)
        return err;

    atlas->depth = depth;

    atlas->bins = calloc(depth, sizeof(struct FnAtlasBin));
    for (int i = 0; i < depth; ++i) {
        bin_init(&atlas->bins[i], width, height);
    }

    return 0;
}

FnError fn_font_from_file(const char *path, uint32_t size, struct FnFont *font,
                          struct FnAtlas *atlas)
{
    unsigned err = FT_New_Face(g_library, path, 0, &font->font_face);
    if (err) {
        return err;
    }

    err = FT_Set_Char_Size(font->font_face, size << 6, size << 6, 96, 96);
    if (err)
        return err;

    font->atlas = atlas;
    font->font_size = size;

    map_new(&font->map, 128);

    return 0;
}

FnError fn_font_get_glyph(struct FnFont *font, uint32_t charpoint,
                          struct FnGlyph *glyph)
{
    if (!map_get(&font->map, charpoint, glyph)) {
        unsigned err = font_load_glyph(font, charpoint, glyph);

        if (err)
            return err;
    }

    return 0;
}

void fn_font_free(struct FnFont *font)
{
    FT_Done_Face(font->font_face);

    map_free(&font->map);
}


static inline uint32_t phi_mix(uint32_t x)
{
    const uint32_t h = x * 0x9E3779B9;
    return h ^ (h >> 16);
}

static void map_new(struct FnGlyphMap *map, int size)
{
    map->keys = calloc(size, sizeof(FnCharpoint));
    map->glyphs = calloc(size, sizeof(struct FnGlyph));
    map->size = 0;
    map->capacity = size;
}

static void map_free(struct FnGlyphMap *map)
{
    free(map->keys);
    free(map->glyphs);
}

static uint32_t map_slot(struct FnGlyphMap *map, uint32_t charpoint)
{
    for (uint32_t idx = phi_mix(charpoint);; ++idx) {
        idx = idx & (map->capacity - 1);

        uint32_t key = map->keys[idx];
        if (key == 0 || key == charpoint) {
            return idx;
        }
    }
}

static bool map_get(struct FnGlyphMap *map, uint32_t charpoint,
                    struct FnGlyph *glyph)
{
    uint32_t slot = map_slot(map, charpoint);
    uint32_t key = map->keys[slot];

    if (key != 0) {
        *glyph = map->glyphs[slot];

        return true;
    } else {
        return false;
    }
}

static void map_put(struct FnGlyphMap *map, uint32_t charpoint,
                    struct FnGlyph glyph)
{
    uint32_t slot = map_slot(map, charpoint);
    uint32_t key = map->keys[slot];

    if (key != 0) {
        map->glyphs[key] = glyph;
    } else {
        if (map->size + 1 > map->capacity) {
            map->glyphs = realloc(map->glyphs, map->capacity << 1);
            map->keys = realloc(map->keys, map->capacity << 1);

            map->capacity <<= 1;
        }

        map->size++;
        map->keys[slot] = charpoint;
        map->glyphs[slot] = glyph;

    }
}

static FnError font_load_glyph(struct FnFont *font, uint32_t charpoint,
                               struct FnGlyph *glyph)
{
    // TODO(fkaa): rasterize glyph and pack into bin(s)

    FT_Glyph ftglyph;
    FT_UInt idx = FT_Get_Char_Index(font->font_face, charpoint);
    unsigned err = FT_Load_Glyph(font->font_face, idx, FT_LOAD_DEFAULT);
    if (err) {
        return err;
    }

    err = FT_Get_Glyph(font->font_face->glyph, &ftglyph);
    if (err) {
        return err;
    }

    err = FT_Glyph_To_Bitmap(&ftglyph, FT_RENDER_MODE_NORMAL, 0, 1);
    if (err) {
        return err;
    }

    FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph) ftglyph;
    FT_Bitmap bitmap = bitmap_glyph->bitmap;:

    struct FnGlyph out_glyph = {
        0,
        bitmap_glyph->left, bitmap_glyph->top,
        bitmap.width, bitmap.rows,
        (int) (ftglyph->advance.x >> 16)
    };

    err = atlas_pack(font->atlas, bitmap.width, bitmap.rows, &out_glyph,
                     bitmap.buffer);
    if (err) {
        FT_Done_Glyph(ftglyph);
        return err;
    }

    FT_Done_Glyph(ftglyph);

    if (glyph)
        *glyph = out_glyph;

    map_put(&font->map, charpoint, out_glyph);

    return 0;
}

static FnError atlas_pack(struct FnAtlas *atlas, uint32_t width,
                          uint32_t height, struct FnGlyph *glyph,
                          void *data)
{
    struct FnRect rect;

    for (int i = 0; i < atlas->depth; ++i) {
        struct FnAtlasBin *bin = &atlas->bins[i];

        if (bin_insert(bin, width, height, &rect)) {
            glyph->handle = atlas->handles + i;
            glyph->x = rect.x;
            glyph->y = rect.y;
            glyph->w = rect.w;
            glyph->h = rect.h;

            g_backend.upload(atlas->handles, i, rect.x, rect.y, rect.w, rect.h,
                             data);
            return 0;
        }

    }

    return 1;
}

static void atlas_free(struct FnAtlas *atlas)
{
    for (int i = 0; i < atlas->depth; ++i) {
        bin_free(&atlas->bins[i]);
    }

    // TODO(fkaa): free texture handles
}

static FnError bin_init(struct FnAtlasBin *bin, uint32_t width, uint32_t height)
{
    bin->skylines = calloc(64, sizeof(struct FnAtlasBinNode));
    bin->capacity = 64;
    bin->size = 1;
    bin->width = width;
    bin->height = height;

    bin->skylines[0] = (struct FnAtlasBinNode) {
            .x = 0,
            .y = 0,
            .width = width
    };

    return 0;
}

static bool bin_fits(struct FnAtlasBin *bin, uint32_t idx, uint32_t width,
                     uint32_t height, uint32_t *y)
{
    uint32_t x = bin->skylines[idx].x;
    if (x + width > bin->width)
        return false;

    int width_left = width;
    int out_y = bin->skylines[idx].y;

    while (width_left > 0) {
        out_y = out_y  > bin->skylines[idx].y ? out_y : bin->skylines[idx].y;

        if (out_y + height > bin->height)
            return false;

        width_left -= bin->skylines[idx].width;
        ++idx;
    }

    *y = out_y;

    return true;
}

static void bin_skyline(struct FnAtlasBin *bin, uint32_t idx,
                        struct FnRect rect)
{
    // if (bin->size + 1 > bin->capacity) {
    //     bin->capacity *= 2;
    //     bin->skylines = realloc(bin->skylines, sizeof(struct FnAtlasBinNode) * bin->capacity);
    // }

    struct FnAtlasBinNode new_node = {
        .x = rect.x,
        .y = rect.y + rect.h,
        .width = rect.w
    };

    bin->skylines[idx] = new_node;

    for (int i = idx + 1; i < bin->size; ++i) {
        if (bin->skylines[i].x < bin->skylines[i - 1].x + bin->skylines[i - 1].width) {
            int shrink = bin->skylines[i - 1].x + bin->skylines[i - 1].width - bin->skylines[i].x;

            bin->skylines[i].x += shrink;
            bin->skylines[i].width -= shrink;

            if (bin->skylines[i].width <= 0) {
                bin->size -= i;
                i--;
            } else {
                break;
            }
        } else {
            break;
        }
    }

    bin_merge(bin);
}

static struct FnRect bin_position(struct FnAtlasBin *bin, int width, int height,
                                  int *best_width, int *best_height,
                                  int *best_idx)
{
    int new_width = INT_MAX;
    int new_height = INT_MAX;
    int new_idx = -1;

    struct FnRect new_node = {0};

    for (int i = 0; i < bin->size; ++i) {
        uint32_t y = 0;
        if (bin_fits(bin, i, width, height, &y)) {
            if (y + height < new_height || (y + height == new_height && bin->skylines[i].width < new_width)) {
                new_width = bin->skylines[i].width;
                new_height = y + height;
                new_idx = i;

                new_node.x = bin->skylines[i].x;
                new_node.y = y;
                new_node.w = width;
                new_node.h = height;
            }
        }
    }

    *best_width = new_width;
    *best_height = new_height;
    *best_idx = new_idx;

    return new_node;
}

static bool bin_insert(struct FnAtlasBin *bin, int width, int height,
                       struct FnRect *rect)
{
    int best_width;
    int best_height;
    int best_idx;
    struct FnRect new_node = bin_position(bin, width, height, &best_width,
                                          &best_height, &best_idx);

    if (best_idx == -1)
        return false;

    bin_skyline(bin, best_idx, new_node);
    *rect = new_node;

    return true;
}

static void bin_merge(struct FnAtlasBin *bin)
{
    for (int i = 0; i < bin->size - 1; ++i) {
        if (bin->skylines[i].y == bin->skylines[i + 1].y) {
            bin->skylines[i].width += bin->skylines[i + 1].width;
            bin->size -= i + 1;

            i--;
        }
    }
}

static void bin_free(struct FnAtlasBin *bin)
{
    free(bin->skylines);
}

// vim: ts=4 sw=4 et
