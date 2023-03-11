/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#cmakedefine HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* libtool defines DLL_EXPORT for windows dll
   builds, but we use MP4V2_EXPORTS instead. */
#ifdef DLL_EXPORT
# define MP4V2_EXPORTS
#else
# define MP4V2_USE_STATIC_LIB
#endif

/* Define to 1 if LFS should be activated */
#cmakedefine NEED_LFS_ACTIVATION 1

/* Name of package */
#define PACKAGE "@PROJECT_name_lower@"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "@PROJECT_email@"

/* Define to the full name of this package. */
#define PACKAGE_NAME "@PROJECT_name@"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "@PROJECT_name_formal@"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "@PROJECT_name_lower@"

/* Define to the home page for this package. */
#define PACKAGE_URL "@PROJECT_url_website@"

/* Define to the version of this package. */
#define PACKAGE_VERSION "@PROJECT_version@"

/* Version number of package */
#define VERSION "@PROJECT_version@"
