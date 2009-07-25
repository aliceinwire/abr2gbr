/*
 * Converts PhotoShop .ABR and Paint Shop Pro .JBR brushes to GIMP .GBR
 * Copyright (C) 2001 Marco Lamberto <lm@sunnyspot.org>
 * Web page: http://the.sunnyspot.org/gimp/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id:$
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>


#define GBRUSH_MAGIC    (('G' << 24) + ('I' << 16) + ('M' << 8) + ('P' << 0))


typedef struct _AbrHeader							AbrHeader;
typedef struct _AbrBrushHeader				AbrBrushHeader;
typedef struct _AbrSampledBrushHeader	AbrSampledBrushHeader;
typedef struct _GbrBrushHeader				GbrBrushHeader;
typedef struct _GbrBrush							GbrBrush;

struct _AbrHeader
{
	gshort version;
	gshort count;
};

struct _AbrBrushHeader
{
	gshort type;
	gint32 size;
};

struct _AbrSampledBrushHeader
{
	gint32 misc;
	gshort spacing;
	gchar antialiasing;
	gshort bounds[4];
	gint32 bounds_long[4];
	gshort depth;
	gboolean wide;
};

struct _GbrBrushHeader
{
	guint header_size;
	guint version;
	guint width;
	guint height;
	guint depth;
	guint magic_number;
	guint spacing;
};

struct _GbrBrush
{
	GbrBrushHeader header;
	gchar *name;
	gchar *description;
	gchar *data;
	gint size;
};


gchar				abr_read_char		(FILE *abr);
gshort			abr_read_short	(FILE *abr);
gint32 			abr_read_long		(FILE *abr);
void				abr_load				(const gchar *file);
GbrBrush *	abr_brush_load	(FILE *abr);
void				gbr_brush_save	(GbrBrush *brush, const gchar *file, const gint id);
void				gbr_brush_free	(GbrBrush *brush);
gint				main						(gint argc, gchar *argv[]);


gchar
abr_read_char(FILE *abr)
{
	g_return_val_if_fail ( abr != NULL, 0 );

	return fgetc(abr);
}

gshort
abr_read_short(FILE *abr)
{
	gshort val;

	g_return_val_if_fail ( abr != NULL, 0 );

	fread(&val, sizeof(val), 1, abr);
	
	return GUINT16_SWAP_LE_BE(val);
}

gint32
abr_read_long(FILE *abr)
{
	gint32 val;

	g_return_val_if_fail ( abr != NULL, 0 );
	
	fread(&val, sizeof(val), 1, abr);

	return GUINT32_SWAP_LE_BE(val);
}

GbrBrush *
abr_brush_load(FILE *abr)
{
	AbrBrushHeader abr_brush_hdr;
	GbrBrush *gbr = NULL;

	g_return_val_if_fail ( abr != NULL, NULL );

	abr_brush_hdr.type = abr_read_short(abr);
	abr_brush_hdr.size = abr_read_long(abr);
	g_print(" + BRUSH\n | << type: %d  block size: %d bytes\n",
		abr_brush_hdr.type, abr_brush_hdr.size);

	switch (abr_brush_hdr.type) {
	case 1: /* computed brush */
		/* FIXME: support it! */
		g_print("WARNING: computed brush unsupported, skipping.\n");
		fseek(abr, abr_brush_hdr.size, SEEK_CUR);
		break;
	case 2: /* sampled brush */
		{
			AbrSampledBrushHeader abr_sampled_brush_hdr;
			gint width, height;
			gint size;
			gint i = 0;

			abr_sampled_brush_hdr.misc					= abr_read_long(abr);
			abr_sampled_brush_hdr.spacing				= abr_read_short(abr);
			abr_sampled_brush_hdr.antialiasing	= abr_read_char(abr);

			for (i = 0; i < 4; i++)
				abr_sampled_brush_hdr.bounds[i] = abr_read_short(abr);
			for (i = 0; i < 4; i++) {
				abr_sampled_brush_hdr.bounds_long[i] = abr_read_long(abr);
			}
			abr_sampled_brush_hdr.depth					= abr_read_short(abr);

			height = abr_sampled_brush_hdr.bounds_long[2] - 
				abr_sampled_brush_hdr.bounds_long[0]; /* bottom - top */
			width = abr_sampled_brush_hdr.bounds_long[3] -
				abr_sampled_brush_hdr.bounds_long[1]; /* right - left */
			size = width * (abr_sampled_brush_hdr.depth >> 3) * height;

			/* FIXME: support wide brushes */
			abr_sampled_brush_hdr.wide = height > 16384;
			if (abr_sampled_brush_hdr.wide)
				g_print("WARING: wide brushes not supported\n");

			gbr = g_new0(GbrBrush, 1);
			gbr->header.version				= g_htonl(2);
			gbr->header.width					= g_htonl(width);
			gbr->header.height				= g_htonl(height);
			gbr->header.depth					= g_htonl(abr_sampled_brush_hdr.depth >> 3);
			gbr->header.magic_number	= g_htonl(GBRUSH_MAGIC);
			gbr->header.spacing				= g_htonl(abr_sampled_brush_hdr.spacing);
			gbr->data									= g_new0(gchar, size);
			gbr->size									= size;

			/* data decoding */
			{
				gshort comp;

				comp = abr_read_char(abr);
				g_print(" | << size: %dx%d %d bit (%d bytes) %s\n",
					width, height, abr_sampled_brush_hdr.depth, size,
					comp ? "compressed" : "raw");
				if (!comp) {
					fread(gbr->data, size, 1, abr);
				} else {
					gint32 n;
					gchar ch;
					gint i, j, c;
					gshort *cscanline_len;
					gchar *data = gbr->data;

					/* read compressed size foreach scanline */
					cscanline_len = g_new0(gshort, height);
					for (i = 0; i < height; i++)
						cscanline_len[i] = abr_read_short(abr);
						
					/* unpack each scanline data */
					for (i = 0; i < height; i++) {
						for (j = 0; j < cscanline_len[i];) {
							n = abr_read_char(abr);
							j++;
							if (n >= 128)			/* force sign */
								n -= 256;
							if (n < 0) {			/* copy the following char -n + 1 times */
								if (n == -128)	/* it's a nop */
									continue;
								n = -n + 1;
								ch = abr_read_char(abr);
								j++;
								for (c = 0; c < n; c++, data++)
									*data = ch;
							} else {					/* read the following n + 1 chars (no compr) */
								for (c = 0; c < n + 1; c++, j++, data++)
									*data = (guchar)abr_read_char(abr);
							}
						}
					}
					g_free(cscanline_len);
				}
			}
		}
		break;
	default:
		g_print("WARNING: unknown brush type, skipping.\n");
		fseek(abr, abr_brush_hdr.size, SEEK_CUR);
	}

	return gbr;
}

void
gbr_brush_free(GbrBrush *brush)
{
	g_free(brush->name);
	g_free(brush->description);
	g_free(brush->data);
	g_free(brush);
}

void
abr_load(const gchar *file)
{
	FILE *abr;
	AbrHeader abr_hdr;
	GbrBrush *brush;
	gint i;

	if (!(abr = fopen(file, "rb"))) {
		g_print("ERROR: can't open '%s'\n", file);
		return;
	}

	abr_hdr.version = abr_read_short(abr);
	abr_hdr.count = abr_read_short(abr);

	g_print("FILE %s\n + HEADER\n | << ver: %d  brushes: %d\n",
		file, abr_hdr.version, abr_hdr.count);

	if (abr_hdr.version > 1) {
		g_print("ERROR: unable to decode abr format version %d\n", abr_hdr.version);
	} else {
		for (i = 0; i < abr_hdr.count; i++) {
			brush = abr_brush_load(abr);
			if (!brush) {
				g_print("ERROR: load error (brush #%d)\n", i);
				break;
			}
			gbr_brush_save(brush, file, i);
			gbr_brush_free(brush);
		}
	}
		
	fclose(abr);
}

void
gbr_brush_save(GbrBrush *brush, const gchar *file, const gint id)
{
	FILE *gbr;
	gchar *p;

	p = strrchr(file, '.');
	brush->name = g_strndup(file, p - file);
	//g_print("%s\n", brush->name);
	p = brush->name;
	brush->name = g_strdup_printf("%s_%03d.gbr", p, id);
	brush->description = g_strdup_printf("%s %03d", p, id);
	g_free(p);
	//g_print("%s\n", brush->name);
	//g_print("%s\n", brush->description);

	brush->header.header_size = g_htonl(sizeof(GbrBrushHeader) +
		strlen(brush->description) + 1);
	gbr = fopen(brush->name, "wb");
	fwrite(&brush->header, sizeof(GbrBrushHeader), 1, gbr);
	fwrite(brush->description, strlen(brush->description) + 1, 1, gbr);
	fwrite(brush->data, brush->size, 1, gbr);
	g_print(" | >> %s (%d bytes)\n", brush->name,
		brush->size + sizeof(GbrBrushHeader) + strlen(brush->description) + 1);
	fclose(gbr);
}


gint
main(gint argc, gchar *argv[])
{
	gint i;
	
	if (argc < 2) {
		g_print("usage: %s {file1.abr} ...\n", argv[0]);
		return -1;
	}
	
	for (i = 1; i < argc; i++)
		abr_load(argv[i]);
	g_print("DONE\n");

	return 0;
}
