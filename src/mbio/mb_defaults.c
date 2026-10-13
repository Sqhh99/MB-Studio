/*--------------------------------------------------------------------
 *    The MB-system:  mb_defaults.c  10/7/94
  *
 *    Copyright (c) 1993-2025 by
 *    David W. Caress (caress@mbari.org)
 *      Monterey Bay Aquarium Research Institute
 *      Moss Landing, California, USA
 *    Dale N. Chayes 
 *      Center for Coastal and Ocean Mapping
 *      University of New Hampshire
 *      Durham, New Hampshire, USA
 *    Christian dos Santos Ferreira
 *      MARUM
 *      University of Bremen
 *      Bremen Germany
 *     
 *    MB-System was created by Caress and Chayes in 1992 at the
 *      Lamont-Doherty Earth Observatory
 *      Columbia University
 *      Palisades, NY 10964
 *
 *    See README.md file for copying and redistribution conditions.
 *--------------------------------------------------------------------*/
/*
 * mb_defaults.c contains two functions - mb_defaults() and mb_env().
 * mb_defaults() returns the default MBIO control parameters and
 * mb_env() returns the default MB-System environment variables - all
 * values are read from ~/.mbio_defaults providing this file exists.
 * The return values are MB_SUCCESS if the file exists and MB_FAILURE
 * if it does not exist.
 *
 * Author:  D. W. Caress
 * Date:  January 23, 1993
 *
 *
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h> /* for GetComputerNameA() */
#else
#include <unistd.h>
#endif

#include "mb_define.h"
#include "mb_status.h"

/*--------------------------------------------------------------------*/
/* Run a shell command on behalf of an MB-System program (wrapper around
   system()). On Windows:
   - the directory containing the running program is put at the front of
     PATH, so that chained MB-System programs (mbinfo, mbsegyinfo, ...) and
     the bundled gmt.exe are found even if the installation is not on PATH;
   - the mbm_* plotting macros (Perl/shell scripts) are not available, so
     such commands are skipped with a notice instead of failing. */
int mb_system(const char *command) {
  if (command == NULL)
    return system(NULL);
#ifdef _WIN32
  const char *c = command;
  while (*c == ' ' || *c == '\t')
    c++;
  if (strncmp(c, "mbm_", 4) == 0) {
    fprintf(stderr, "\nNote: the MB-System plot macro command\n\t%s\n"
                    "was skipped (mbm_* macros are not available on Windows).\n", command);
    return 0;
  }
  static bool path_set = false;
  if (!path_set) {
    path_set = true;
    char exe[MB_PATH_MAXLINE];
    const DWORD n = GetModuleFileNameA(NULL, exe, sizeof(exe));
    if (n > 0 && n < sizeof(exe)) {
      char *sep = strrchr(exe, '\\');
      if (sep != NULL) {
        *sep = '\0';
        const char *path = getenv("PATH");
        const size_t len = strlen(exe) + (path != NULL ? strlen(path) : 0) + 2;
        char *newpath = (char *)malloc(len);
        if (newpath != NULL) {
          snprintf(newpath, len, "%s;%s", exe, path != NULL ? path : "");
          _putenv_s("PATH", newpath);
          free(newpath);
        }
      }
    }
  }
#endif
  return system(command);
}

/*--------------------------------------------------------------------*/
/* Copy a command line option argument (typically a file path) into dest,
   a buffer of size bytes. Unlike sscanf(optarg, "%s", dest) this keeps
   embedded spaces, so that paths such as "C:\\Users\\First Last\\line.mb88"
   or "/Volumes/Survey Data/line.mb88" work. Leading and trailing white space
   is removed and the result is always null terminated. Returns dest. */
char *mb_copy_arg(char *dest, const char *src, size_t size) {
  if (dest == NULL || size == 0)
    return dest;
  dest[0] = '\0';
  if (src == NULL)
    return dest;
  while (*src == ' ' || *src == '\t' || *src == '\n' || *src == '\r')
    src++;
  size_t len = strlen(src);
  while (len > 0 && (src[len - 1] == ' ' || src[len - 1] == '\t' || src[len - 1] == '\n' || src[len - 1] == '\r'))
    len--;
  if (len > size - 1)
    len = size - 1;
  memcpy(dest, src, len);
  dest[len] = '\0';
  return dest;
}

/*--------------------------------------------------------------------*/
/* Return the user's home directory, where MB-System keeps its defaults
   files (.mbio_defaults, ...). POSIX systems use $HOME. Windows does not set
   HOME by default, so there fall back to %USERPROFILE% and then to
   %HOMEDRIVE%%HOMEPATH%. Returns NULL if no home directory is known. */
const char *mb_get_home_dir(void) {
  const char *home = getenv("HOME");
  if (home != NULL && home[0] != '\0')
    return home;
#ifdef _WIN32
  home = getenv("USERPROFILE");
  if (home != NULL && home[0] != '\0')
    return home;
  static char home_buffer[MB_PATH_MAXLINE];
  const char *drive = getenv("HOMEDRIVE");
  const char *path = getenv("HOMEPATH");
  if (drive != NULL && path != NULL) {
    snprintf(home_buffer, sizeof(home_buffer), "%s%s", drive, path);
    return home_buffer;
  }
#endif
  return NULL;
}

/*--------------------------------------------------------------------*/
int mb_version(int verbose, char *version_string, int *version_id, int *version_major, 
                int *version_minor, int *version_archive, int *error) {
  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", __func__);
    fprintf(stderr, "dbg2  Input arguments:\n");
    fprintf(stderr, "dbg2       verbose:         %d\n", verbose);
  }

  /* get version string */
  strcpy(version_string, MB_VERSION);

  /* get version components */
  const int nscan = sscanf(version_string, "%d.%d.%d", version_major, version_minor, version_archive);
  if (nscan == 3) {
    *error = MB_ERROR_NO_ERROR;
    // 5.5.2303 ==> 50000000 + 500000 + 2303 ==>  50502303
    *version_id = 10000000 * (*version_major) + 100000 * (*version_minor) + *version_archive;
  }
  else {
    *error = MB_ERROR_UNINTELLIGIBLE;
    *version_id = 0;
    *version_major = 0;
    *version_minor = 0;
    *version_archive = 0;
  }

  const int status = MB_SUCCESS;

  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", __func__);
    fprintf(stderr, "dbg2  Return values:\n");
    fprintf(stderr, "dbg2       version_string:  %s\n", version_string);
    fprintf(stderr, "dbg2       version_id:      %d\n", *version_id);
    fprintf(stderr, "dbg2       version_major:   %d\n", *version_major);
    fprintf(stderr, "dbg2       version_minor:   %d\n", *version_minor);
    fprintf(stderr, "dbg2       version_archive: %d\n", *version_archive);
    fprintf(stderr, "dbg2       error:           %d\n", *error);
    fprintf(stderr, "dbg2  Return status:\n");
    fprintf(stderr, "dbg2       status:          %d\n", status);
  }

  return (status);
}
/*--------------------------------------------------------------------*/
int mb_user_host_date(int verbose, char user[256], char host[256], char date[32],
               int *error) {
  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", __func__);
    fprintf(stderr, "dbg2  Input arguments:\n");
    fprintf(stderr, "dbg2       verbose:    %d\n", verbose);
  }

  memset(user, 0, 256);
  char *user_ptr = NULL;
  char *unknown = "Unknown";
	if ((user_ptr = getenv("USER")) == NULL)
		if ((user_ptr = getenv("LOGNAME")) == NULL)
		  if ((user_ptr = getenv("USERNAME")) == NULL) /* Windows */
        user_ptr = unknown;
	strncpy(user, user_ptr, 255);

  memset(host, 0, 256);
#ifdef _WIN32
  /* gethostname() requires Winsock to be initialized with WSAStartup(), so
     ask Windows for the computer name directly */
  DWORD host_size = 255;
  if (!GetComputerNameA(host, &host_size))
    host[0] = '\0';
  if (host[0] == '\0') {
    const char *host_ptr = getenv("COMPUTERNAME");
    if (host_ptr == NULL)
      host_ptr = getenv("USERDOMAIN");
    if (host_ptr != NULL)
      strncpy(host, host_ptr, 255);
  }
#else
	gethostname(host, 255);
#endif

  memset(date, 0, 32);
	time_t right_now = time((time_t *)0);
	strncpy(date, ctime(&right_now), 31);
  date[strlen(date) - 1] = '\0'; // trim line return

  const int status = MB_SUCCESS;

  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", __func__);
    fprintf(stderr, "dbg2  Return values:\n");
    fprintf(stderr, "dbg2       user:       %s\n", user);
    fprintf(stderr, "dbg2       host:       %s\n", host);
    fprintf(stderr, "dbg2       date:       %s\n", date);
    fprintf(stderr, "dbg2       error:      %d\n", *error);
    fprintf(stderr, "dbg2  Return status:\n");
    fprintf(stderr, "dbg2       status:     %d\n", status);
  }

  return (status);
}
/*--------------------------------------------------------------------*/
int mb_default_defaults(int verbose, int *format, int *pings, int *lonflip,
                double bounds[4], int *btime_i, int *etime_i,
                double *speedmin, double *timegap) {
  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", __func__);
    fprintf(stderr, "dbg2  Input arguments:\n");
    fprintf(stderr, "dbg2       verbose: %d\n", verbose);
  }

  /* set system default values */
  *format = 0;
  *pings = 1;
  *lonflip = 0;
  bounds[0] = -360.;
  bounds[1] = 360.;
  bounds[2] = -90.;
  bounds[3] = 90.;
  btime_i[0] = 1962;
  btime_i[1] = 2;
  btime_i[2] = 21;
  btime_i[3] = 10;
  btime_i[4] = 30;
  btime_i[5] = 0;
  btime_i[6] = 0;
  etime_i[0] = 2062;
  etime_i[1] = 2;
  etime_i[2] = 21;
  etime_i[3] = 10;
  etime_i[4] = 30;
  etime_i[5] = 0;
  etime_i[6] = 0;
  *speedmin = 0.0;
  *timegap = 1.0;

  /* successful no matter what happens */
  const int status = MB_SUCCESS;

  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", __func__);
    fprintf(stderr, "dbg2  Return values:\n");
    fprintf(stderr, "dbg2       format:     %d\n", *format);
    fprintf(stderr, "dbg2       pings:      %d\n", *pings);
    fprintf(stderr, "dbg2       lonflip:    %d\n", *lonflip);
    fprintf(stderr, "dbg2       bounds[0]:  %f\n", bounds[0]);
    fprintf(stderr, "dbg2       bounds[1]:  %f\n", bounds[1]);
    fprintf(stderr, "dbg2       bounds[2]:  %f\n", bounds[2]);
    fprintf(stderr, "dbg2       bounds[3]:  %f\n", bounds[3]);
    fprintf(stderr, "dbg2       btime_i[0]: %d\n", btime_i[0]);
    fprintf(stderr, "dbg2       btime_i[1]: %d\n", btime_i[1]);
    fprintf(stderr, "dbg2       btime_i[2]: %d\n", btime_i[2]);
    fprintf(stderr, "dbg2       btime_i[3]: %d\n", btime_i[3]);
    fprintf(stderr, "dbg2       btime_i[4]: %d\n", btime_i[4]);
    fprintf(stderr, "dbg2       btime_i[5]: %d\n", btime_i[5]);
    fprintf(stderr, "dbg2       btime_i[6]: %d\n", btime_i[6]);
    fprintf(stderr, "dbg2       etime_i[0]: %d\n", etime_i[0]);
    fprintf(stderr, "dbg2       etime_i[1]: %d\n", etime_i[1]);
    fprintf(stderr, "dbg2       etime_i[2]: %d\n", etime_i[2]);
    fprintf(stderr, "dbg2       etime_i[3]: %d\n", etime_i[3]);
    fprintf(stderr, "dbg2       etime_i[4]: %d\n", etime_i[4]);
    fprintf(stderr, "dbg2       etime_i[5]: %d\n", etime_i[5]);
    fprintf(stderr, "dbg2       etime_i[6]: %d\n", etime_i[6]);
    fprintf(stderr, "dbg2       speedmin:   %f\n", *speedmin);
    fprintf(stderr, "dbg2       timegap:    %f\n", *timegap);
    fprintf(stderr, "dbg2  Return status:\n");
    fprintf(stderr, "dbg2       status:  %d\n", status);
  }

  return (status);
}
/*--------------------------------------------------------------------*/
int mb_defaults(int verbose, int *format, int *pings, int *lonflip,
                double bounds[4], int *btime_i, int *etime_i,
                double *speedmin, double *timegap) {
  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", __func__);
    fprintf(stderr, "dbg2  Input arguments:\n");
    fprintf(stderr, "dbg2       verbose: %d\n", verbose);
  }

  /* set system default values */
  int status = mb_default_defaults(verbose, format, pings, lonflip,
                  bounds, btime_i, etime_i, speedmin, timegap);

  /* set the filename */
  const char *home_ptr = mb_get_home_dir();
  if (home_ptr != NULL) {
    char file[MB_PATH_MAXLINE];
    strcpy(file, home_ptr);
    strcat(file, "/.mbio_defaults");

    /* open and read values from file if possible */
    FILE *fp = fopen(file, "r");
    if (fp != NULL) {
      char string[MB_PATH_MAXLINE];
      while (fgets(string, sizeof(string), fp) != NULL) {
        if (strncmp(string, "lonflip:", 8) == 0)
          sscanf(string, "lonflip: %d", lonflip);
        if (strncmp(string, "speed:", 6) == 0)
          sscanf(string, "timegap: %lf", timegap);
      }
      fclose(fp);
    }
  }

  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", __func__);
    fprintf(stderr, "dbg2  Return values:\n");
    fprintf(stderr, "dbg2       format:     %d\n", *format);
    fprintf(stderr, "dbg2       pings:      %d\n", *pings);
    fprintf(stderr, "dbg2       lonflip:    %d\n", *lonflip);
    fprintf(stderr, "dbg2       bounds[0]:  %f\n", bounds[0]);
    fprintf(stderr, "dbg2       bounds[1]:  %f\n", bounds[1]);
    fprintf(stderr, "dbg2       bounds[2]:  %f\n", bounds[2]);
    fprintf(stderr, "dbg2       bounds[3]:  %f\n", bounds[3]);
    fprintf(stderr, "dbg2       btime_i[0]: %d\n", btime_i[0]);
    fprintf(stderr, "dbg2       btime_i[1]: %d\n", btime_i[1]);
    fprintf(stderr, "dbg2       btime_i[2]: %d\n", btime_i[2]);
    fprintf(stderr, "dbg2       btime_i[3]: %d\n", btime_i[3]);
    fprintf(stderr, "dbg2       btime_i[4]: %d\n", btime_i[4]);
    fprintf(stderr, "dbg2       btime_i[5]: %d\n", btime_i[5]);
    fprintf(stderr, "dbg2       btime_i[6]: %d\n", btime_i[6]);
    fprintf(stderr, "dbg2       etime_i[0]: %d\n", etime_i[0]);
    fprintf(stderr, "dbg2       etime_i[1]: %d\n", etime_i[1]);
    fprintf(stderr, "dbg2       etime_i[2]: %d\n", etime_i[2]);
    fprintf(stderr, "dbg2       etime_i[3]: %d\n", etime_i[3]);
    fprintf(stderr, "dbg2       etime_i[4]: %d\n", etime_i[4]);
    fprintf(stderr, "dbg2       etime_i[5]: %d\n", etime_i[5]);
    fprintf(stderr, "dbg2       etime_i[6]: %d\n", etime_i[6]);
    fprintf(stderr, "dbg2       speedmin:   %f\n", *speedmin);
    fprintf(stderr, "dbg2       timegap:    %f\n", *timegap);
    fprintf(stderr, "dbg2  Return status:\n");
    fprintf(stderr, "dbg2       status:  %d\n", status);
  }

  return (status);
}
/*--------------------------------------------------------------------*/
int mb_env(int verbose, char *psdisplay, char *imgdisplay, char *mbproject) {
  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", __func__);
    fprintf(stderr, "dbg2  Input arguments:\n");
    fprintf(stderr, "dbg2       verbose: %d\n", verbose);
  }

/* set system default Postscript displayer */
/* for Linux, Darwin (Mac), OpenBSD, and Cygwin the system default programs
   as set by the user should be used - the value "Default" results in the
   appropriate program being used. On older Unix systems the relevant programs
   need to be directly referenced.
  If desired, users can set the default postscript and image screen viewers
  using mbdefaults. */
  strcpy(psdisplay, "Default");
  strcpy(imgdisplay, "Default");

  /* set system default project name */
  strcpy(mbproject, "none");

  /* set the filename */
  const char *home_ptr = mb_get_home_dir();
  if (home_ptr != NULL) {
    char file[MB_PATH_MAXLINE];
    strcpy(file, home_ptr);
    strcat(file, "/.mbio_defaults");

    /* open and read values from file if possible */
    FILE *fp = fopen(file, "r");
    if (fp != NULL) {
      char string[MB_PATH_MAXLINE];
      while (fgets(string, sizeof(string), fp) != NULL) {
        if (strncmp(string, "ps viewer:", 10) == 0)
          sscanf(string, "ps viewer: %s", psdisplay);
        if (strncmp(string, "img viewer:", 10) == 0)
          sscanf(string, "img viewer: %s", imgdisplay);
        if (strncmp(string, "project:", 8) == 0)
          sscanf(string, "project: %s", mbproject);
      }
      fclose(fp);
    }
  }

  /* successful no matter what happens */
  const int status = MB_SUCCESS;

  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", __func__);
    fprintf(stderr, "dbg2  Return values:\n");
    fprintf(stderr, "dbg2       psdisplay:  %s\n", psdisplay);
    fprintf(stderr, "dbg2       mbproject:  %s\n", mbproject);
    fprintf(stderr, "dbg2  Return status:\n");
    fprintf(stderr, "dbg2       status:  %d\n", status);
  }

  return (status);
}
/*--------------------------------------------------------------------*/
int mb_lonflip(int verbose, int *lonflip) {
  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", __func__);
    fprintf(stderr, "dbg2  Input arguments:\n");
    fprintf(stderr, "dbg2       verbose: %d\n", verbose);
  }

  /* set system default values */
  *lonflip = 0;

  /* set the filename */
  const char *home_ptr = mb_get_home_dir();
  if (home_ptr != NULL) {
    char file[MB_PATH_MAXLINE];
    strcpy(file, home_ptr);
    strcat(file, "/.mbio_defaults");

    /* open and read values from file if possible */
    FILE *fp = fopen(file, "r");
    if (fp != NULL) {
      char string[MB_PATH_MAXLINE];
      while (fgets(string, sizeof(string), fp) != NULL) {
        if (strncmp(string, "lonflip:", 8) == 0)
          sscanf(string, "lonflip: %d", lonflip);
      }
      fclose(fp);
    }
  }

  /* successful no matter what happens */
  int status = MB_SUCCESS;

  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", __func__);
    fprintf(stderr, "dbg2  Return values:\n");
    fprintf(stderr, "dbg2       lonflip:    %d\n", *lonflip);
    fprintf(stderr, "dbg2  Return status:\n");
    fprintf(stderr, "dbg2       status:  %d\n", status);
  }

  return (status);
}
/*--------------------------------------------------------------------*/
int mb_mbview_defaults(int verbose, int *primary_colortable, int *primary_colortable_mode, int *primary_shade_mode,
                       int *slope_colortable, int *slope_colortable_mode, int *secondary_colortable,
                       int *secondary_colortable_mode, double *illuminate_magnitude, double *illuminate_elevation,
                       double *illuminate_azimuth, double *slope_magnitude) {
  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", __func__);
    fprintf(stderr, "dbg2  Input arguments:\n");
    fprintf(stderr, "dbg2       verbose: %d\n", verbose);
  }

  /* set system default values */
  *primary_colortable = 0;
  *primary_colortable_mode = 0;
  *primary_shade_mode = 2;
  *slope_colortable = 0;
  *slope_colortable_mode = 0;
  *secondary_colortable = 0;
  *secondary_colortable_mode = 1;
  *illuminate_magnitude = 2.0;
  *illuminate_elevation = 5.0;
  *illuminate_azimuth = 90.0;
  *slope_magnitude = 1.0;

  /* set the filename */
  const char *home_ptr = mb_get_home_dir();
  if (home_ptr != NULL) {
    char file[MB_PATH_MAXLINE];
    strcpy(file, home_ptr);
    strcat(file, "/.mbio_defaults");

    /* open and read values from file if possible */
    FILE *fp = fopen(file, "r");
    if (fp != NULL) {
      char string[MB_PATH_MAXLINE];
      while (fgets(string, sizeof(string), fp) != NULL) {
        if (strncmp(string, "mbview_primary_colortable:", 25) == 0)
          sscanf(string, "mbview_primary_colortable:%d", primary_colortable);
        if (strncmp(string, "mbview_primary_colortable_mode:", 30) == 0)
          sscanf(string, "mbview_primary_colortable_mode:%d", primary_colortable_mode);
        if (strncmp(string, "mbview_primary_shade_mode:", 25) == 0)
          sscanf(string, "mbview_primary_shade_mode:%d", primary_shade_mode);
        if (strncmp(string, "mbview_slope_colortable:", 23) == 0)
          sscanf(string, "mbview_slope_colortable:%d", slope_colortable);
        if (strncmp(string, "mbview_slope_colortable_mode:", 28) == 0)
          sscanf(string, "mbview_slope_colortable_mode:%d", slope_colortable_mode);
        if (strncmp(string, "mbview_secondary_colortable:", 27) == 0)
          sscanf(string, "mbview_secondary_colortable:%d", secondary_colortable);
        if (strncmp(string, "mbview_secondary_colortable_mode:", 32) == 0)
          sscanf(string, "mbview_secondary_colortable_mode:%d", secondary_colortable_mode);
        if (strncmp(string, "mbview_illuminate_magnitude:", 27) == 0)
          sscanf(string, "mbview_illuminate_magnitude:%lf", illuminate_magnitude);
        if (strncmp(string, "mbview_illuminate_elevation:", 27) == 0)
          sscanf(string, "mbview_illuminate_elevation:%lf", illuminate_elevation);
        if (strncmp(string, "mbview_illuminate_azimuth:", 25) == 0)
          sscanf(string, "mbview_illuminate_azimuth:%lf", illuminate_azimuth);
        if (strncmp(string, "mbview_slope_magnitude:", 22) == 0)
          sscanf(string, "mbview_slope_magnitude:%lf", slope_magnitude);
      }
      fclose(fp);
    }
  }

  /* successful no matter what happens */
  int status = MB_SUCCESS;

  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", __func__);
    fprintf(stderr, "dbg2  Return values:\n");
    fprintf(stderr, "dbg2       primary_colortable:         %d\n", *primary_colortable);
    fprintf(stderr, "dbg2       primary_colortable_mode:    %d\n", *primary_colortable_mode);
    fprintf(stderr, "dbg2       primary_shade_mode:         %d\n", *primary_shade_mode);
    fprintf(stderr, "dbg2       slope_colortable:           %d\n", *slope_colortable);
    fprintf(stderr, "dbg2       slope_colortable_mode:      %d\n", *slope_colortable_mode);
    fprintf(stderr, "dbg2       secondary_colortable:       %d\n", *secondary_colortable);
    fprintf(stderr, "dbg2       secondary_colortable_mode:  %d\n", *secondary_colortable_mode);
    fprintf(stderr, "dbg2       illuminate_magnitude:       %f\n", *illuminate_magnitude);
    fprintf(stderr, "dbg2       illuminate_elevation:       %f\n", *illuminate_elevation);
    fprintf(stderr, "dbg2       illuminate_azimuth:         %f\n", *illuminate_azimuth);
    fprintf(stderr, "dbg2       slope_magnitude:            %f\n", *slope_magnitude);
    fprintf(stderr, "dbg2  Return status:\n");
    fprintf(stderr, "dbg2       status:  %d\n", status);
  }

  return (status);
}
/*--------------------------------------------------------------------*/
int mb_fbtversion(int verbose, int *fbtversion) {
  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", __func__);
    fprintf(stderr, "dbg2  Input arguments:\n");
    fprintf(stderr, "dbg2       verbose: %d\n", verbose);
  }

  /* set system default values */
  *fbtversion = 3;

  /* set the filename */
  const char *home_ptr = mb_get_home_dir();
  if (home_ptr != NULL) {
    char file[MB_PATH_MAXLINE];
    strcpy(file, home_ptr);
    strcat(file, "/.mbio_defaults");

    /* open and read values from file if possible */
    FILE *fp = fopen(file, "r");
    if (fp != NULL) {
                  char string[MB_PATH_MAXLINE];
      while (fgets(string, sizeof(string), fp) != NULL) {
        if (strncmp(string, "fbtversion:", 11) == 0)
          sscanf(string, "fbtversion: %d", fbtversion);
      }
      fclose(fp);
    }
  }

  /* successful no matter what happens */
  const int status = MB_SUCCESS;

  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", __func__);
    fprintf(stderr, "dbg2  Return values:\n");
    fprintf(stderr, "dbg2       fbtversion: %d\n", *fbtversion);
    fprintf(stderr, "dbg2  Return status:\n");
    fprintf(stderr, "dbg2       status:     %d\n", status);
  }

  return (status);
}
/*--------------------------------------------------------------------*/
int mb_uselockfiles(int verbose, bool *uselockfiles) {
  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", __func__);
    fprintf(stderr, "dbg2  Input arguments:\n");
    fprintf(stderr, "dbg2       verbose: %d\n", verbose);
  }

#ifdef WIN32
  /* It would crash because the lock file is attempted to be created with the new "wx"
     that VC12 does not know and we don't have any use for this anyway.
  */
  *uselockfiles = 0;
  return (MB_SUCCESS);
#else  /* WIN32 */

  /* set system default values */
  *uselockfiles = true;

  /* set the filename */
  const char *home_ptr = mb_get_home_dir();
  if (home_ptr != NULL) {
    char file[MB_PATH_MAXLINE];
    strcpy(file, home_ptr);
    strcat(file, "/.mbio_defaults");

    /* open and read values from file if possible */
    FILE *fp = fopen(file, "r");
    if (fp != NULL) {
      mb_path string = "";
      int uselockfilesint = 0;
      while (fgets(string, sizeof(string), fp) != NULL) {
        if (strncmp(string, "uselockfiles:", 13) == 0)
          sscanf(string, "uselockfiles:%d", &uselockfilesint);
        if (uselockfilesint < 0 || uselockfilesint >= 1)
          *uselockfiles = true;
        else
          *uselockfiles = false;
      }
      fclose(fp);
    }
  }

  /* successful no matter what happens */
  const int status = MB_SUCCESS;

  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", __func__);
    fprintf(stderr, "dbg2  Return values:\n");
    fprintf(stderr, "dbg2       uselockfiles: %d\n", *uselockfiles);
    fprintf(stderr, "dbg2  Return status:\n");
    fprintf(stderr, "dbg2       status:       %d\n", status);
  }

  return (status);
#endif  /* WIN32 */
}
/*--------------------------------------------------------------------*/
int mb_fileiobuffer(int verbose, int *fileiobuffer) {
  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", __func__);
    fprintf(stderr, "dbg2  Input arguments:\n");
    fprintf(stderr, "dbg2       verbose: %d\n", verbose);
  }

  /* set system default values */
  *fileiobuffer = 0;

  /* set the filename */
  const char *home_ptr = mb_get_home_dir();
  if (home_ptr != NULL) {
    char file[MB_PATH_MAXLINE];
    strcpy(file, home_ptr);
    strcat(file, "/.mbio_defaults");

    /* open and read values from file if possible */
    FILE *fp = fopen(file, "r");
    if (fp != NULL) {
      char string[MB_PATH_MAXLINE];
      while (fgets(string, sizeof(string), fp) != NULL) {
        if (strncmp(string, "fileiobuffer:", 13) == 0)
          sscanf(string, "fileiobuffer:%d", fileiobuffer);
      }
      fclose(fp);
    }
  }

  /* successful no matter what happens */
  const int status = MB_SUCCESS;

  if (verbose >= 2) {
    fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", __func__);
    fprintf(stderr, "dbg2  Return values:\n");
    fprintf(stderr, "dbg2       fileiobuffer: %d\n", *fileiobuffer);
    fprintf(stderr, "dbg2  Return status:\n");
    fprintf(stderr, "dbg2       status:       %d\n", status);
  }

  return (status);
}
/*--------------------------------------------------------------------*/
