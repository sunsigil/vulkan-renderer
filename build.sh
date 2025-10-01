if [ ! -d build ]; then
	mkdir build
fi
if [ ! -f build/makefile ]; then
	cd build
	cmake ..
	cd ..
fi

cd build
make -j8
cd ..

build/main
