# INSTRUCTIONS FOR BUILDING EXTERNAL LIBS  

Prepare 
```
apt-get update && apt-get install -y unzip automake build-essential curl file pkg-config git python libtool
mkdir /opt/android/
cd /opt/android
```

Build cmake
```
curl -s -O https://cmake.org/files/v3.12/cmake-3.12.1.tar.gz 
tar -xzf cmake-3.12.1.tar.gz 
cd cmake-3.12.1
./configure 
make -j 4
make install
```

Fetch and prepare android ndk
```
cd /opt/android
curl -s -O https://dl.google.com/android/repository/android-ndk-r17b-linux-x86_64.zip 
unzip android-ndk-r17b-linux-x86_64.zip 
rm -f android-ndk-r17b-linux-x86_64.zip 
ln -s android-ndk-r17b ndk
ndk/build/tools/make_standalone_toolchain.py --api 21 --stl=libc++ --arch arm --install-dir /opt/android/tool/arm 
ndk/build/tools/make_standalone_toolchain.py --api 21 --stl=libc++ --arch arm64 --install-dir /opt/android/tool/arm64 
ndk/build/tools/make_standalone_toolchain.py --api 21 --stl=libc++ --arch x86 --install-dir /opt/android/tool/x86 
ndk/build/tools/make_standalone_toolchain.py --api 21 --stl=libc++ --arch x86_64 --install-dir /opt/android/tool/x86_64
```
Build openssl for android
```
git clone https://github.com/m2049r/android-openssl.git --depth=1 
curl -L -s -O https://github.com/openssl/openssl/archive/OpenSSL_1_0_2l.tar.gz 
cd android-openssl 
tar xfz ../OpenSSL_1_0_2l.tar.gz 
rm -f ../OpenSSL_1_0_2l.tar.gz 
ANDROID_NDK_ROOT=/opt/android/ndk ./build-all-arch.sh

mkdir -p /opt/android/build/openssl/{arm,arm64,x86,x86_64} 
cp -a /opt/android/android-openssl/prebuilt/armeabi /opt/android/build/openssl/arm/lib 
cp -a /opt/android/android-openssl/prebuilt/arm64-v8a /opt/android/build/openssl/arm64/lib 
cp -a /opt/android/android-openssl/prebuilt/x86 /opt/android/build/openssl/x86/lib 
cp -a /opt/android/android-openssl/prebuilt/x86_64 /opt/android/build/openssl/x86_64/lib 
cp -aL /opt/android/android-openssl/openssl-OpenSSL_1_0_2l/include/openssl/ /opt/android/build/openssl/include 
ln -s /opt/android/build/openssl/include /opt/android/build/openssl/arm/include 
ln -s /opt/android/build/openssl/include /opt/android/build/openssl/arm64/include 
ln -s /opt/android/build/openssl/include /opt/android/build/openssl/x86/include 
ln -s /opt/android/build/openssl/include /opt/android/build/openssl/x86_64/include 
ln -sf /opt/android/build/openssl/include /opt/android/tool/arm/sysroot/usr/include/openssl 
ln -sf /opt/android/build/openssl/arm/lib/*.so /opt/android/tool/arm/sysroot/usr/lib 
ln -sf /opt/android/build/openssl/include /opt/android/tool/arm64/sysroot/usr/include/openssl 
ln -sf /opt/android/build/openssl/arm64/lib/*.so /opt/android/tool/arm64/sysroot/usr/lib 
ln -sf /opt/android/build/openssl/include /opt/android/tool/x86/sysroot/usr/include/openssl 
ln -sf /opt/android/build/openssl/x86/lib/*.so /opt/android/tool/x86/sysroot/usr/lib 
ln -sf /opt/android/build/openssl/include /opt/android/tool/x86_64/sysroot/usr/include/openssl 
ln -sf /opt/android/build/openssl/x86_64/lib/*.so /opt/android/tool/x86_64/sysroot/usr/lib64
```
Build boost
```
cd /opt/android 
curl -s -L -o  boost_1_67_0.tar.bz2 https://sourceforge.net/projects/boost/files/boost/1.67.0/boost_1_67_0.tar.bz2/download  
tar -xvf boost_1_67_0.tar.bz2 
rm -f /usr/boost_1_67_0.tar.bz2 
cd boost_1_67_0 
./bootstrap.sh
PATH=/opt/android/tool/arm/arm-linux-androideabi/bin:/opt/android/tool/arm/bin:$PATH ./b2 --build-type=minimal link=static runtime-link=static --with-chrono --with-date_time --with-filesystem --with-program_options --with-regex --with-serialization --with-system --with-thread --build-dir=android-arm --prefix=/opt/android/build/boost/arm --includedir=/opt/android/build/boost/include toolset=clang threading=multi threadapi=pthread target-os=android install 
PATH=/opt/android/tool/arm64/aarch64-linux-android/bin:/opt/android/tool/arm64/bin:$PATH ./b2 --build-type=minimal link=static runtime-link=static --with-chrono --with-date_time --with-filesystem --with-program_options --with-regex --with-serialization --with-system --with-thread --build-dir=android-arm64 --prefix=/opt/android/build/boost/arm64 --includedir=/opt/android/build/boost/include toolset=clang threading=multi threadapi=pthread target-os=android install 
PATH=/opt/android/tool/x86/i686-linux-android/bin:/opt/android/tool/x86/bin:$PATH ./b2 --build-type=minimal link=static runtime-link=static --with-chrono --with-date_time --with-filesystem --with-program_options --with-regex --with-serialization --with-system --with-thread --build-dir=android-x86 --prefix=/opt/android/build/boost/x86 --includedir=/opt/android/build/boost/include toolset=clang threading=multi threadapi=pthread target-os=android install 
PATH=/opt/android/tool/x86_64/x86_64-linux-android/bin:/opt/android/tool/x86_64/bin:$PATH ./b2 --build-type=minimal link=static runtime-link=static --with-chrono --with-date_time --with-filesystem --with-program_options --with-regex --with-serialization --with-system --with-thread --build-dir=android-x86_64 --prefix=/opt/android/build/boost/x86_64 --includedir=/opt/android/build/boost/include toolset=clang threading=multi threadapi=pthread target-os=android install 
ln -sf ../include /opt/android/build/boost/arm 
ln -sf ../include /opt/android/build/boost/arm64 
ln -sf ../include /opt/android/build/boost/x86 
ln -sf ../include /opt/android/build/boost/x86_64
```
Build libsodium
```
cd /opt/android 
git clone https://github.com/jedisct1/libsodium.git -b 1.0.17 --depth=1 
cd libsodium 
test `git rev-parse HEAD` = b732443c442239c2e0184820e9b23cca0de0828c || exit 1 
./autogen.sh
```
ignore the version info warnings 
```
PATH=/opt/android/tool/arm/arm-linux-androideabi/bin:/opt/android/tool/arm/bin:$PATH ; CC=clang CXX=clang++ CFLAGS="-fPIC" CXXFLAGS="-fPIC" ./configure --prefix=/opt/android/build/libsodium/arm --host=arm-linux-androideabi --enable-static --disable-shared && make && make install && make clean 
PATH=/opt/android/tool/arm64/aarch64-linux-android/bin:/opt/android/tool/arm64/bin:$PATH ; CC=clang CXX=clang++ CFLAGS="-fPIC" CXXFLAGS="-fPIC" ./configure --prefix=/opt/android/build/libsodium/arm64 --host=aarch64-linux-android --enable-static --disable-shared && make && make install && make clean 
PATH=/opt/android/tool/x86/i686-linux-android/bin:/opt/android/tool/x86/bin:$PATH ; CC=clang CXX=clang++ CFLAGS="-fPIC" CXXFLAGS="-fPIC" ./configure --prefix=/opt/android/build/libsodium/x86 --host=i686-linux-android --enable-static --disable-shared && make && make install && make clean 
PATH=/opt/android/tool/x86_64/x86_64-linux-android/bin:/opt/android/tool/x86_64/bin:$PATH ; CC=clang CXX=clang++ CFLAGS="-fPIC" CXXFLAGS="-fPIC" ./configure --prefix=/opt/android/build/libsodium/x86_64 --host=x86_64-linux-android --enable-static --disable-shared && make && make install && make clean
```
Build libzmq
```
cd /opt/android 
git clone https://github.com/zeromq/libzmq.git -b v4.3.1 --depth=1 
cd libzmq 
test `git rev-parse HEAD` = 2cb1240db64ce1ea299e00474c646a2453a8435b || exit 1 
./autogen.sh 
PATH=/opt/android/tool/arm/arm-linux-androideabi/bin:/opt/android/tool/arm/bin:$PATH ; CC=clang CXX=clang++ CFLAGS="-fPIC" CXXFLAGS="-fPIC" ./configure --prefix=/opt/android/build/libzmq/arm --host=arm-linux-androideabi --enable-static --disable-shared && make && make install && ldconfig && make clean 
PATH=/opt/android/tool/arm64/aarch64-linux-android/bin:/opt/android/tool/arm64/bin:$PATH ; CC=clang CXX=clang++ CFLAGS="-fPIC" CXXFLAGS="-fPIC" ./configure --prefix=/opt/android/build/libzmq/arm64 --host=aarch64-linux-android --enable-static --disable-shared && make && make install && ldconfig && make clean 
PATH=/opt/android/tool/x86/i686-linux-android/bin:/opt/android/tool/x86/bin:$PATH ; CC=clang CXX=clang++ CFLAGS="-fPIC" CXXFLAGS="-fPIC" ./configure --prefix=/opt/android/build/libzmq/x86 --host=i686-linux-android --enable-static --disable-shared && make && make install && ldconfig && make clean 
PATH=/opt/android/tool/x86_64/x86_64-linux-android/bin:/opt/android/tool/x86_64/bin:$PATH ; CC=clang CXX=clang++ CFLAGS="-fPIC" CXXFLAGS="-fPIC" ./configure --prefix=/opt/android/build/libzmq/x86_64 --host=x86_64-linux-android --enable-static --disable-shared && make && make install && ldconfig && make clean
```
Build sumokoin
```
cd /opt/android 
git clone --recursive https://github.com/sumoprojects/sumokoin.git -b android-wallet 
``` 
Rename sumokoin folder to monero for convenience
```
mv /opt/android/sumokoin /opt/android/monero/
```
```
cd monero
mkdir -p build/release 
sudo apt update && sudo apt install build-essential cmake pkg-config libunbound-dev libunwind8-dev liblzma-dev libreadline6-dev libldns-dev libexpat1-dev doxygen graphviz libpgm-dev qttools5-dev-tools libhidapi-dev libusb-dev libprotobuf-dev protobuf-compiler
chmod u+x build-all-archs.sh
./build-all-archs.sh
```
Bringing the files we need into sumokoin android wallet
```
cd /opt/android
git clone --recursive https://github.com/sumoprojects/sumo-android-wallet
cd /opt/android/sumo-android-wallet/external-libs
find . -type f -name "*.a" -exec rm -f {} \;
find . -type f -name "*.h" -exec rm -f {} \;
chmod u+x collect.sh
./collect.sh
```
Done! You may get your sumo-android-wallet now and compile the app on Android Studio
