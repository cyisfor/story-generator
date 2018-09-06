# sigh...

git submodule update --init
set -e
cd libxml2
function doit() {
		mkdir -p build
		cd build
		../configure --prefix=$prefix
		make -j12 -l12
		make install
}
doit
cd ..
doit
cd ..
doit
