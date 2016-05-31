/*===
cexcept.h 2.0.1 (2008-Jul-19-Sat)
http://www.nicemice.net/cexcept/
Adam M. Costello
http://www.nicemice.net/amc/

An interface for exception-handling in ANSI C (C89 and subsequent ISO
standards), developed jointly with Cosmin Truta.

    Copyright (c) 2000-2008 Adam M. Costello and Cosmin Truta.
    This software may be modified only if its author and version
    information is updated accurately, and may be redistributed
    only if accompanied by this unaltered notice.  Subject to those
    restrictions, permission is granted to anyone to do anything
    with this software.  The copyright holders make no guarantees
    regarding this software, and are not responsible for any damage
    resulting from its use.

The cexcept interface is not compatible with and cannot interact
with system exceptions (like division by zero or memory segmentation
violation), compiler-generated exceptions (like C++ exceptions), or
other exception-handling interfaces.

When using this interface across multiple .c files, do not include
this header file directly.  Instead, create a wrapper header file that
includes this header file and then invokes the define_exception_type
macro (see below).  The .c files should then include that header file.

The interface consists of one type, one well-known name, and six macros.


define_exception_type(type_name);

    This macro is used like an external declaration.  It specifies
    the type of object that gets copied from the exception thrower to
    the exception catcher.  The type_name can be any type that can be
    assigned to, that is, a non-constant arithmetic type, struct, union,
    or pointer.  Examples:

        define_exception_type(int);

        enum exception { out_of_memory, bad_arguments, disk_full };
        define_exception_type(enum exception);

        struct exception { int code; const char *msg; };
        define_exception_type(struct exception);

    Because throwing an exception causes the object to be copied (not
    just once, but twice), programmers may wish to consider size when
    choosing the exception type.


struct exception_context;

    This type may be used after the define_exception_type() macro has
    been invoked.  A struct exception_context must be known to both
    the thrower and the catcher.  It is expected that there be one
    context for each thread that uses exceptions.  It would certainly
    be dangerous for multiple threads to access the same context.
    One thread can use multiple contexts, but that is likely to be
    confusing and not typically useful.  The application can allocate
    this structure in any way it pleases--automatic, static, or dynamic.
    The application programmer should pretend not to know the structure
    members, which are subject to change.


struct exception_context *the_exception_context;

    The Try/Catch and Throw statements (described below) implicitly
    refer to a context, using the name the_exception_context.  It is
    the application's responsibility to make sure that this name yields
    the address of a mutable (non-constant) struct exception_context
    wherever those statements are used.  Subject to that constraint, the
    application may declare a variable of this name anywhere it likes
    (inside a function, in a parameter list, or externally), and may
    use whatever storage class specifiers (static, extern, etc) or type
    qualifiers (const, volatile, etc) it likes.  Examples:

        static struct exception_context
          * const the_exception_context = &foo;

        { struct exception_context *the_exception_context = bar; ... }

        int blah(struct exception_context *the_exception_context, ...);

        extern struct exception_context the_exception_context[1];

    The last example illustrates a trick that avoids creating a pointer
    object separate from the structure object.

    The name could even be a macro, for example:

        struct exception_context ec_array[numthreads];
        #define the_exception_context (ec_array + thread_id)

    Be aware that the_exception_context is used several times by the
    Try/Catch/Throw macros, so it shouldn't be expensive or have side
    effects.  The expansion must be a drop-in replacement for an
    identifier, so it's safest to put parentheses around it.


void init_exception_context(struct exception_context *ec);

    For context structures allocated statically (by an external
    definition or using the "static" keyword), the implicit
    initialization to all zeros is sufficient, but contexts allocated
    by other means must be initialized using this macro before they
    are used by a Try/Catch statement.  It does no harm to initialize
    a context more than once (by using this macro on a statically
    allocated context, or using this macro twice on the same context),
    but a context must not be re-initialized after it has been used by a
    Try/Catch statement.


Try statement
Catch (expression) statement

    The Try/Catch/Throw macros are capitalized in order to avoid
    confusion with the C++ keywords, which have subtly different
    semantics.

    A Try/Catch statement has a syntax similar to an if/else statement,
    except that the parenthesized expression goes after the second
    keyword rather than the first.  As with if/else, there are two
    clauses, each of which may be a simple statement ending with a
    semicolon or a brace-enclosed compound statement.  But whereas
    the else clause is optional, the Catch clause is required.  The
    expression must be a modifiable lvalue (something capable of being
    assigned to) of the same type (disregarding type qualifiers) that
    was passed to define_exception_type().

    If a Throw that uses the same exception context as the Try/Catch is
    executed within the Try clause (typically within a function called
    by the Try clause), and the exception is not caught by a nested
    Try/Catch statement, then a copy of the exception will be assigned
    to the expression, and control will jump to the Catch clause.  If no
    such Throw is executed, then the assignment is not performed, and
    the Catch clause is not executed.

    The expression is not evaluated unless and until the exception is
    caught, which is significant if it has side effects, for example:

        Try foo();
        Catch (p[++i].e) { ... }

    IMPORTANT: Jumping into or out of a Try clause (for example via
    return, break, continue, goto, longjmp) is forbidden--the compiler
    will not complain, but bad things will happen at run-time.  Jumping
    into or out of a Catch clause is okay, and so is jumping around
    inside a Try clause.  In many cases where one is tempted to return
    from a Try clause, it will suffice to use Throw, and then return
    from the Catch clause.  Another option is to set a flag variable and
    use goto to jump to the end of the Try clause, then check the flag
    after the Try/Catch statement.

    IMPORTANT: The values of any non-volatile automatic variables
    changed within the Try clause are undefined after an exception is
    caught.  Therefore, variables modified inside the Try block whose
    values are needed later outside the Try block must either use static
    storage or be declared with the "volatile" type qualifier.


Throw expression;

    A Throw statement is very much like a return statement, except that
    the expression is required.  Whereas return jumps back to the place
    where the current function was called, Throw jumps back to the Catch
    clause of the innermost enclosing Try clause.  The expression must
    be compatible with the type passed to define_exception_type().  The
    exception must be caught, otherwise the program may crash.

    Slight limitation:  If the expression is a comma-expression, it must
    be enclosed in parentheses.


Try statement
Catch_anonymous statement

    When the value of the exception is not needed, a Try/Catch statement
    can use Catch_anonymous instead of Catch (expression).


Everything below this point is for the benefit of the compiler.  The
application programmer should pretend not to know any of it, because it
is subject to change.

===*/


#ifndef CEXCEPT_H
#define CEXCEPT_H

#if 1//ndef WIN32
#include <setjmp.h>

#ifdef WIN32
#include<windows.h>
#define EXCEPT_FOR_PID 0
#else
#define EXCEPT_FOR_PID 1
#ifdef setjmp
#undef setjmp
#endif

#define setjmp(env) sigsetjmp(env,1/*savesigs*/)
#define longjmp(env,val) siglongjmp(env,val)
#define jmp_buf sigjmp_buf
#include<signal.h>
#endif

enum exception_flavor { okay, 
		sighup,
		sigint,
		sigquit,
		sigill,
		sigtrap,
		sigabrt,	// sigiot
		sigbus,
		sigfpe,
		sigkill, //kill -9
		sigusr1,
		sigsegv,
		sigusr2,
		sigpipe,
		sigalrm,
		sigterm, //killall
		sigstkflt,
		sigchld,
		sigcont,
		sigstop,
		sigcont2,
		sigttin,
		sigttou,
		sigurg,
		sigxcpu,
		sigxfsz,
		sigvtalrm,
		sigprof,
		sigwinch,
		sigio,		// sigpoll
		sigpwr,
		sigsys,		// sigunused
		sigrtmin
//		sigrtmax = _NSIG
};
typedef struct Cexception {
  enum exception_flavor flavor;
#define NO_INFO_EXCEPT  0
#if !NO_INFO_EXCEPT
  const char *msg;
  union {
    int oops;
    long screwup;
    long segvexcept;
    char barf[8];
  } info;
#endif
}Cexception_t;


#define define_exception_type(etype) \
	struct exception_context { \
	  jmp_buf *penv; \
	  int caught; \
	  volatile struct _ETYPE_ { etype etmp; } v; \
	}

/* etmp must be volatile because the application might use automatic */
/* storage for the_exception_context, and etmp is modified between   */
/* the calls to setjmp() and longjmp().  A wrapper struct is used to */
/* avoid warnings about a duplicate volatile qualifier in case etype */
/* already includes it.                                              */

#define init_exception_context(ec) ((void)((ec)->penv = 0))

#define Try \
  { \
    jmp_buf *exception__prev, exception__env; \
    exception__prev = the_exception_context->penv; \
    the_exception_context->penv = &exception__env; \
    if (setjmp(exception__env) == 0) { \
      do

#define exception__catch(action) \
      while (the_exception_context->caught = 0, \
             the_exception_context->caught); \
    } \
    else { \
      the_exception_context->caught = 1; \
    } \
    the_exception_context->penv = exception__prev; \
  } \
  if (!the_exception_context->caught || action) { } \
  else

#define Catch(e) exception__catch(((e) =*(Cexception_t*)&(the_exception_context->v.etmp), 0))
#define Catch_anonymous exception__catch(0)

/* Try ends with do, and Catch begins with while(0) and ends with     */
/* else, to ensure that Try/Catch syntax is similar to if/else        */
/* syntax.                                                            */
/*                                                                    */
/* The 0 in while(0) is expressed as x=0,x in order to appease        */
/* compilers that warn about constant expressions inside while().     */
/* Most compilers should still recognize that the condition is always */
/* false and avoid generating code for it.                            */

#define Throw \
  for (;; longjmp(*the_exception_context->penv, 1)) \
    the_exception_context->v.etmp =

#define MAX_THREAD	20

#if 1
define_exception_type(Cexception_t);
#else
define_exception_type(int);
#endif


struct thread_state {
  int blah;long int pid;void* arg;int count;
  struct exception_context ec[1];
  unsigned long junk;
};

extern struct thread_state g_state[MAX_THREAD];

struct thread_state* allocstate(long int pid);
struct thread_state* findstate(long int pid,const char* dbgStr);
void freestate(long int pid);
void sighandler(int signum);
void init_except_handler();
void SetTID(char *name, long int tid);

//			int tid=pthread_self()
//			state->count=0;state->arg=arg; 
#if EXCEPT_FOR_PID
#define get_tid()               getpid()
#define equal_tid(t1,t2)        (t1==t2)
#else
#define get_tid()       pthread_self()
#define equal_tid(t1,t2)     pthread_equal(t1,t2)
#endif
#if 1
#define NEW_THREAD_EXCEPT_CONTEXT(e)   struct thread_state *state; \
			struct exception_context *the_exception_context; \
			long int ___tid=get_tid(); \
			Cexception_t e; \
			state=allocstate(___tid); \
			if(state)the_exception_context = state->ec; \
			if(state){init_exception_context(the_exception_context); e.flavor=okay;}
#else
#define NEW_THREAD_EXCEPT_CONTEXT(e, except_name)   struct thread_state *state; \
			struct exception_context *the_exception_context; \
			Cexception_t e; \
			state=allocstate(except_name); \
			the_exception_context = state->ec; \
			init_exception_context(the_exception_context);
#endif

#define DEL_THREAD_EXCEPT_CONTEXT() freestate(___tid);

#define FIND_THREAD_EXCEPT_CONTEXT(e,dbgStr)   struct thread_state *state; \
		struct exception_context *the_exception_context=(struct exception_context *)NULL; \
		long int ___tid=get_tid(); \
		Cexception_t e; \
		state=findstate(___tid,dbgStr); \
		if(!state){  fprintf(stderr,"FIND_THREAD_EXCEPT_CONTEXT: Cannot handle for pid%d(%s)\n",(unsigned int)___tid,dbgStr);} \
		else the_exception_context = state->ec;
#define CHECK_THREAD_EXCEPT_CONTEXT(e,dbgStr)	\
		state=findstate(___tid,dbgStr); \
		if(!state){  fprintf(stderr,"CHECK_THREAD_EXCEPT_CONTEXT: Cannot handle for pid%d(%s)\n",(unsigned int)___tid,dbgStr);} 
#endif
void test_exception(void);
#endif /* CEXCEPT_H */
