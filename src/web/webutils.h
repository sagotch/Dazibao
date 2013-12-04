#ifndef _WEBUTILS_H
#define _WEBUTILS_H 1

/** @file
 * Utilities for the Web server
 **/

#include "utils.h"
#include <time.h>

/* = Global server properties = */

/**
 * A global struct used to store server properties
 **/
struct wserver_info {
        /** port the server is listening on **/
        int port;
        /** debug mode (used to serve Pad1s and PadNs) */
        char debug;
        /** hostname of the server */
        char *hostname;
        /** current Dazibao prettified name */
        char *dzname;
        /** path of the current Dazibao */
        char *dzpath;
        /** name of the server */
        char *name;
} WSERVER;

/* = I/O = */

#define MAX_FILE_PATH_LENGTH 256

/**
 * Wrapper around write(2) to write the whole buffer instead of (sometimes)
 * only a part of it.
 **/
int write_all(int fd, char *buff, int len);

/* = Other helpers = */

#define JPEG_EXT ".jpg"
#define PNG_EXT  ".png"
#define DEFAULT_EXT ""

/**
 * Guess the type of an image TLV from its path. If it ends with .png, it's a
 * TLV_PNG, if it ends with .jpg it's a TLV_JPEG. The function returns -1 if
 * the TLV type cannot be found.
 **/
int get_image_tlv_type(const char *path);

/**
 * Return a string representing a given GMT date. 'secs' is the number of
 * seconds since the Epoch, or -2 if you want the current date.
 **/
char *gmtdate(time_t secs);

#endif
