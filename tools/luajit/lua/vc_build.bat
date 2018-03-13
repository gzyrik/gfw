@echo off
call "%VS90COMNTOOLS%\vsvars32.bat" >NUL
echo devenv %*
devenv %*
IF ERRORLEVEL 1 (
        echo "***************************"
        echo " 注意,编译发生错误 !!!"
        echo " 按Ctrl+c, Ctrl+c终止, 否则"
        pause
)
@echo on
