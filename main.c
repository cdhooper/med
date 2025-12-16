#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef AMIGA
#ifdef _DCC
#include "amiga_stdint.h"
#endif
#include <exec/types.h>
#include "cpu_control.h"
#else
#include <unistd.h>
#include <err.h>
#endif
#include "cmdline.h"

int
main(int argc, char **argv)
{
    char *cmdbuf = cmd_string_from_argv(argc - 1, argv + 1);
#ifdef AMIGAOS
    cpu_control_init();
#endif

    if (cmdbuf == NULL) {
        exit(cmdline());
    } else {
        int rc = cmd_exec_string(cmdbuf);
        if (cmdbuf != NULL)
            free(cmdbuf);
        exit(rc);
    }
}
