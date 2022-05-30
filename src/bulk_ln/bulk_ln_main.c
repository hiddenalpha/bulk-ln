
#include "bulk_ln.h"

int main( int argc, char**argv ){
    int err = bulk_ln_main(argc, argv);
    /* Ensure to return 7-bit value only (See POSIX) */
    if( err < 0 ) err = -err;
    return err > 127 ? 1 : err;
}

