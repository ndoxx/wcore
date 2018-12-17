#include <execinfo.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <iostream>

#include "stack_trace.h"

namespace wcore
{
#ifdef __linux__
void print_backtrace()
{
    void* bt[1024];
    int bt_size = backtrace(bt, 1024);
    char** bt_syms = backtrace_symbols(bt, bt_size);

    std::cout << "BACKTRACE ------------" << std::endl;
    for(int ii=1; ii<bt_size; ++ii)
    {
        /*Dl_info  DlInfo;
        if (dladdr(bt[ii], &DlInfo) != 0)
            std::cout << DlInfo.dli_sname << std::endl;*/
            //full_write(STDERR_FILENO, DlInfo.dli_sname, strlen(DlInfo.dli_sname));

        std::cout << bt_syms[ii] << std::endl;
    }
    std::cout << "----------------------" << std::endl;
    free(bt_syms);
}
#endif

} // namespace wcore
