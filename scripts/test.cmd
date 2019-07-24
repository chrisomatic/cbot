mkdir output\test || goto :error
echo "Hello There." > output\test.txt

exit /B 0

:error
exit /B 1
