#ifndef FNONT_H
#define FNONT_H

#include <stddef.h>
#include <stdint.h>

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

typedef uint32_t FnTextureHandle;
typedef uint32_t FnError;

struct FnFont;
struct FnGlyph;

typedef FnError (*FnCreateFunc)(uint32_t width, uint32_t height,
                                uint32_t depth, FnTextureHandle *handle);

typedef FnError (*FnExpandFunc)(FnTextureHandle *handle, uint32_t index,
                                uint32_t width, uint32_t height);

typedef FnError (*FnUploadFunc)(FnTextureHandle *handle, uint32_t index,
                                uint32_t x, uint32_t y, uint32_t width,
                                uint32_t height, unsigned char *data);

typedef FnError (*FnDeleteFunc)(FnTextureHandle *handle);

struct FnBackend {
    FnCreateFunc create;
    FnExpandFunc expand;
    FnUploadFunc upload;
    FnDeleteFunc destroy;
};

FNONT_API extern FnError fn_initialize();
FNONT_API extern FnError fn_font_from_file(const char *path, uint32_t size,
                                           struct FnFont *font);

#endif /* vim: ts=4 et st=4 sts=4 : */

