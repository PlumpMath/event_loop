#include <windows.h>
#include <dbghelp.h>
#include <assert.h>
#include <stdio.h>

/* This implementation of backtrace(3) and backtrace_symbols(3) comes with a
 * number of limitations:
 *
 *    - The maxium number of stack frames is limited by the implementation of
 *      CaptureStackBackTrace() in Windows XP, which can only retrieve a
 *      maximum of 63 frames.
 *
 *    - All of the DbgHelp functions in Windows are not thread safe. This
 *      means that our implementation of the backtrace functions are also not
 *      thread safe.
 *
 *    - The frames returned by this implementation of backtrace(3) include
 *      those those in the Windows kernel. This means that kernel32.dll and
 *      ntdll.dll appear at the bottom end of the stack trace, and obviously
 *      you wouldn't see these on Linux.
 */

/* backtrace() returns a backtrace for the calling program, in the array
 * pointed to by buffer. A backtrace is the series of currently active
 * function calls for the program. Each item in the array pointed to by
 * buffer is of type void *, and is the return address from the corresponding
 * stack frame. The size argument specifies the maximum number of addresses
 * that can be stored in buffer. If the backtrace is larger than size, then
 * the addresses corresponding to the size most recent function calls are
 * returned; to obtain the complete backtrace, make sure that buffer and size
 * are large enough.
 */
int backtrace(void **buffer, int size)
{
    HANDLE process = GetCurrentProcess();
    const int xp_max_frame_depth = 61;
    if (size > xp_max_frame_depth)
        size = xp_max_frame_depth;

    /* Ignore this function when getting the stack frames. */
    SymInitialize(process, NULL, TRUE);
    return CaptureStackBackTrace(1, size, buffer, NULL);
}

/* Given the set of addresses returned by backtrace() in buffer,
 * backtrace_symbols() translates the addresses into an array of strings that
 * describe the addresses symbolically. The size argument specifies the
 * number of addresses in buffer. The symbolic representation of each address
 * consists of the function name (if this can be determined), a hexadecimal
 * offset into the function, and the actual return address (in hexadecimal).
 * The address of the array of string pointers is returned as the function
 * result of backtrace_symbols(). This array is malloc(3)ed by
 * backtrace_symbols(), and must be freed by the caller. (The strings pointed
 * to by the array of pointers need not and should not be freed.)
 */
char **backtrace_symbols(void *const *buffer, int size)
{
    HANDLE process = GetCurrentProcess();
    SYMBOL_INFO *symbol;
    IMAGEHLP_MODULE64 module_info;
    const int chars_needed_to_display_address = 16;
    const int additional_characters = 11;
    int chars_required = 0;
    int i;
    char **result;
    DWORD64 offset;
    char* frame_text;

    symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + (MAX_SYM_NAME + 1) * sizeof(char), 1);
    symbol->MaxNameLen = MAX_SYM_NAME;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    module_info.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

    /* The glibc documentation doen't say whether the strings we're preparing
     * should contain decorated or undecorated symbol names; it simply states
     * that it should be a 'printable representation' of the frame. The man
     * page doesn't offer any guidance either. We'll use undecorated symbols,
     * as these are probably more helpful.
     */
    SymSetOptions(SymGetOptions() | SYMOPT_UNDNAME);

    /* Compute the amount of memory required to hold the results. Each string
     * takes the following form:
     *
     *     ./prog(myfunc+0x21) [0x8048894]
     *
     * So we need the module name and the symbol name (which will vary in
     * length), two addresses (which we assume are 64 bits and therefore
     * require 16 characters each), ten other characters (brackets, spaces,
     * etc.) and a terminating NULL.
     *
     * We also need a string lookup table, which contains pointers to these
     * strings. The string lookup table comes first in memory, followed by
     * the strings themselves.
     */
    chars_required = size * sizeof(char*);
    for (i = 0; i < size; ++i)
    {
        if ((SymFromAddr(process, (DWORD64)(buffer[i]), 0, symbol)) &&
            (SymGetModuleInfo64(process, symbol->Address, &module_info)))
        {
            chars_required += strlen(symbol->Name) + strlen(module_info.LoadedImageName) +
                (2 * chars_needed_to_display_address) + additional_characters;
        }
    }

    /* Allocate enough memory to hold the strings and the string lookup
     * table. This memory buffer is returned once it has been populated, and
     * it is the responsibility of the caller to free(3) the memory.
     */
    result = (char**) malloc(chars_required);

    /* Now populate the string lookup table and the strings with the text
     * describing a frame. The pointer 'frame_text' is within the buffer we
     * have just allocated and points to the start of the next string to
     * write.
     */
    if (result)
    {
        frame_text = (char*) (result + size);
        for (i = 0; i < size; ++i)
        {
            result[i] = frame_text;

            if ((SymFromAddr(process, (DWORD64)(buffer[i]), &offset, symbol)) &&
                (SymGetModuleInfo64(process, symbol->Address, &module_info)))
            {
                frame_text += 1 + sprintf(frame_text, "%s(%s+0x%lx) [0x%lx]",
                    module_info.LoadedImageName, symbol->Name, (unsigned long)offset,
                    (unsigned long)buffer[i]);
            }
            else
                frame_text += 1 + sprintf(frame_text, "[0x%lx]", (unsigned long)buffer[i]);
        }
        assert(frame_text < (char*)result + chars_required);
    }

    free(symbol);
    return result;
}

/* backtrace_symbols_fd() takes the same buffer and size arguments as
 * backtrace_symbols(), but instead of returning an array of strings to the
 * caller, it writes the strings, one per line, to the file descriptor fd.
 * backtrace_symbols_fd() does not call malloc(3), and so can be employed in
 * situations where the latter function might fail.
 */
void backtrace_symbols_fd(void *const *buffer, int size, int fd)
{
    /* A Cygwin implementation of backtrace_symbols_fd(3) is going to be
     * somewhat problematic and will demand a compromise at some point along
     * the way. The problem is how to allocate memory for the SYMBOL_INFO
     * structure, given that we're not supposed to use heap memory. Windows
     * defines MAX_SYM_NAME as 2000 characters, and clearly we shouldn't go
     * trying to allocate that much memory on the stack.
     *
     * Then there's the issue of how we actually get the data out to the file
     * descriptor supplied. If Cygwin supports dprintf(3) then that's all
     * well and good - but if not we'll have to use write(2), and that
     * involves allocating another buffer to hold the text we want to write
     * out - and that means a second copy of our long symbol name on the
     * stack.
     *
     * Clearly a compromise is needed. Here are some options:
     *
     *    - Instead of MAX_SYM_NAME, use a much, much smaller value (say
     *      256). Then we could allocate all the memory we need on the stack
     *      and still display the majority of symbol names.
     *
     *    - Allocate heap memory anyway, irrespective of what the man page
     *      says. In environments where Cygwin is run (i.e. the Windows
     *      desktop), heap is probably plentiful, so does this matter?
     *
     *    - Do not provide backtrace_symbols_fd(3) at all.
     *
     * For the moment, I'll take the latter of those options.
     */
}

