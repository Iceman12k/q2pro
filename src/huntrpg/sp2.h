
#define SP2_IDENT       MakeLittleLong('I','D','S','2')
#define SP2_VERSION     2

#define SP2_MAX_FRAMES      32
#define SP2_MAX_FRAMENAME   64

typedef struct {
    uint32_t    width, height;
    uint32_t    origin_x, origin_y;         // raster coordinates inside pic
    char        name[SP2_MAX_FRAMENAME];    // name of pcx file
} dsp2frame_t;

typedef struct {
    uint32_t    ident;
    uint32_t    version;
    uint32_t    numframes;
} dsp2header_t;

