@echo off
call GenerateMSVC2022.cmd
cd ..

IF NOT EXIST Build (
echo Build folder does not exist.
goto ERROR
)

cd Build

MSBuild Heady.sln /p:Configuration=Debug && (
  echo Debug build succeeded
) || (
  echo Debug build failed
  goto ERROR
)

MSBuild Heady.sln /p:Configuration=Release && (
  echo Release build succeeded
) || (
  echo Release build failed
  goto ERROR
)

cd ..
cd Bin
EXIT /B 0

:ERROR
cd ..
cd Bin
EXIT /B 1
