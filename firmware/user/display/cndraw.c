/*
 * cndraw.c
 *
 *  Created on: Sep 26, 2020
 *      Author: adam, CNLohr
 */

#include <osapi.h>
#include "oled.h"
#include "cndraw.h"

/**
 * Fill a rectangular display area with a single color
 *
 * @param x1 The X pixel to start at
 * @param y1 The Y pixel to start at
 * @param x2 The X pixel to end at
 * @param y2 The Y pixel to end at
 * @param c  The color to fill
 */
void ICACHE_FLASH_ATTR fillDisplayArea(int16_t x1, int16_t y1, int16_t x2, int16_t y2, color c)
{
    int16_t x, y;
    for (x = x1; x <= x2; x++)
    {
        for (y = y1; y <= y2; y++)
        {
            drawPixel(x, y, c);
        }
    }
}

/**
 * 'Shade' an area by drawing black pixels over it in a ordered-dithering way
 *
 * @param x1 The X pixel to start at
 * @param y1 The Y pixel to start at
 * @param x2 The X pixel to end at
 * @param y2 The Y pixel to end at
 * @param shadeLevel The level of shading, Higher means more shaded. Must be 0 to 4
 */
void ICACHE_FLASH_ATTR shadeDisplayArea(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t shadeLevel)
{
    for(int16_t dy = y1; dy < y2; dy++)
    {
        for(int16_t dx = x1; dx < x2; dx++)
        {
            switch(shadeLevel)
            {
                case 0:
                {
                    // 25% faded
                    if(dy % 2 == 0 && dx % 2 == 0)
                    {
                        drawPixel(dx, dy, BLACK);
                    }
                    break;
                }
                case 1:
                {
                    // 37.5% faded
                    if(dy % 2 == 0 && dx % 2 == 0)
                    {
                        drawPixel(dx, dy, BLACK);
                    }
                    else if (dx % 4 == 0)
                    {
                        drawPixel(dx, dy, BLACK);
                    }
                    break;
                }
                case 2:
                {
                    // 50% faded
                    if((dy % 2) == (dx % 2))
                    {
                        drawPixel(dx, dy, BLACK);
                    }
                    break;
                }
                case 3:
                {
                    // 62.5% faded
                    if(dy % 2 == 0 && dx % 2 == 0)
                    {
                        drawPixel(dx, dy, BLACK);
                    }
                    else if (dx % 4 < 3)
                    {
                        drawPixel(dx, dy, BLACK);
                    }
                    break;
                }
                case 4:
                {
                    // 75% faded
                    if(dy % 2 == 0 || dx % 2 == 0)
                    {
                        drawPixel(dx, dy, BLACK);
                    }
                    break;
                }
                default:
                {
                    return;
                }
            }
        }
    }
}

/**
 * @brief Optimized method to quickly draw a white line.
 *
 * @param x1, x0 Column of display, 0 is at the left
 * @param y1, y0 Row of the display, 0 is at the top
 *
 */
void ICACHE_FLASH_ATTR speedyWhiteLine( int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool thicc )
{
    //Tune this as a function of the size of your viewing window, line accuracy, and worst-case scenario incoming lines.
#define BRESEN_W OLED_WIDTH
#define BRESEN_H OLED_HEIGHT
#define FIXEDPOINT 16
#define FIXEDPOINTD2 15
    int dx = (x1 - x0);
    int dy = (y1 - y0);
    int sdx = (dx > 0) ? 1 : -1;
    int sdy = (dy > 0) ? 1 : -1;
    int yerrdiv = ( dx * sdx );  //dy, but always positive.
    int xerrdiv = ( dy * sdy );  //dx, but always positive.
    int yerrnumerator = 0;
    int xerrnumerator = 0;
    int cx = x0;
    int cy = y0;

    if( cx < 0 && x1 < 0 )
    {
        return;
    }
    if( cy < 0 && y1 < 0 )
    {
        return;
    }
    if( cx >= BRESEN_W && x1 >= BRESEN_W )
    {
        return;
    }
    if( cy >= BRESEN_H && y1 >= BRESEN_H )
    {
        return;
    }

    //We put the checks above to check this, in case we have a situation where
    // we have a 0-length line outside of the viewable area.  If that happened,
    // we would have aborted before hitting this code.

    if( yerrdiv > 0 )
    {
        int dxA = 0;
        if( cx < 0 )
        {
            dxA = 0 - cx;
            cx = 0;
        }
        if( cx > BRESEN_W - 1 )
        {
            dxA = (cx - (BRESEN_W - 1));
            cx = BRESEN_W - 1;
        }
        if( dxA || xerrdiv <= yerrdiv )
        {
            yerrnumerator = (((dy * sdy) << FIXEDPOINT) + yerrdiv / 2) / yerrdiv;
            if( dxA )
            {
                cy += (((yerrnumerator * dxA)) * sdy) >> FIXEDPOINT; //This "feels" right
                //Weird situation - if we cal, and now, both ends are out on the same side abort.
                if( cy < 0 && y1 < 0 )
                {
                    return;
                }
                if( cy > BRESEN_H - 1 && y1 > BRESEN_H - 1 )
                {
                    return;
                }
            }
        }
    }

    if( xerrdiv > 0 )
    {
        int dyA = 0;
        if( cy < 0 )
        {
            dyA = 0 - cy;
            cy = 0;
        }
        if( cy > BRESEN_H - 1 )
        {
            dyA = (cy - (BRESEN_H - 1));
            cy = BRESEN_H - 1;
        }
        if( dyA || xerrdiv > yerrdiv )
        {
            xerrnumerator = (((dx * sdx) << FIXEDPOINT) + xerrdiv / 2 ) / xerrdiv;
            if( dyA )
            {
                cx += (((xerrnumerator * dyA)) * sdx) >> FIXEDPOINT; //This "feels" right.
                //If we've come to discover the line is actually out of bounds, abort.
                if( cx < 0 && x1 < 0 )
                {
                    return;
                }
                if( cx > BRESEN_W - 1 && x1 > BRESEN_W - 1 )
                {
                    return;
                }
            }
        }
    }

    if( x1 == cx && y1 == cy )
    {
        drawPixelUnsafe( cx, cy );
        return;
    }

    //Make sure we haven't clamped the wrong way.
    //Also this checks for vertical/horizontal violations.
    if( dx > 0 )
    {
        if( cx > BRESEN_W - 1 )
        {
            return;
        }
        if( cx > x1 )
        {
            return;
        }
    }
    else if( dx < 0 )
    {
        if( cx < 0 )
        {
            return;
        }
        if( cx < x1 )
        {
            return;
        }
    }

    if( dy > 0 )
    {
        if( cy > BRESEN_H - 1 )
        {
            return;
        }
        if( cy > y1 )
        {
            return;
        }
    }
    else if( dy < 0 )
    {
        if( cy < 0 )
        {
            return;
        }
        if( cy < y1 )
        {
            return;
        }
    }

    //Force clip end coordinate.
    //NOTE: We have an extra check within the inner loop, to avoid complicated math here.
    //Theoretically, we could math this so that in the end-coordinate clip stage
    //to make sure this condition just could never be hit, however, that is very
    //difficult to guarantee under all situations and may have weird edge cases.
    //So, I've decided to stick this here.

    if( xerrdiv > yerrdiv )
    {
        int xerr = 1 << FIXEDPOINTD2;
        if( x1 < 0 )
        {
            x1 = 0;
        }
        if( x1 > BRESEN_W - 1)
        {
            x1 = BRESEN_W - 1;
        }
        x1 += sdx; //Tricky - make sure the "next" mark we hit doesn't overflow.

        if( y1 < 0 )
        {
            y1 = 0;
        }
        if( y1 > BRESEN_H - 1 )
        {
            y1 = BRESEN_H - 1;
        }

        for( ; cy != y1; cy += sdy )
        {
            drawPixelUnsafe( cx, cy );
            xerr += xerrnumerator;
            while( xerr >= (1 << FIXEDPOINT) )
            {
                cx += sdx;
                if( cx == x1 )
                {
                    return;
                }
                if( thicc )
                {
                    drawPixelUnsafe( cx, cy );
                }
                xerr -= 1 << FIXEDPOINT;
            }
        }
        drawPixelUnsafe( cx, cy );
    }
    else
    {
        int yerr = 1 << FIXEDPOINTD2;

        if( y1 < 0 )
        {
            y1 = 0;
        }
        if( y1 > BRESEN_H - 1 )
        {
            y1 = BRESEN_H - 1;
        }
        y1 += sdy;        //Tricky: Make sure the NEXT mark we hit doens't overflow.

        if( x1 < 0 )
        {
            x1 = 0;
        }
        if( x1 > BRESEN_W - 1)
        {
            x1 = BRESEN_W - 1;
        }

        for( ; cx != x1; cx += sdx )
        {
            drawPixelUnsafe( cx, cy );
            yerr += yerrnumerator;
            while( yerr >= 1 << FIXEDPOINT )
            {
                cy += sdy;
                if( cy == y1 )
                {
                    return;
                }
                if( thicc )
                {
                    drawPixelUnsafe( cx, cy );
                }
                yerr -= 1 << FIXEDPOINT;
            }
        }
        drawPixelUnsafe( cx, cy );
    }
}




/**
 * @brief Optimized method to quickly draw a black line.
 *
 * @param x1, x0 Column of display, 0 is at the left
 * @param y1, y0 Row of the display, 0 is at the top
 *
 */
void ICACHE_FLASH_ATTR speedyBlackLine( int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool thicc )
{
    //Tune this as a function of the size of your viewing window, line accuracy, and worst-case scenario incoming lines.
#define BRESEN_W OLED_WIDTH
#define BRESEN_H OLED_HEIGHT
#define FIXEDPOINT 16
#define FIXEDPOINTD2 15
    int dx = (x1 - x0);
    int dy = (y1 - y0);
    int sdx = (dx > 0) ? 1 : -1;
    int sdy = (dy > 0) ? 1 : -1;
    int yerrdiv = ( dx * sdx );  //dy, but always positive.
    int xerrdiv = ( dy * sdy );  //dx, but always positive.
    int yerrnumerator = 0;
    int xerrnumerator = 0;
    int cx = x0;
    int cy = y0;

    if( cx < 0 && x1 < 0 )
    {
        return;
    }
    if( cy < 0 && y1 < 0 )
    {
        return;
    }
    if( cx >= BRESEN_W && x1 >= BRESEN_W )
    {
        return;
    }
    if( cy >= BRESEN_H && y1 >= BRESEN_H )
    {
        return;
    }

    //We put the checks above to check this, in case we have a situation where
    // we have a 0-length line outside of the viewable area.  If that happened,
    // we would have aborted before hitting this code.

    if( yerrdiv > 0 )
    {
        int dxA = 0;
        if( cx < 0 )
        {
            dxA = 0 - cx;
            cx = 0;
        }
        if( cx > BRESEN_W - 1 )
        {
            dxA = (cx - (BRESEN_W - 1));
            cx = BRESEN_W - 1;
        }
        if( dxA || xerrdiv <= yerrdiv )
        {
            yerrnumerator = (((dy * sdy) << FIXEDPOINT) + yerrdiv / 2) / yerrdiv;
            if( dxA )
            {
                cy += (((yerrnumerator * dxA)) * sdy) >> FIXEDPOINT; //This "feels" right
                //Weird situation - if we cal, and now, both ends are out on the same side abort.
                if( cy < 0 && y1 < 0 )
                {
                    return;
                }
                if( cy > BRESEN_H - 1 && y1 > BRESEN_H - 1 )
                {
                    return;
                }
            }
        }
    }

    if( xerrdiv > 0 )
    {
        int dyA = 0;
        if( cy < 0 )
        {
            dyA = 0 - cy;
            cy = 0;
        }
        if( cy > BRESEN_H - 1 )
        {
            dyA = (cy - (BRESEN_H - 1));
            cy = BRESEN_H - 1;
        }
        if( dyA || xerrdiv > yerrdiv )
        {
            xerrnumerator = (((dx * sdx) << FIXEDPOINT) + xerrdiv / 2 ) / xerrdiv;
            if( dyA )
            {
                cx += (((xerrnumerator * dyA)) * sdx) >> FIXEDPOINT; //This "feels" right.
                //If we've come to discover the line is actually out of bounds, abort.
                if( cx < 0 && x1 < 0 )
                {
                    return;
                }
                if( cx > BRESEN_W - 1 && x1 > BRESEN_W - 1 )
                {
                    return;
                }
            }
        }
    }

    if( x1 == cx && y1 == cy )
    {
        drawPixelUnsafeBlack( cx, cy );
        return;
    }

    //Make sure we haven't clamped the wrong way.
    //Also this checks for vertical/horizontal violations.
    if( dx > 0 )
    {
        if( cx > BRESEN_W - 1 )
        {
            return;
        }
        if( cx > x1 )
        {
            return;
        }
    }
    else if( dx < 0 )
    {
        if( cx < 0 )
        {
            return;
        }
        if( cx < x1 )
        {
            return;
        }
    }

    if( dy > 0 )
    {
        if( cy > BRESEN_H - 1 )
        {
            return;
        }
        if( cy > y1 )
        {
            return;
        }
    }
    else if( dy < 0 )
    {
        if( cy < 0 )
        {
            return;
        }
        if( cy < y1 )
        {
            return;
        }
    }

    //Force clip end coordinate.
    //NOTE: We have an extra check within the inner loop, to avoid complicated math here.
    //Theoretically, we could math this so that in the end-coordinate clip stage
    //to make sure this condition just could never be hit, however, that is very
    //difficult to guarantee under all situations and may have weird edge cases.
    //So, I've decided to stick this here.

    if( xerrdiv > yerrdiv )
    {
        int xerr = 1 << FIXEDPOINTD2;
        if( x1 < 0 )
        {
            x1 = 0;
        }
        if( x1 > BRESEN_W - 1)
        {
            x1 = BRESEN_W - 1;
        }
        x1 += sdx; //Tricky - make sure the "next" mark we hit doesn't overflow.

        if( y1 < 0 )
        {
            y1 = 0;
        }
        if( y1 > BRESEN_H - 1 )
        {
            y1 = BRESEN_H - 1;
        }

        for( ; cy != y1; cy += sdy )
        {
            drawPixelUnsafeBlack( cx, cy );
            xerr += xerrnumerator;
            while( xerr >= (1 << FIXEDPOINT) )
            {
                cx += sdx;
                if( cx == x1 )
                {
                    return;
                }
                if( thicc )
                {
                    drawPixelUnsafeBlack( cx, cy );
                }
                xerr -= 1 << FIXEDPOINT;
            }
        }
        drawPixelUnsafeBlack( cx, cy );
    }
    else
    {
        int yerr = 1 << FIXEDPOINTD2;

        if( y1 < 0 )
        {
            y1 = 0;
        }
        if( y1 > BRESEN_H - 1 )
        {
            y1 = BRESEN_H - 1;
        }
        y1 += sdy;        //Tricky: Make sure the NEXT mark we hit doens't overflow.

        if( x1 < 0 )
        {
            x1 = 0;
        }
        if( x1 > BRESEN_W - 1)
        {
            x1 = BRESEN_W - 1;
        }

        for( ; cx != x1; cx += sdx )
        {
            drawPixelUnsafeBlack( cx, cy );
            yerr += yerrnumerator;
            while( yerr >= 1 << FIXEDPOINT )
            {
                cy += sdy;
                if( cy == y1 )
                {
                    return;
                }
                if( thicc )
                {
                    drawPixelUnsafeBlack( cx, cy );
                }
                yerr -= 1 << FIXEDPOINT;
            }
        }
        drawPixelUnsafeBlack( cx, cy );
    }
}


/**
 * @brief Optimized method to draw a triangle with outline.
 *
 * @param x2, x1, x0 Column of display, 0 is at the left
 * @param y2, y1, y0 Row of the display, 0 is at the top
 * @param colorA filled area color
 * @param colorB outline color
 *
 */
void ICACHE_FLASH_ATTR outlineTriangle( int16_t v0x, int16_t v0y, int16_t v1x, int16_t v1y,
                                        int16_t v2x, int16_t v2y, color colorA, color colorB )
{
    int16_t i16tmp;

    //Sort triangle such that v0 is the top-most vertex.
    //v0->v1 is LEFT edge.
    //v0->v2 is RIGHT edge.

    if( v0y > v1y )
    {
        i16tmp = v0x;
        v0x = v1x;
        v1x = i16tmp;
        i16tmp = v0y;
        v0y = v1y;
        v1y = i16tmp;
    }
    if( v0y > v2y )
    {
        i16tmp = v0x;
        v0x = v2x;
        v2x = i16tmp;
        i16tmp = v0y;
        v0y = v2y;
        v2y = i16tmp;
    }

    //v0 is now top-most vertex.  Now orient 2 and 3.
    //Tricky: Use slopes!  Otherwise, we could get it wrong.
    {
        int slope02;
        if( v2y - v0y )
        {
            slope02 = ((v2x - v0x) << FIXEDPOINT) / (v2y - v0y);
        }
        else
        {
            slope02 = ((v2x - v0x) > 0) ? 0x7fffff : -0x800000;
        }

        int slope01;
        if( v1y - v0y )
        {
            slope01 = ((v1x - v0x) << FIXEDPOINT) / (v1y - v0y);
        }
        else
        {
            slope01 = ((v1x - v0x) > 0) ? 0x7fffff : -0x800000;
        }

        if( slope02 < slope01 )
        {
            i16tmp = v1x;
            v1x = v2x;
            v2x = i16tmp;
            i16tmp = v1y;
            v1y = v2y;
            v2y = i16tmp;
        }
    }

    //We now have a fully oriented triangle.
    int16_t x0A = v0x;
    int16_t y0A = v0y;
    int16_t x0B = v0x;
    //int16_t y0B = v0y;

    //A is to the LEFT of B.
    int dxA = (v1x - v0x);
    int dyA = (v1y - v0y);
    int dxB = (v2x - v0x);
    int dyB = (v2y - v0y);
    int sdxA = (dxA > 0) ? 1 : -1;
    int sdyA = (dyA > 0) ? 1 : -1;
    int sdxB = (dxB > 0) ? 1 : -1;
    int sdyB = (dyB > 0) ? 1 : -1;
    int xerrdivA = ( dyA * sdyA );  //dx, but always positive.
    int xerrdivB = ( dyB * sdyB );  //dx, but always positive.
    int xerrnumeratorA = 0;
    int xerrnumeratorB = 0;

    if( xerrdivA )
    {
        xerrnumeratorA = (((dxA * sdxA) << FIXEDPOINT) + xerrdivA / 2 ) / xerrdivA;
    }
    else
    {
        xerrnumeratorA = 0x7fffff;
    }

    if( xerrdivB )
    {
        xerrnumeratorB = (((dxB * sdxB) << FIXEDPOINT) + xerrdivB / 2 ) / xerrdivB;
    }
    else
    {
        xerrnumeratorB = 0x7fffff;
    }

    //X-clipping is handled on a per-scanline basis.
    //Y-clipping must be handled upfront.

    /*
        //Optimization BUT! Can't do this here, as we would need to be smarter about it.
        //If we do this, and the second triangle is above y=0, we'll get the wrong answer.
        if( y0A < 0 )
        {
            delta = 0 - y0A;
            y0A = 0;
            y0B = 0;
            x0A += (((xerrnumeratorA*delta)) * sdxA) >> FIXEDPOINT; //Could try rounding.
            x0B += (((xerrnumeratorB*delta)) * sdxB) >> FIXEDPOINT;
        }
    */

    {
        //Section 1 only.
        int yend = (v1y < v2y) ? v1y : v2y;
        int errA = 1 << FIXEDPOINTD2;
        int errB = 1 << FIXEDPOINTD2;
        int y;

        //Going between x0A and x0B
        for( y = y0A; y < yend; y++ )
        {
            int x = x0A;
            int endx = x0B;
            int suppress = 1;

            if( y >= 0 && y <= (BRESEN_H - 1) )
            {
                suppress = 0;
                if( x < 0 )
                {
                    x = 0;
                }
                if( endx > (BRESEN_W) )
                {
                    endx = (BRESEN_W);
                }
                if( x0A >= 0  && x0A <= (BRESEN_W - 1) )
                {
                    drawPixelUnsafeC( x0A, y, colorB );
                    x++;
                }
                for( ; x < endx; x++ )
                {
                    drawPixelUnsafeC( x, y, colorA );
                }
                if( x0B <= (BRESEN_W - 1) && x0B >= 0 )
                {
                    drawPixelUnsafeC( x0B, y, colorB );
                }
            }

            //Now, advance the start/end X's.
            errA += xerrnumeratorA;
            errB += xerrnumeratorB;
            while( errA >= (1 << FIXEDPOINT) && x0A != v1x )
            {
                x0A += sdxA;
                //if( x0A < 0 || x0A > (BRESEN_W-1) ) break;
                if( x0A >= 0 && x0A <= (BRESEN_W - 1) && !suppress )
                {
                    drawPixelUnsafeC( x0A, y, colorB );
                }
                errA -= 1 << FIXEDPOINT;
            }
            while( errB >= (1 << FIXEDPOINT) && x0B != v2x )
            {
                x0B += sdxB;
                //if( x0B < 0 || x0B > (BRESEN_W-1) ) break;
                if( x0B >= 0 && x0B <= (BRESEN_W - 1) && !suppress )
                {
                    drawPixelUnsafeC( x0B, y, colorB );
                }
                errB -= 1 << FIXEDPOINT;
            }
        }

        //We've come to the end of section 1.  Now, we need to figure

        //Now, yend is the highest possible hit on the triangle.
        yend = (v1y < v2y) ? v2y : v1y;

        //v1 is LEFT OF v2
        // A is LEFT OF B
        if( v1y < v2y )
        {
            //V1 has terminated, move to V1->V2 but keep V0->V2[B] segment
            yend = v2y;
            dxA = (v2x - v1x);
            dyA = (v2y - v1y);
            sdxA = (dxA > 0) ? 1 : -1;
            sdyA = (dyA > 0) ? 1 : -1;
            xerrdivA = ( dyA * sdyA );  //dx, but always positive.
            if( xerrdivA )
            {
                xerrnumeratorA = (((dxA * sdxA) << FIXEDPOINT) + xerrdivA / 2 ) / xerrdivA;
            }
            else
            {
                xerrnumeratorA = 0x7fffff;
            }
            x0A = v1x;
            errA = 1 << FIXEDPOINTD2;
        }
        else
        {
            //V2 has terminated, move to V2->V1 but keep V0->V1[A] segment
            yend = v1y;
            dxB = (v1x - v2x);
            dyB = (v1y - v2y);
            sdxB = (dxB > 0) ? 1 : -1;
            sdyB = (dyB > 0) ? 1 : -1;
            xerrdivB = ( dyB * sdyB );  //dx, but always positive.
            if( xerrdivB )
            {
                xerrnumeratorB = (((dxB * sdxB) << FIXEDPOINT) + xerrdivB / 2 ) / xerrdivB;
            }
            else
            {
                xerrnumeratorB = 0x7fffff;
            }
            x0B = v2x;
            errB = 1 << FIXEDPOINTD2;
        }

        if( yend > (BRESEN_H - 1) )
        {
            yend = BRESEN_H - 1;
        }



        if( xerrnumeratorA > 1000000 || xerrnumeratorB > 1000000 )
        {
            if( x0A < x0B )
            {
                sdxA = 1;
                sdxB = -1;
            }
            if( x0A > x0B )
            {
                sdxA = -1;
                sdxB = 1;
            }
            if( x0A == x0B )
            {
                if( x0A >= 0 && x0A <= (BRESEN_W - 1) && y >= 0 && y <= (BRESEN_H - 1) )
                {
                    drawPixelUnsafeC( x0A, y, colorB );
                }
                return;
            }
        }

        for( ; y <= yend; y++ )
        {
            int x = x0A;
            int endx = x0B;
            int suppress = 1;

            if( y >= 0 && y <= (BRESEN_H - 1) )
            {
                suppress = 0;
                if( x < 0 )
                {
                    x = 0;
                }
                if( endx >= (BRESEN_W) )
                {
                    endx = (BRESEN_W);
                }
                if( x0A >= 0  && x0A <= (BRESEN_W - 1) )
                {
                    drawPixelUnsafeC( x0A, y, colorB );
                    x++;
                }
                for( ; x < endx; x++ )
                {
                    drawPixelUnsafeC( x, y, colorA );
                }
                if( x0B <= (BRESEN_W - 1) && x0B >= 0 )
                {
                    drawPixelUnsafeC( x0B, y, colorB );
                }
            }

            //Now, advance the start/end X's.
            errA += xerrnumeratorA;
            errB += xerrnumeratorB;
            while( errA >= (1 << FIXEDPOINT) )
            {
                x0A += sdxA;
                //if( x0A < 0 || x0A > (BRESEN_W-1) ) break;
                if( x0A >= 0 && x0A <= (BRESEN_W - 1) && !suppress )
                {
                    drawPixelUnsafeC( x0A, y, colorB );
                }
                errA -= 1 << FIXEDPOINT;
                if( x0A == x0B  )
                {
                    return;
                }
            }
            while( errB >= (1 << FIXEDPOINT) )
            {
                x0B += sdxB;
                if( x0B >= 0 && x0B <= (BRESEN_W - 1) && !suppress )
                {
                    drawPixelUnsafeC( x0B, y, colorB );
                }
                errB -= 1 << FIXEDPOINT;
                if( x0A == x0B  )
                {
                    return;
                }
            }

        }
    }
}
