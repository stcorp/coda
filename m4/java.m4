# ST_CHECK_JAVA
# -------------
# Check for the availability of Java
AC_DEFUN([ST_CHECK_JAVA],
[AC_ARG_VAR(JAVA,[The java executable])
AC_PATH_PROG([JAVA], [java])
AC_ARG_VAR(JAVAC,[The javac executable])
AC_PATH_PROG([JAVAC], [javac])
AC_ARG_VAR(JAR,[The jar executable])
AC_PATH_PROG([JAR], [jar])
AC_ARG_VAR(JAVA_HOME,[The Java Home directory])
if test "$JAVA_HOME" = "" ; then
  if test "$JAVA" != "" ; then
    AC_MSG_CHECKING([for JAVA_HOME])
    [cat > conftest.java <<_ACEOF
public class conftest {
    public static void main(String[] args) {
        if (args != null && args.length > 0) {
            System.out.println(System.getProperty(args[0]));
        }
    }
}
_ACEOF]
    if { (eval echo "$as_me:$LINENO: \"$JAVAC conftest.java\"") >&5
         (eval $JAVAC conftest.java >&5) 2>conftest.er1
         ac_status=$?
         grep -v '^ *+' conftest.er1 >conftest.err
         rm -f conftest.er1
         cat conftest.err >&5
         echo "$as_me:$LINENO: \$? = $ac_status" >&5
         (exit $ac_status); } >/dev/null; then
      if { (eval echo "$as_me:$LINENO: \"$JAVA -classpath . conftest\"") >&5
           (eval $JAVA -classpath . conftest java.home >conftest.out) 2>conftest.er1
           ac_status=$?
           grep -v '^ *+' conftest.er1 >conftest.err
           rm -f conftest.er1
           cat conftest.err >&5
           echo "$as_me:$LINENO: \$? = $ac_status" >&5
           (exit $ac_status); } >/dev/null; then
         JAVA_HOME=`cat conftest.out`
         AC_MSG_RESULT([$JAVA_HOME])           
       else
         AC_MSG_RESULT([failed])
       fi
    else
      AC_MSG_RESULT([failed])
    fi

  fi
fi
AC_MSG_CHECKING(for JAVA installation)
if test "$JAVA" = "" || test "$JAVAC" = "" || test "$JAR" = "" ||
   test "$JAVA_HOME" = "" ; then
  st_cv_have_java=no
else
  st_cv_have_java=yes
fi
AC_MSG_RESULT($st_cv_have_java)
])# ST_CHECK_JAVA
