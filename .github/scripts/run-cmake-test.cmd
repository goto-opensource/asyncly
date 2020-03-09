mkdir "_build" || EXIT /B 1
cd "_build" || EXIT /B 1
cmake .. -A Win32 -DCMAKE_TOOLCHAIN_FILE=%VCPKG_INSTALLATION_ROOT%\scripts\buildsystems\vcpkg.cmake || EXIT /B 1
cmake --build . --config Debug || EXIT /B 1
cmake --build . --config Release || EXIT /B 1
ctest -C Debug -V -LE Benchmark || EXIT /B 1
ctest -C Release -V || EXIT /B 1
