#ifndef _ASSETS_H_
#define _ASSETS_H_

typedef struct
{
    uint32_t* assetPtr;
    uint32_t idx;

    uint8_t* compressed;
    uint8_t* decompressed;
    uint8_t* frame;
    uint32_t allocedSize;

    uint16_t width;
    uint16_t height;
    uint16_t x;
    uint16_t y;

    uint16_t nFrames;
    uint16_t cFrame;
    uint16_t duration;
    os_timer_t timer;
} gifHandle;

uint32_t* getAsset(const char* name, uint32_t* retLen);
void drawBitmapFromAsset(const char* name, int16_t x, int16_t y, bool flipLR);
void drawGifFromAsset(const char* name, int16_t x, int16_t y, gifHandle* handle);
void freeGifMemory(gifHandle* handle);

#endif