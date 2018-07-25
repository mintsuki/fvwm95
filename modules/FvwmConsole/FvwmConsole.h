#ifdef ISC
#include <sys/bsdtypes.h> 
#endif 


#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <signal.h>

#include <string.h>

#include <FVWMconfig.h>
#include <fvwm/version.h>
#include <fvwm/fvwmlib.h>     
#include "../../fvwm/module.h"



#define S_NAME "/tmp/FvConSocket"
#define BUFSIZE 511   /* maximum error message size */
#define NEWLINE "\n"

/* timeout for packet after issuing command*/
#define TIMEOUT 400000

/* #define M_PASS M_ERROR */

#define M_PASS M_ERROR

/* number of default arguments when invoked from fvwm */
#define FARGS 6   

#define XTERM "xterm"
#define FCARGS 4
