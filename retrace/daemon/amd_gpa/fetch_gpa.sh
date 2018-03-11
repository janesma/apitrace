#!/bin/sh

git clone git://github.com/janesma/amd_gpa.git GPA
python -u GPA/Scripts/UpdateCommon.py
cd GPA/Build/Linux
sh build.sh debug skip32bitbuild skipopencl skiphsa skiptests
