@ECHO OFF

SETLOCAL

REM User definable settings

SET CODASRCDIR=..\java
SET LIBCODADIR=Release

SET JAR="%JAVA_HOME%\bin\jar"
SET JAVAC="%JAVA_HOME%\bin\javac"


ECHO Compiling java files
%JAVAC% -d . %CODASRCDIR%\nl\stcorp\coda\*.java

ECHO Creating coda.jar
%JAR% -cf coda.jar nl


ENDLOCAL
