/*
 * Xpost - a Level-2 Postscript interpreter
 * Copyright (C) 2013-2016, Michael Joshua Ryan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the Xpost software product nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
   This is a simple example of a client calling xpost as a library
   with a postscript program, desiring the raster data of the
   generated image.

   The "raster" device can operate in different modes specified with a colon.
   "raster:rgb" (default) 24bit rgb
   "raster:argb" 32big argb
   "raster:bgr" 24bit bgr
   "raster:bgra" 32bit bgra

   The BUFFEROUT output type is currently the only practical output of the buffer.
   The pointer supplied (by reference) is updated by calls to the `showpage` operator.
   The buffer size is currently hardcoded to US Letter dimensions in Postscript units
   1 unit = 1/72 inch.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xpost.h"

const char *prog =
    "%%BoundingBox: 200 300 400 500\n"
    "0 0 1 setrgbcolor\n"
    "300 400 100 0 360 arc\n"
    "fill\n"
    "0 0 0 setrgbcolor\n"
    "290 390 moveto\n"
    "/Palatino-Roman 20 selectfont\n"
    "(Xpost) show\n"
    "showpage\n";

static void
_xpost_client_license(void)
{
    printf("BSD 3-clause\n");
}

static void
_xpost_client_version(const char *filename)
{
    int maj;
    int min;
    int mic;

    xpost_version_get(&maj, &min, &mic);
    printf("%s %d.%d.%d\n", filename, maj, min, mic);
}

static void
_xpost_client_usage(const char *filename)
{
    printf("Usage: %s [options] [file.png]\n\n", filename);
    printf("Postscript level 2 interpreter\n\n");
    printf("Options:\n");
    printf("  -q, --quiet    suppress interpreter messages (default)\n");
    printf("  -v, --verbose  do not go quiet into that good night\n");
    printf("  -t, --trace    add additional tracing messages, implies -v\n");
    printf("  -L, --license  show program license\n");
    printf("  -V, --version  show program version\n");
    printf("  -h, --help     show this message\n");
}

int main(int argc, const char *argv[])
{
    Xpost_Context *ctx;
    void *buffer_type_object;
    const char *filename;
    const char *device;
    const void *ptr;
    Xpost_Output_Type output_type;
    int ret;
    int output_msg = XPOST_OUTPUT_MESSAGE_QUIET;
    int i;

    filename = NULL;

    i = 0;
    while (++i < argc)
    {
        if (*argv[i] == '-')
        {
            if ((!strcmp(argv[i], "-h")) ||
                (!strcmp(argv[i], "--help")))
            {
                _xpost_client_usage(argv[0]);
                return EXIT_SUCCESS;
            }
            else if ((!strcmp(argv[i], "-V")) ||
                     (!strcmp(argv[i], "--version")))
            {
                _xpost_client_version(argv[0]);
                return EXIT_SUCCESS;
            }
            else if ((!strcmp(argv[i], "-L")) ||
                     (!strcmp(argv[i], "--license")))
            {
                _xpost_client_license();
                return EXIT_SUCCESS;
            }
            else if ((!strcmp(argv[i], "-q")) ||
                     (!strcmp(argv[i], "--quiet")))
            {
                output_msg = XPOST_OUTPUT_MESSAGE_QUIET;
            }
            else if ((!strcmp(argv[i], "-v")) ||
                     (!strcmp(argv[i], "--verbose")))
            {
                output_msg = XPOST_OUTPUT_MESSAGE_VERBOSE;
            }
            else if ((!strcmp(argv[i], "-t")) ||
                     (!strcmp(argv[i], "--trace")))
            {
                output_msg = XPOST_OUTPUT_MESSAGE_TRACING;
            }
            else
            {
                printf("unknown option\n");
                _xpost_client_usage(argv[0]);
                return EXIT_FAILURE;
            }
        }
        else
            filename = argv[i];
    }

    xpost_init();
    if (filename)
    {
        device = "png";
        output_type = XPOST_OUTPUT_FILENAME;
        ptr = filename;
    }
    else
    {
        device = "raster:bgr";
        output_type = XPOST_OUTPUT_BUFFEROUT;
        ptr = &buffer_type_object;
    }
    if (!(ctx = xpost_create(device,
                             output_type,
                             ptr,
                             XPOST_SHOWPAGE_RETURN,
                             output_msg,
                             XPOST_IGNORE_SIZE, 0, 0)))
    {
        fprintf(stderr, "unable to create interpreter context");
        exit(0);
    }
    printf("created interpreter context. executing program...\n");
    ret = xpost_run(ctx, XPOST_INPUT_STRING, prog);
    printf("executed program. xpost_run returned %s\n", ret? "yieldtocaller": "zero");
    if (!ret)
    {
        fprintf(stderr, "error before showpage\n");
    }
    else if (!filename)
    {
        typedef struct { unsigned char blue, green, red; } pixel;
        pixel *buffer = buffer_type_object;
        int i,j;
        FILE *fp = fopen("xpost_client_out.ppm", "w");
        fprintf(fp, "P3\n612 792\n255\n");
        for (i = 0; i < 792; i++)
        {
            for (j = 0; j < 612; j++)
            {
                pixel pix = *buffer++;
                fprintf(fp, "%d %d %d ", pix.red, pix.green, pix.blue);
                if ((j%20)==0)
                    fprintf(fp, "\n");
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
    }
    xpost_destroy(ctx);
    //free(buffer_type_object);
    xpost_quit();
    return 0;
}

