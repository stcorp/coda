AC_DEFUN([ST_THREAD_LOCAL_STORAGE], [
  AC_MSG_CHECKING([for thread local storage (TLS) keyword])
  AC_CACHE_VAL([ac_cv_tls],
    [for st_tls_keyword in thread_local _Thread_local __thread '__declspec(thread)' none; do
       AS_CASE([$st_tls_keyword], [none], [ac_cv_tls=none ; break],
               [AC_TRY_COMPILE([void f(void) { static ] $st_tls_keyword [ int bar; }],
                               [], [ac_cv_tls=$st_tls_keyword ; break], ac_cv_tls=none)])
     done])
  AC_MSG_RESULT([$ac_cv_tls])
  AS_IF([test "$ac_cv_tls" != "none"],
    [AC_DEFINE_UNQUOTED([THREAD_LOCAL],[$ac_cv_tls],[thread local storage (TLS) keyword])])
])
