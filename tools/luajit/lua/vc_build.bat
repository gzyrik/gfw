@echo off
call "%VS90COMNTOOLS%\vsvars32.bat" >NUL
echo devenv %*
devenv %*
IF ERRORLEVEL 1 (
        echo "***************************"
        echo " ע��,���뷢������ !!!"
        echo " ��Ctrl+c, Ctrl+c��ֹ, ����"
        pause
)
@echo on
