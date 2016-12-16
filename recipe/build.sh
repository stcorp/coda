./bootstrap

if [ "$(uname)" == Darwin ]; then
    export CFLAGS="-headerpad_max_install_names"
fi

./configure --prefix="$PREFIX" \
            --with-hdf4 HDF4_LIB="$PREFIX/lib" HDF4_INCLUDE="$PREFIX/include"\
            JPEG_LIB="$PREFIX/lib" ZLIB_LIB="$PREFIX/lib" \
            --with-hdf5 HDF5_LIB="$PREFIX/lib" HDF5_INCLUDE="$PREFIX/include" \
            --enable-python PYTHON="$PREFIX/bin/python"
make
make install
