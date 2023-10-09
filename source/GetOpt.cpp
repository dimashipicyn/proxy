#include "GetOpt.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int opterr; /* if error message should be printed */
int optind; /* index into parent argv vector */
int optopt; /* character checked for validity */
int optreset; /* reset getopt */
const char* optarg; /* argument associated with option */

#define __P(x) x
#define _DIAGASSERT(x) assert(x)

static char* __progname __P((char*));
int getopt_internal __P((int, char* const*, const char*));

static char* __progname(char* nargv0)
{
    char* tmp;

    _DIAGASSERT(nargv0 != NULL);

    tmp = strrchr(nargv0, '/');
    if (tmp)
        tmp++;
    else
        tmp = nargv0;
    return (tmp);
}

#define BADCH (int)'?'
#define BADARG (int)':'
#define EMSG ""

/*
 * getopt --
 *	Parse argc/argv argument vector.
 */
int getopt_internal(int nargc, char* const* nargv, const char* ostr)
{
    static const char* place = EMSG; /* option letter processing */
    const char* oli = nullptr; /* option letter list index */

    _DIAGASSERT(nargv != NULL);
    _DIAGASSERT(ostr != NULL);

    if (optreset || !*place)
    { /* update scanning pointer */
        optreset = 0;
        if (optind >= nargc || *(place = nargv[optind]) != '-')
        {
            place = EMSG;
            return (-1);
        }
        if (place[1] && *++place == '-')
        { /* found "--" */
            /* ++optind; */
            place = EMSG;
            return (-2);
        }
    } /* option letter okay? */
    if ((optopt = (int)*place++) == (int)':' || !(oli = strchr(ostr, optopt)))
    {
        /*
         * if the user didn't specify '-' as an option,
         * assume it means -1.
         */
        if (optopt == (int)'-')
            return (-1);
        if (!*place)
            ++optind;
        if (opterr && *ostr != ':')
            (void)fprintf(stderr,
                "%s: illegal option -- %c\n", __progname(nargv[0]), optopt);
        return (BADCH);
    }
    if (*++oli != ':')
    { /* don't need argument */
        optarg = NULL;
        if (!*place)
            ++optind;
    }
    else
    { /* need an argument */
        if (*place) /* no white space */
            optarg = place;
        else if (nargc <= ++optind)
        { /* no arg */
            place = EMSG;
            if ((opterr) && (*ostr != ':'))
                (void)fprintf(stderr,
                    "%s: option requires an argument -- %c\n",
                    __progname(nargv[0]), optopt);
            return (BADARG);
        }
        else /* white space */
            optarg = nargv[optind];
        place = EMSG;
        ++optind;
    }
    return (optopt); /* dump back option letter */
}

#if 0
/*
 * getopt --
 *	Parse argc/argv argument vector.
 */
int
getopt2(nargc, nargv, ostr)
	int nargc;
	char * const *nargv;
	const char *ostr;
{
	int retval;

	if ((retval = getopt_internal(nargc, nargv, ostr)) == -2) {
		retval = -1;
		++optind; 
	}
	return(retval);
}
#endif

/*
 * getopt_long --
 *	Parse argc/argv argument vector.
 */
int getopt_long(int nargc, char** nargv, const char* options, struct option* long_options, int* index)
{
    int retval;

    _DIAGASSERT(nargv != NULL);
    _DIAGASSERT(options != NULL);
    _DIAGASSERT(long_options != NULL);
    /* index may be NULL */

    if ((retval = getopt_internal(nargc, nargv, options)) == -2)
    {
        char *current_argv = nargv[optind++] + 2, *has_equal;
        int i, current_argv_len, match = -1;

        if (*current_argv == '\0')
        {
            return (-1);
        }
        if ((has_equal = strchr(current_argv, '=')) != NULL)
        {
            current_argv_len = has_equal - current_argv;
            has_equal++;
        }
        else
            current_argv_len = strlen(current_argv);

        for (i = 0; long_options[i].name; i++)
        {
            if (strncmp(current_argv, long_options[i].name, current_argv_len))
                continue;

            if (strlen(long_options[i].name) == (unsigned)current_argv_len)
            {
                match = i;
                break;
            }
            if (match == -1)
                match = i;
        }
        if (match != -1)
        {
            if (long_options[match].has_arg == required_argument || long_options[match].has_arg == optional_argument)
            {
                if (has_equal)
                    optarg = has_equal;
                else
                    optarg = nargv[optind++];
            }
            if ((long_options[match].has_arg == required_argument)
                && (optarg == NULL))
            {
                /*
                 * Missing argument, leading :
                 * indicates no error should be generated
                 */
                if ((opterr) && (*options != ':'))
                    (void)fprintf(stderr,
                        "%s: option requires an argument -- %s\n",
                        __progname(nargv[0]), current_argv);
                return (BADARG);
            }
        }
        else
        { /* No matching argument */
            if ((opterr) && (*options != ':'))
                (void)fprintf(stderr,
                    "%s: illegal option -- %s\n", __progname(nargv[0]), current_argv);
            return (BADCH);
        }
        if (long_options[match].flag)
        {
            *long_options[match].flag = long_options[match].val;
            retval = 0;
        }
        else
            retval = long_options[match].val;
        if (index)
            *index = match;
    }
    return (retval);
}