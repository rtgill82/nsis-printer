/* KJD 2004, public domain,
 * NSIS plugin wrapper for testing plugins...
 */


#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
// #include "nsis_tchar.h"


/* specifies the maximum size of a string pushed,
   popped, passed, etc.  set to match config.h
   or whatever for testing.  NSIS v2 default is 1024
   */
#define NSIS_MAX_STRLEN 1024


/* stack element, ie parameter to plugin, etc */
typedef struct stack_t 
{
    struct stack_t *next;
    TCHAR text[NSIS_MAX_STRLEN];
} stack_t;


/* returns true if stack is empty, assumes initialized and valid stack */
BOOL empty(stack_t **stack)
{
    if ((stack == NULL) || (*stack == NULL))
        return TRUE;
    else
        return FALSE;
}


/* push an element onto the specified stack
   in a manner compatible with how NSIS pushes them
   copies str onto top of stack
   if bottom is true, then assumes this is the 
   1st element pushed onto stack and inits underlying
   stack structure accordingly.
   */
void pushstring(stack_t **stack, LPTSTR str, BOOL bottom)
{
    stack_t *element;

    /* verify a valid stack (at least nonNULL) was provided */
    if (stack == NULL)
    {
        printf("Error, passed NULL stack, unable to push [%S]\n", str);
        exit(5);
    }

    /* allocate memory element, must match method plugins expect */
    element = (stack_t *)GlobalAlloc(GPTR /*fixed & zeroed*/, sizeof(stack_t));
    if (element == NULL)
    {
        printf("Error, unable to allocate memory to push [%S] onto stack\n",
                str);
        exit(4);
    }

    /* copy string to stack */
    lstrcpyn(element->text, str, NSIS_MAX_STRLEN);

    /* update stack to actually 'push' this item onto top of it */
    if (bottom) element->next = NULL;
    else element->next = *stack;

    *stack = element;
}


/* pop an element off the specified stack
   in a manner compatible with how NSIS pops them
   copies top of stack to str
   */
void popstring(stack_t **stack, LPTSTR str)
{
    stack_t *element;;

    /* verify a valid stack (at least nonNULL) was provided */
    if (empty(stack))
    {
        printf("Warning, attempt to pop from an empty stack!\n");
        return;
    }
    element = *stack;

    /* copy text from top of stack to str */
    lstrcpyn(str, element->text, NSIS_MAX_STRLEN);

    /* mark new stack top */
    *stack = element->next;

    /* free memory in manner matching how NSIS plugins expect allocated */
    GlobalFree((HGLOBAL)element);
}


/* standard user variables,
   the 1st 0-9 are accessed as $# in NSIS (e.g. $0 )
   then 10-19 are R0-R9 accessed as $R# in NSIS (e.g. $R0 )
   then 20-24 are $CMDLINE, $INSTDIR, $OUTDIR, $EXEDIR, $LANGUAGE
   */
#define MAXUSERVARS 25


/* sets indicated user variable, i.e.
   copies str to user_vars[which] 
   prints warning if which is invalid and returns
   */
void setuservar(LPTSTR user_vars, int which, LPCTSTR str)
{
    LPTSTR val = NULL;
    if ((which < 0) || (which >= MAXUSERVARS))
    {
        printf("Warning attempting to set invalid user_var[%i] with %S\n",
                which, str);
        return;
    }

    /* array is stored in a flat char buffer */
    val = user_vars + (which * NSIS_MAX_STRLEN);

    /* actually copy the string to the variable */
    lstrcpyn(val, str, NSIS_MAX_STRLEN);
}


/* returns a pointer to the indicated user variable,
   warning, pointer returned should be treated as 
   static buffer and copied to local buffer; contents
   may change via other calls to setuservar or by
   directly manipulating the string returned.
   returns NULL on error, such as invalid user var requested.
   */
LPCTSTR getuservar(LPTSTR user_vars, int which)
{
    LPCTSTR val = NULL;

    if ((which < 0) || (which >= MAXUSERVARS))
    {
        printf("Warning attempting to get invalid user_var[%i]\n", which);
        return NULL;
    }

    /* array is stored in a flat char buffer */
    val = user_vars + (which * NSIS_MAX_STRLEN);

    return val;
}


/* initializes default values
   presently just clears to 0, 
   */
void initpredefvars(LPTSTR user_vars)
{
    int i;
    /* simply set $0-$9 and $R0-$R10 to 0 */
    for (i = 0; i < 20; i++)
        setuservar(user_vars, i, _T("0"));

    /* set others to some reasonable value */
    setuservar(user_vars, 20, _T(""));     /* empty command line */
    setuservar(user_vars, 21, _T(".\\"));  /* $INSTDIR = current ??? */
    setuservar(user_vars, 22, _T(".\\"));  /* $OUTDIR */
    setuservar(user_vars, 23, _T(".\\"));  /* $EXEDIR */
    setuservar(user_vars, 24, _T("1033")); /* $LANGUAGE=english */
}


/* The exported API without name mangling */
typedef void (*pluginFunc)
    (HWND hwndParent, int string_size, LPTSTR variables, stack_t **stacktop);


    /* returns pointer to function in plugin or exits with message on any error */
pluginFunc getPluginFunction(LPCTSTR plugin, LPCTSTR exportedName)
{
    pluginFunc pFn = NULL;
    char *_exportedName;
    size_t len;

    HMODULE hMod = LoadLibrary(plugin);
    if (hMod == NULL)
    {
        printf("Failed to load %S\n", plugin);
        exit(3);
    }

    wcstombs_s(&len, NULL, 0, exportedName, 0);
    _exportedName = GlobalAlloc(GPTR, (len + 1) * sizeof(char));
    wcstombs_s(&len, _exportedName, len + 1, exportedName, len);
    pFn = (pluginFunc) GetProcAddress(hMod, _exportedName);
    if (pFn == NULL)
    {
        printf("Failed to obtain address of function %S in module %S\n",
                exportedName, plugin);
        exit(2);
    }

    GlobalFree(_exportedName);
    return pFn;
}


/* displays the full contents of the stack and user variables */
void showstuff(LPTSTR variables, stack_t **stacktop)
{
    int i;
    stack_t *element;

    printf("User variables are:\n");
    for (i = 0; i < 10; i++)
        printf("$%i = [%S]\n", i, getuservar(variables, i));
    for (i = 10; i < 20; i++)
        printf("$R%i = [%S]\n", i, getuservar(variables, i));
    printf("$CMDLINE  = [%S]\n", getuservar(variables, 20));
    printf("$INSTDIR  = [%S]\n", getuservar(variables, 21));
    printf("$OUTDIR   = [%S]\n", getuservar(variables, 22));
    printf("$EXEDIR   = [%S]\n", getuservar(variables, 23));
    printf("$LANGUAGE = [%S]\n", getuservar(variables, 24));
    printf("Stack is %S\n", empty(stacktop)?_T("empty") : _T(""));
    if (!empty(stacktop))  /* peek at all elements */
    {
        element = *stacktop;
        do {
            printf("[%S]\n", element->text);
            element = element->next;
        } while (element != NULL);
    }
}


/* displays a single result pushed on to stack after returning from plugin
   sets global variable result to top of stack, overwritten on all calls.
   */
TCHAR result[NSIS_MAX_STRLEN];
void showresult(stack_t **stack) 
{
    /* we expect a success or error message pushed on stack */
    if (empty(stack))
    {
        printf("No result returned on stack as stack is empty.\n");
        result[0] = '\0';
    }
    else
    {
        popstring(stack, result);
        printf("Found [%S] on stack and now stack is %Sempty.\n",
                result, empty(stack)?"":"NOT ");
    }
}


int _tmain(int argc, _TCHAR *argv[])
{
    pluginFunc pFn;
    int i, j, pn=0, pf=0;
    BOOL initstack=TRUE;

    /* these are passed to plugin */
    stack_t *stack = NULL;
    TCHAR user_vars[NSIS_MAX_STRLEN*MAXUSERVARS];

    /* initialize to predefined stuff */
    initpredefvars(user_vars);


    /* check command line appears valid, else print basic help */
    if (argc < 3)
    {
        printf("NSIS plugin wrapper/tester\nUsage: %S plugin function [args]\n", argv[0]);
        printf("Where plugin is the name (dll) of the plugin to load\n");
        printf("and function is the name of the exported function to invoke.\n");
        printf("User variables may be set using /VAR # str\n");
        printf("The remaining arguments (if any) are the strings pushed onto\n"
                "the stack and passed to the plugin (strings are pushed in\n"
                "reverse order passed on command line; ie use calling order).\n");
        return 0;
    }

    /* get plugin and function to invoke and possibly user variables */
    for (i = 1; (i < argc) && !pf; i++)
    {
        if (lstrcmp(_T("/VAR"), argv[i]) == 0)
        {
            setuservar(user_vars, _ttoi(argv[i+1]), argv[i+2]);
            i+=2;
        }
        else if (!pn)
        {
            pn = i;
        }
        else /* if (!pf) */
        {
            pf = i;
        }
    }
    /* now push items onto stack in reverse order */
    for (j=argc-1; j >= i; j--)
    {
        if (lstrcmp(_T("/VAR"), argv[j-2]) == 0)
        {
            setuservar(user_vars, _ttoi(argv[j-1]), argv[j]);
            j-=2;
        }
        else
        {
            pushstring(&stack, argv[j], initstack);
            initstack = FALSE;
        }
    }

    /* get the exported functions we want to test */
    pFn = getPluginFunction(argv[pn], argv[pf]);


    /*** NOTE: add any other initing your plugin may expect here ***/


    /* display to user stack contents prior to invoking and all variables */
    showstuff(user_vars, &stack);

    /* perform the call to plugin and display the results */
    printf("now invoking %S.%S\n", argv[pn], argv[pf]);
    pFn(NULL, NSIS_MAX_STRLEN, user_vars, &stack);
    showresult(&stack);

    /* display to user stack contents after invoking and all variables */
    showstuff(user_vars, &stack);

    return 0;
}
