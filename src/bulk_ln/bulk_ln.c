
/* This */
#include "bulk_ln.h"

/* System */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


#define DATA_FILE_FIELD_SEP_CHR '\t'

typedef  int  bool;
typedef  struct BulkLn  BulkLn;


/* Root context */
struct BulkLn {

    /** File (path) where we read the paths-to-link from. The special
     * value - (dash) means to use the stdin stream. */
    char *dataFilePath;

    FILE *dataFd;

    /** Count of links we created */
    int createdLinksCount;

    /** Count of direcrories we created */
    int createdDirsCount;

    /** If a dry-run got requested */
    bool dryRun;

    /** If true, we will print to stdout for every link we create (like we do
     * in dry run) */
    bool isPrintEachCreateLink;

    /** print every call we make to mkdir to stdout. Usually only useful for
     * debugging */
    bool isPrintEachMkdir;

    /** if true, we periodically print some kind of progress to stderr */
    bool isPrintStatus;

    /** if true, we print a summary what we did at the end of the run to stderr */
    bool isPrintSummary;

    /** if true, we will override existing files. This will be enabled for
     * example if --force got specified. */
    bool isRelinkExistingFiles;
};



static void printHelp(){
    printf("  \n   %s @ " STR_QUOT(PROJECT_VERSION) "\n"
        "  \n"
        "  Takes paths (pairwise) from stdin (see --stdin for details) and\n"
        "  creates a hardlink for each pair from the 1st path to the 2nd.\n"
        "  \n"
        "  Writing a custom implementation of 'ln' got necessary as we found no\n"
        "  way to instruct 'ln' to create a few thousand links in an acceptable\n"
        "  amount of time. Remind that wheels that do not fit should be\n"
        "  re-invented (yes, no matter how much they already exist!). So now we\n"
        "  have it. Our re-invented wheel that finally fits our use-case ;) \n"
        "  \n"
        "  See also\n"
        "  https://pubs.opengroup.org/onlinepubs/009695399/utilities/ln.html\n"
        "  for info about the original 'ln'.\n"
        "  \n"
        "  Options:\n"
        "  \n"
        "    --stdin\n"
        "        Read the path pairs to link from stdin. The format is like:\n"
        "  \n"
        "          <src-path> <tab> <dst-path> <newline>\n"
        "  \n"
        "        Example:\n"
        "  \n"
        "          origin/foo.txt\tnew/gugg.txt\n"
        "          origin/bar.txt\tnew/da.txt\n"
        "  \n"
        "        HINT: Preferred <newline> is LF. But CRLF should work too.\n"
        "  \n"
        "    --quiet\n"
        "        Don't print status or similar stuff. Errors will still be\n"
        "        printed to stderr.\n"
        "  \n"
        "    --verbose\n"
        "        Print stupid amount of logs. Usually only helpful for debugging.\n"
        "        Should NOT be combined with --quiet as this would be nonsense\n"
        "        anyway.\n"
        "  \n"
        "    --dry-run\n"
        "        Will print the actions to stdout instead executing them.\n"
        "        HINT: Directory count in summary will be inaccurate in this\n"
        "        mode.\n"
        "  \n"
        "    --force\n"
        "        Same meaning as in original 'ln' command.\n"
        "  \n", strrchr(__FILE__,'/')+1 );
}


/** returns non-zero on errors. Error messages will already be printed
 * internally. */
static int parseArgs( int argc, char**argv, BulkLn*bulkLn ){
    /* init (aka set defaults) */
    bulkLn->dataFilePath = NULL;
    bulkLn->dryRun = 0;
    bulkLn->isPrintEachCreateLink = 0;
    bulkLn->isPrintEachMkdir = 0;
    bulkLn->isPrintStatus = !0;
    bulkLn->isPrintSummary = !0;
    bulkLn->isRelinkExistingFiles = 0;

    // Parse args
    for( int i = 1 ; i < argc ; ++i ){
        char *arg = argv[i];
        if( !strcmp(arg, "--help") ){
            printHelp();
            return -1;
        }
        else if( !strcmp(arg, "--dry-run") ){
            bulkLn->dryRun = !0;
        }
        else if( !strcmp(arg, "--force") ){
            bulkLn->isRelinkExistingFiles = !0;
        }
        else if( !strcmp(arg, "--quiet") ){
            bulkLn->isPrintStatus = 0;
            bulkLn->isPrintSummary = 0;
        }
        else if( !strcmp(arg, "--stdin") ){
            bulkLn->dataFilePath = "-";
        }
        else if( !strcmp(arg, "--verbose") ){
            bulkLn->isPrintStatus = !0;
            bulkLn->isPrintSummary = !0;
            bulkLn->isPrintEachCreateLink = !0;
            bulkLn->isPrintEachMkdir = !0;
        }
        else{
            fprintf(stderr, "EINVAL: '%s'\n", arg);
            return -1;
        }
    }

    /* MUST specify input method. Yes there is only one input method. But
     * requiring args is the simplest way to prevent damage in case someone (eg
     * accidentally) invokes the utility wihout args. Further this also makes
     * the utility easier to extend wihout breaking everything. */
    if( bulkLn->dataFilePath == NULL ){
        fprintf(stderr, "EINVAL: '--stdin' missing.\n");
        return -1;
    }

    return 0;
}


/**
 * Like:  mkdirs -p path
 *
 * WARN: Passed 'path' might be temporarily changed during execution.
 * Nevertheless will be in original state after return.
 */
static int mkdirs( char*path, BulkLn*bulkLn ){
    int err;
    char *tmpEnd = path;
    /* Backup original length so we later still can recognize the original end
     * as we are going to place zeroes in the path during the run. */
    char *pathEnd = path + strlen(path);

    for(;;){
        /* Loop for each slash in the path beginning from the topmost dir toward the
         * innermost dir.
         * This way our path gets longer by one segment each time we call 'mkdir' below.
         * This is to have the same effect as we would call 'mkdir --parents' from cli.
         * In other words we make sure every parent dir exists before creating the next
         * one */

        tmpEnd = strchr(tmpEnd + 1, '/');
        if( tmpEnd == NULL ){
            /* The last (innermost) segment to create a dir for */
            tmpEnd = pathEnd;
        }

        /* Temporarily zero-terminate the path so we can create the parent dir
         * up to there */
        tmpEnd[0] = '\0';

        /* Print if requested */
        if( bulkLn->dryRun || bulkLn->isPrintEachMkdir ){
            printf("mkdir(\"%s\")\n", path);
        }

        /* Create dir up to that found path */
        if( ! bulkLn->dryRun ){
            /* Perform the real action */
            /* mode gets masked by umask and after that we have the defaults
             * (aka what we want) */
            err = mkdir(path, 0777);
            if( err ){
                if( errno == EEXIST ){
                    /* Fine :) So just continue with the next one. */
                }else{
                    fprintf(stderr, "mkdir(\"%s\"): %s\n", path, strerror(errno));
                    err = -1; goto finally;
                }
            }else{
                /* Only increment the directory counter if we really created
                 * the dir. Eg if it did NOT already exist */
                bulkLn->createdDirsCount += 1;
            }
        }else{
            /* We cannot really count how many dirs we created. Because maybe
             * we are (hypothetically) creating this directory the 100th time
             * now. But we have no way to tell as we do not really create em.
             * So just increment to have some value at all */
            bulkLn->createdDirsCount += 1;
        }

        if( tmpEnd == pathEnd ){
            /* Nothing to restore as we point to end-of-string.
             * This also means we're done. So end the loop */
            break;
        }else{
            /* Restore where we did cut-off the path */
            tmpEnd[0] = '/';
            /* Then loop to add and process one more segment */
        }
    }

    err = 0;
finally:
    return err;
}


/** Like:  ln srcPath dstPath  */
static int createHardlink( char*srcPath, char*dstPath, BulkLn*bulkLn ){
    int err;

    if( bulkLn->dryRun || bulkLn->isPrintEachCreateLink ){
        printf("link(\"%s\", \"%s\")\n", srcPath, dstPath);
    }

    if( ! bulkLn->dryRun ){
        /* Perform the real action */
        if( bulkLn->isRelinkExistingFiles ){
            /* Delete beforehand so we can be sure 'link' does not fail due
             * already existing file */
            err = unlink(dstPath);
            if( err ){
                if( errno == ENOENT ){
                    /* There is no such entry we could delete. So we're already
                     * fine :) */
                }else{
                    /* Some other (unepxected) error */
                    fprintf(stderr, "unlink(%s): %s\n", dstPath, strerror(errno));
                    err = -1; goto finally;
                }
            }
        }
        err = link(srcPath, dstPath);
        if( err ){
            fprintf(stderr, "link(\"%s\", \"%s\"): %s\n", srcPath, dstPath, strerror(errno));
            err = -1; goto finally;
        }
    }

    bulkLn->createdLinksCount += 1;

    err = 0;
finally:
    return err;
}


static int onPathPair( char*srcPath, char*dstPath, BulkLn*bulkLn ){
    assert(srcPath != NULL);
    assert(dstPath != NULL);
    assert(bulkLn != NULL);
    int err;

    if( bulkLn->isPrintStatus && bulkLn->createdLinksCount % 10000 == 0 ){
        fprintf(stderr, "Created %7d links so far.\n", bulkLn->createdLinksCount);
    }

    /* Search end of parent dir path */
    char *tmpEnd = strrchr(dstPath, '/');
    if( tmpEnd != NULL ){
        /* Temporarily cut-off the last segment (filename) to create the
         * parent-dirs */
        tmpEnd[0] = '\0';
        /* Create missing parent dirs */
        err = mkdirs(dstPath, bulkLn);
        /* Restore path */
        tmpEnd[0] = '/';
        if( err ){ err = -1; goto finally; }
    }

    err = createHardlink(srcPath, dstPath, bulkLn);
    if( err ){ err = -1; goto finally; }

    err = 0;
finally:
    return err;
}


static int parseDataFileAsPairPerLine( BulkLn*bulkLn ){
    int err;
    size_t buf_cap = 0;
    size_t buf_len = 0;
    char *buf = NULL;
    size_t lineNum = 0;

    for(;;){
        lineNum += 1;

        /* Read input line-by-line. Not the most elegant way to parse stuff,
         * but should suffice for our use-case */
        err = getline(&buf, &buf_cap, bulkLn->dataFd);
        if(unlikely( err < 0 )){
            /* Error handling */
            if( feof(bulkLn->dataFd) ){
                break; /* End-Of-File. Just break off the loop */
            }else if( ferror(bulkLn->dataFd) ){
                fprintf(stderr, "getline(%s): %s\n", bulkLn->dataFilePath, strerror(errno));
                err = -1; goto finally;
            }else{
                abort(); /* I don't know how this could happen */
            }
        }
        buf_len = err;

        /* Extract the two paths from our line */
        char *srcPath = buf;
        char *tab = memchr(buf, DATA_FILE_FIELD_SEP_CHR, buf_len);
        if( tab == NULL ){
            fprintf(stderr, "Too few field separators (tab) in '%s' @ %lu\n",
                    bulkLn->dataFilePath, lineNum);
            err = -1; goto finally;
        }
        char *unwantedTab = memchr(tab + 1, DATA_FILE_FIELD_SEP_CHR, buf_len - (tab + 1 - buf));
        if( unwantedTab != NULL ){
            fprintf(stderr, "Too many field separators (tab) in '%s' line %lu\n",
                    bulkLn->dataFilePath, lineNum);
            err = -1; goto finally;
        }
        char *dstPath = tab + 1; /* <- path starts one char after the separator */
        char *dstPath_end = buf + buf_len;
        for(;; --dstPath_end ){
            if(unlikely( dstPath_end < buf )){
                fprintf(stderr, "IMHO cannot happen  %s:%d\n", __FILE__, __LINE__);
                err = -1; goto finally;
            }
            if( dstPath_end[0]=='\n' || dstPath_end[0]=='\0' || dstPath_end[0]=='\r' ){
                continue; /* last char not found yet */
            }
            /* 'dstPath_end' now points to the last char of our line. So add
             * one to point to the 'end' */
            dstPath_end += 1;
            break;
        }

        /* Zero-Terminate the two strings */
        tab[0] = '\0';
        dstPath_end[0] = '\0';

        /* Publish this pair for processing */
        err = onPathPair(srcPath, dstPath, bulkLn);
        if( err ){ err = -1; goto finally; }
    }

    if( bulkLn->isPrintStatus ){
        fprintf(stderr, "Parsed %lu records from '%s'\n", lineNum - 1, bulkLn->dataFilePath);
    }

    err = 0;
finally:
    free(buf);
    return err;
}


int bulk_ln_main( int argc, char**argv ){
    int err;
    BulkLn bulkLn = {0};
    #define bulkLn (&bulkLn)

    err = parseArgs(argc, argv, bulkLn);
    if( err ){ err = -1; goto finally; }

    /* Open data source */
    if( !strcmp(bulkLn->dataFilePath, "-") ){
        bulkLn->dataFd = stdin;
    }else{
        bulkLn->dataFd = fopen(bulkLn->dataFilePath, "rb");
        if( bulkLn->dataFd == NULL ){
            fprintf(stderr, "fopen(%s): %s\n", bulkLn->dataFilePath, strerror(errno));
            err = -1; goto finally;
        }
    }

    err = parseDataFileAsPairPerLine(bulkLn);
    if( err ){ err = -1; goto finally; }

    if( bulkLn->isPrintSummary ){
        fprintf(stderr, "Created %d directories and linked %d files.\n",
                bulkLn->createdDirsCount, bulkLn->createdLinksCount);
    }

    err = 0;
finally:
    if( bulkLn->dataFd != NULL && bulkLn->dataFd != stdin ){
        fclose(bulkLn->dataFd);
    }
    return -err;
    #undef bulkLn
}

