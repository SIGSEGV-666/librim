#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
//#include "cstruct2.c"
#define RIM_MAGIC_LEN 7

typedef enum {
    RIMSTAT_OK,
    RIMSTAT_FILENOTFOUND,
    RIMSTAT_NOT_A_RIM,
    RIMSTAT_EARLY_EOF,
    RIMSTAT_NULL_FILE,
    RIMSTAT_NULL_PTR,
    RIMSTAT_NULL_OUTRIM,
    RIMSTAT_NULL_INRIM,
    RIMSTAT_WRITE_ERROR
} rim_status;
const char* STR_RIMSTAT_OK = "Operation successful!";
const char* STR_RIMSTAT_FILENOTFOUND = "File not found.";
const char* STR_RIMSTAT_NOT_A_RIM = "Not a valid RIM!";
const char* STR_RIMSTAT_EARLY_EOF = "Premature EOF in file.";
const char* STR_RIMSTAT_NULL_FILE = "NULL file pointer.";
const char* STR_RIMSTAT_NULL_PTR  = "Generic NULL pointer error.";
const char* STR_RIMSTAT_NULL_OUTRIM = "outrim** pointer is NULL.";
const char* STR_RIMSTAT_NULL_INRIM = "rim* pointer is NULL.";
const char* STR_RIMSTAT_WRITE_ERROR = "A writing error occurred.";
const char* rimstatus2str(rim_status s)
{
    if (s == RIMSTAT_FILENOTFOUND)
    {
        return STR_RIMSTAT_FILENOTFOUND;
    }
    if (s == RIMSTAT_NOT_A_RIM)
    {
        return STR_RIMSTAT_NOT_A_RIM;
    }
    if (s == RIMSTAT_EARLY_EOF)
    {
        return STR_RIMSTAT_EARLY_EOF;
    }
    if (s == RIMSTAT_NULL_FILE)
    {
        return STR_RIMSTAT_NULL_FILE;
    }
    if (s == RIMSTAT_NULL_PTR)
    {
        return STR_RIMSTAT_NULL_PTR;
    }
    if (s == RIMSTAT_NULL_OUTRIM)
    {
        return STR_RIMSTAT_NULL_OUTRIM;
    }
    if (s == RIMSTAT_NULL_INRIM)
    {
        return STR_RIMSTAT_NULL_INRIM;
    }
    if (s == RIMSTAT_WRITE_ERROR)
    {
        return STR_RIMSTAT_WRITE_ERROR;
    }
    return STR_RIMSTAT_OK;
}

typedef uint32_t rim_colorfmt_t;
const rim_colorfmt_t COLFMT_GRAYSCALE = 1;
const rim_colorfmt_t COLFMT_GRAYALPHA = 2;
const rim_colorfmt_t COLFMT_RGB       = 3;
const rim_colorfmt_t COLFMT_RGBA      = 4;


const char RIM_MAGIC[RIM_MAGIC_LEN] = "RAWIMG\0";
const char RIM_MAGIC_INDEXED[RIM_MAGIC_LEN] = "RAWIMGP";
typedef struct rim_t {
    uint32_t width;
    uint32_t height;
    rim_colorfmt_t colorfmt;
    uint8_t* palette; //256 color palette, can be NULL if image isn't indexed.
    uint8_t* pixels;
} rim_t;
/*
bool read_uint32le(FILE* in, uint32_t* out)
{
    return cs2_read_uint32(in, out, CS2_LITTLE_ENDIAN);
}
bool write_uint32le(uint32_t value, FILE* fp)
{
    return cs2_write_uint32(value, fp, CS2_LITTLE_ENDIAN);
}
bool rim_read_uint32le(FILE* in, uint32_t* out)
{
    uint32_t i = 0;
    uint8_t buf[4] = {0, 0, 0, 0};
    size_t nread = fread(buf, 1, 4, in);
    //printf("nread = %u\n", nread);
    if (nread != 4)
    {
        return false;
    }
    //printf("DEBUG: freaded!\n");
    int index = 0;
    for (unsigned int exp = 0; exp < 32; exp+=8)
    {
        i |= (((uint32_t)(buf[index])) << exp);
        index++;
    }
    (*out) = i;
    return true;
}
*/
bool rim_machine_is_little_endian()
{
    uint8_t intbuf[4] = {1, 0, 0, 0};
    uint32_t endian_check = *((uint32_t*)intbuf);
    return endian_check == 1;
}
bool rim_read_uint32le(FILE* in, uint32_t* out)
{
    uint8_t bufs[2][4] = { };
    size_t nread = fread(bufs[0], 1, 4, in);
    if (nread != 4){return false;}
    if (!rim_machine_is_little_endian()){for (unsigned int i = 0; i < 4; i++){bufs[1][i] = bufs[0][3-i];}}
    else {memcpy(bufs[1], bufs[0], 4);}
    (*out) = *((uint32_t*)(bufs[1]));
    return true;
}
bool rim_write_uint32le(uint32_t value, FILE* fp)
{
    uint8_t bufs[2][4] = { };
    memcpy(bufs[0], &value, 4);
    if (!rim_machine_is_little_endian()){for (unsigned int i = 0; i < 4; i++){bufs[1][i] = bufs[0][3-i];}}
    else {memcpy(bufs[1], bufs[0], 4);}
    size_t nwritten = fwrite(bufs[1], 1, 4, fp);
    if (nwritten < 4){return false;}
    return true;
}
bool is_rim_indexed(rim_t* img)
{
    if (img == NULL)
    {
        return false;
    }
    return (img->palette != NULL);
}
rim_t* malloc_rim()
{
    rim_t* rim = malloc(sizeof(rim_t));
    if (rim == NULL)
    {
        return NULL;
    }
    rim->palette = NULL;
    rim->pixels = NULL;
    return rim;
}
void free_rim(rim_t** rimptr)
{
    //printf("stage 1\n");
    if (rimptr == NULL)
    {
        return;
    }
    //printf("stage 2\n");
    if ((*rimptr) == NULL)
    {
        return;
    }
    //printf("stage 3\n");
    if ((*rimptr)->palette != NULL)
    {
        free((*rimptr)->palette);
        //printf("freed palette\n");
        (*rimptr)->palette = NULL;
    }
    //printf("stage 4\n");
    if ((*rimptr)->pixels != NULL)
    {
        free((*rimptr)->pixels);
        (*rimptr)->pixels = NULL;
    }
    //printf("stage 5\n");
    free(*rimptr);
    (*rimptr) = NULL;
    return;
}
rim_status load_rimf_nef(FILE* fp, rim_t** outrim)
{
    if (fp == NULL)
    {
        return RIMSTAT_NULL_FILE;
    }
    if (outrim == NULL)
    {
        return RIMSTAT_NULL_OUTRIM;
    }
    if ((*outrim) == NULL)
    {
        return RIMSTAT_NULL_PTR;
    }
    char magicbuf[RIM_MAGIC_LEN] = { };
    //printf("DEBUG: reading magic.\n");
    size_t nmread = fread(magicbuf, 1, RIM_MAGIC_LEN, fp);
    //printf("nmread = %u\n", nmread);
    if (nmread != RIM_MAGIC_LEN)
    {
        return RIMSTAT_EARLY_EOF;
    }
    bool is_og_magic, is_ind_magic;
    is_og_magic = true; is_ind_magic = true;
    char cmchar;
    //printf("DEBUG: comparing magic.\n");
    for (int i = 0; i < RIM_MAGIC_LEN; i++)
    {
        cmchar = magicbuf[i];
        if (cmchar != RIM_MAGIC[i])
        {
            is_og_magic = false;
        }
        if (cmchar != RIM_MAGIC_INDEXED[i])
        {
            is_ind_magic = false;
        }
    }
    if ((!is_og_magic) && (!is_ind_magic))
    {
        //printf("DEBUG: Not a rim.\n");
        return RIMSTAT_NOT_A_RIM;
    }
    bool haspal = is_ind_magic;
    //printf("DEBUG: read RIM header.\n");
    if ((!rim_read_uint32le(fp, &((*outrim)->width))) || (!rim_read_uint32le(fp, &((*outrim)->height))) || (!rim_read_uint32le(fp, &((*outrim)->colorfmt))))
    {
        return RIMSTAT_EARLY_EOF;
    }
    //printf("DEBUG: done reading header.\n");
    uint32_t width, height;
    width = (*outrim)->width; height = (*outrim)->height;
    size_t pxarrsize;
    rim_colorfmt_t colorfmt = (*outrim)->colorfmt;
    //printf("DEBUG: width=%u, height=%u, colorfmt=%u\n", width, height, colorfmt);
    if (haspal)
    {
        //printf("DEBUG: read palette.\n");
        (*outrim)->palette = malloc(256*colorfmt);
        if (fread((*outrim)->palette, 1, 256*colorfmt, fp) < (256*colorfmt))
        {
            return RIMSTAT_EARLY_EOF;
        }
        pxarrsize = width*height;
    }
    else
    {
        (*outrim)->palette = NULL;
        pxarrsize = width*height*colorfmt;
    }
    //printf("DEBUG: read pixels array.\n");
    (*outrim)->pixels = malloc(pxarrsize);
    if (fread((*outrim)->pixels, 1, pxarrsize, fp) < pxarrsize)
    {
        return RIMSTAT_EARLY_EOF;
    }
    //printf("DEBUG: done!\n");
    return RIMSTAT_OK;
}
rim_t* load_rimf(FILE* fp, void (*on_error)(rim_status, void*))
{
    rim_t* out = malloc_rim();
    rim_status stat = load_rimf_nef(fp, &out);
    if (stat != RIMSTAT_OK)
    {
        free_rim(&out);
        //printf("DEBUG: freed rim on error.\n");
        if (on_error != NULL)
        {
            on_error(stat, NULL);
        }
        return NULL;
    }
    return out;
}
rim_t* load_rim(char* path, void (*on_error)(rim_status, void*))
{
    FILE* fp = fopen(path, "rb");
    if (fp == NULL)
    {
        if (on_error != NULL)
        {
            on_error(RIMSTAT_FILENOTFOUND, path);
        }
        return NULL;
    }
    
    rim_t* res = load_rimf(fp, on_error);
    fclose(fp);
    return res;
}
rim_status save_rimf_nef(rim_t* rim, FILE* fp)
{
    if (fp == NULL)
    {
        return RIMSTAT_NULL_FILE;
    }
    if (rim == NULL)
    {
        return RIMSTAT_NULL_INRIM;
    }
    bool is_indexed = is_rim_indexed(rim);
    size_t num_fwritten;
    if (is_indexed)
    {
        num_fwritten = fwrite(RIM_MAGIC_INDEXED, RIM_MAGIC_LEN, 1, fp);
    }
    else
    {
        num_fwritten = fwrite(RIM_MAGIC, RIM_MAGIC_LEN, 1, fp);
    }
    if (num_fwritten != 1){return RIMSTAT_WRITE_ERROR;}
    printf("wrote RIM magic!\n");
    if ((!rim_write_uint32le(rim->width, fp)) || (!rim_write_uint32le(rim->height, fp)) || (!rim_write_uint32le(rim->colorfmt, fp)))
    {
        return RIMSTAT_WRITE_ERROR;
    }
    printf("wrote header!\n");
    size_t palsize = (rim->colorfmt)*256;
    size_t pxasize = (rim->width)*(rim->height);
    if (is_indexed)
    {
        if (fwrite(rim->palette, sizeof(uint8_t), palsize, fp) != palsize){return RIMSTAT_WRITE_ERROR;}
        printf("wrote palette!\n");
    }
    else
    {
        pxasize *= rim->colorfmt;
    }
    size_t nwritten = fwrite(rim->pixels, sizeof(uint8_t), pxasize, fp);
    if (nwritten != pxasize){return RIMSTAT_WRITE_ERROR;}
    printf("wrote pixels!\n");
    return RIMSTAT_OK;
}
uint8_t* get_rim_pixelptr(rim_t* rim, int x, int y)
{
    if (((x >= 0) && (x < (rim->width))) && ((y >= 0) && (y < (rim->height))))
    {
        return (rim->pixels)+((y*(rim->width))+x);
    }
    else
    {
        return NULL;
    }
}
bool get_rim_color(rim_t* rim, int x, int y, uint8_t* outcolor)
{
    uint8_t* pixelptr = get_rim_pixelptr(rim, x, y);
    if (pixelptr == NULL)
    {
        return false; //out-of-bounds access.
    }
    uint8_t* palptr = NULL; //pointer to palette entry.
    if (is_rim_indexed(rim))
    {
        palptr = (rim->palette)+(pixelptr[0]); //palette lookup.
        memcpy(outcolor, palptr, rim->colorfmt); //copy the result into outcolor.
        return true; //success!
    }
    else
    {
        memcpy(outcolor, pixelptr, rim->colorfmt); //copy the result into outcolor.
        return true; //success!
    }
    return false; //just for safe measure.
}
uint8_t* palette_as_rgb(uint8_t* palette, rim_colorfmt_t colorfmt)
{
    uint8_t* rgbpal = malloc(768);
    uint8_t* opalitem = NULL;
    uint8_t* cpalitem = NULL;
    for (int i = 0; i < 256; i++)
    {
        cpalitem = palette+(colorfmt*i);
        opalitem = rgbpal+(3*i);
        if (colorfmt == COLFMT_GRAYSCALE || colorfmt == COLFMT_GRAYALPHA)
        {
            opalitem[0] = opalitem[1] = opalitem[2] = cpalitem[0];
        }
        else if (colorfmt == COLFMT_RGB || colorfmt == COLFMT_RGBA)
        {
            for (int i = 0; i < 3; i++)
            {
                opalitem[i] = cpalitem[i];
            }
        }
    }
    return rgbpal;
}
