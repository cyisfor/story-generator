# sigh...

git submodule update --init
set -e

prefix=$(realpath $(dirname $0))/deps/


function doit() {
		mkdir -p build
		cd build
		../configure --prefix=$prefix "$@"
		make -j12 -l12
		make -i install
}
pushd libxml2
doit --disable-python
popd
pushd libxmlfixes
doit
popd
pushd htmlish
doit
popd
cd ../..
doit
