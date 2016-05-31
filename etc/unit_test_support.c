#if TICB_UNITTEST_SUPPORT
static int static_num_tests = 0;
// static int static_total_tests = 0;
#define static_total_tests static_num_tests
static int static_failed_tests = 0;

#ifdef _WIN32         //////////////// Windows specific defines and includes
#ifndef __func__
#define STRX(x) #x
#define STR(x) STRX(x)
#define __func__ __FILE__ ":" STR(__LINE__)
#endif
#else
#if 0
#if __STDC_VERSION__ < 199901L && !defined(WIN32)
#define __func__ ""
#warning "__func__ is empty"
#endif
#endif
#endif

#ifdef NS_TEST_ABORT_ON_FAIL
#define NS_TEST_ABORT abort()
#else
#define NS_TEST_ABORT
#endif

#if 1
#define FAIL(str, line) do {                    \
  printf("%s:%d:1 [%s] (in %s)\n", __FILE__,    \
         line, str, __func__);                  \
  static_failed_tests++; NS_TEST_ABORT;                                \
  return -1;                                   \
} while (0)
#else
#define FAIL(str, line) do {                    \
  printf("%s:%d:1 [%s] (in %s)\n", __FILE__,    \
         line, str, __func__);                  \
  NS_TEST_ABORT;                                \
  return str;                                   \
} while (0)
#endif

#define ASSERT(expr) do {             \
  static_num_tests++;                 \
  if (!(expr)) FAIL(#expr, __LINE__); \
} while (0)

#define RUN_TEST(test) do {                 \
  const char *msg = NULL;                   \
  if (strstr(# test, filter)) msg = test(); \
  if (msg) return msg;                      \
} while (0)
#endif


