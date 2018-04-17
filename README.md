# Sumokoin

Copyright (c) 2017, Sumokoin.org

Copyright (c) 2014-2017, The Monero Project

Portions Copyright (c) 2012-2013, The Cryptonote developers

## Development Resources

- Web: [www.sumokoin.org](https://www.sumokoin.org)
- Mail: [contact@sumokoin.org](mailto:contact@sumokoin.org)

Please note that code is developed on the [dev branch](https://github.com/sumoprojects/sumokoin/tree/dev), if you want to check out the latest updates, before they are merged on main branch, please refer there. Master branch will always point to a version that we consider stable, so you can download the code by simply typing `git clone https://github.com/sumoprojects/sumokoin.git`

## Introduction

Sumokoin (スモコイン in Japanese) is a fork from Monero, one of the most respectable cryptocurrency well-known for **security, privacy, untraceability** and **active development**. Starting as an educational project, we found that it would be great to create a new coin with high level of privacy by (1) moving forward right away to **Ring Confidential Transactions (RingCT)**, (2) setting **minimum transaction _mixin_ to 12** that would greatly reduce chance of being attacked, traced or identified by (blockchain) statistical analysis.

Sumokoin, therefore, is a new Monero without its legacy, a _truely fungible_ cryptocurrency among just a few ones in the market.

## Coin Supply & Emission

- **Total supply**: **88,888,888** coins in first 20 years, then **263,000** coins each year for inflation. 
About 10% (~8.8 million) was premined to reserve for future development, i.e. **80 million coins available** for community mining.
- **Coin symbol**: **SUMO**
- **Coin Units**:
  + 1 Sumoshi &nbsp;= 0.000000001 **SUMO** (10<sup>-9</sup> - _the smallest coin unit_)
  + 1 Sumokun = 0.000001 **SUMO** (10<sup>-6</sup>)
  + 1 Sumosan = 0.001 **SUMO** (10<sup>-3</sup>)
- **Hash algorithm**: CryptoNight (Proof-Of-Work)
- **Emission scheme**: Sumokoin's block reward changes _every 6-months_ as the following "Camel" distribution* (inspired by _real-world mining production_ like of crude oil, coal etc. that is often slow at first, 
accelerated in the next few years before declined and depleted). However, the emission path of Sumokoin is generally not far apart from what of Bitcoin (view charts below).

![](http://www.sumokoin.org/images/block_reward_by_calendar_year.png)

![](http://www.sumokoin.org/images/block_reward_by_calendar_month.png)

![](http://www.sumokoin.org/images/emission_speed_sumo_vs_btc.png)

\* The emulated algorithm of Sumokoin block-reward emission can be found in Python and C++ scripts at [scripts](scripts) directory.

## About this Project

This is the core implementation of Sumokoin. It is open source and completely free to use without restrictions, except for those specified in the license agreement below. There are no restrictions on anyone creating an alternative implementation of Sumokoin that uses the protocol and network in a compatible manner.

As with many development projects, the repository on Github is considered to be the "staging" area for the latest changes. Before changes are merged into that branch on the main repository, they are tested by individual developers in their own branches, submitted as a pull request, and then subsequently tested by contributors who focus on testing and code reviews. That having been said, the repository should be carefully considered before using it in a production environment, unless there is a patch in the repository for a particular show-stopping issue you are experiencing. It is generally a better idea to use a tagged release for stability.

**Anyone is welcome to contribute to Sumokoin's codebase!** If you have a fix or code change, feel free to submit is as a pull request directly to the "master" branch. In cases where the change is relatively small or does not affect other parts of the codebase it may be merged in immediately by any one of the collaborators. On the other hand, if the change is particularly large or complex, it is expected that it will be discussed at length either well in advance of the pull request being submitted, or even directly on the pull request.

## License

Please view [LICENSE](LICENSE)

[![License](https://img.shields.io/badge/license-BSD3-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

## Compiling Sumokoin from Source

### Dependencies

The following table summarizes the tools and libraries required to build.  A
few of the libraries are also included in this repository (marked as
"Vendored"). By default, the build uses the library installed on the system,
and ignores the vendored sources. However, if no library is found installed on
the system, then the vendored source will be built and used. The vendored
sources are also used for statically-linked builds because distribution
packages often include only shared library binaries (`.so`) but not static
library archives (`.a`).

| Dep            | Min. Version  | Vendored | Debian/Ubuntu Pkg  | Arch Pkg       | Optional | Purpose        |
| -------------- | ------------- | ---------| ------------------ | -------------- | -------- | -------------- |
| GCC            | 4.7.3         | NO       | `build-essential`  | `base-devel`   | NO       |                |
| CMake          | 3.0.0         | NO       | `cmake`            | `cmake`        | NO       |                |
| pkg-config     | any           | NO       | `pkg-config`       | `base-devel`   | NO       |                |
| Boost          | 1.58          | NO       | `libboost-all-dev` | `boost`        | NO       |                |
| OpenSSL      	 | basically any | NO       | `libssl-dev`       | `openssl`      | NO       | sha256 sum     |
| BerkeleyDB     | 4.8           | NO       | `libdb{,++}-dev`   | `db`           | NO       |                |
| libevent       | 2.0           | NO       | `libevent-dev`     | `libevent`     | NO       |                |
| libunbound     | 1.4.16        | YES      | `libunbound-dev`   | `unbound`      | NO       |                |
| libminiupnpc   | 2.0           | YES      | `libminiupnpc-dev` | `miniupnpc`    | YES      | NAT punching   |
| libunwind      | any           | NO       | `libunwind8-dev`   | `libunwind`    | YES      | stack traces   |
| ldns           | 1.6.17        | NO       | `libldns-dev`      | `ldns`         | YES      | ?              |
| expat          | 1.1           | NO       | `libexpat1-dev`    | `expat`        | YES      | ?              |
| GTest          | 1.5           | YES      | `libgtest-dev`^    | `gtest`        | YES      | test suite     |
| Doxygen        | any           | NO       | `doxygen`          | `doxygen`      | YES      | documentation  |
| Graphviz       | any           | NO       | `graphviz`         | `graphviz`     | YES      | documentation  |

[^] On Debian/Ubuntu `libgtest-dev` only includes sources and headers. You must
build the library binary manually. This can be done with the following command ```sudo apt-get install libgtest-dev && cd /usr/src/gtest && sudo cmake . && sudo make && sudo mv libg* /usr/lib/ ```

### Build instructions

Sumokoin uses the CMake build system and a top-level [Makefile](Makefile) that
invokes cmake commands as needed.

#### On Linux and OS X

* Install the dependencies (see the list above)

    \- On Ubuntu 16.04, essential dependencies can be installed with the following command:

    	sudo apt install build-essential cmake libboost-all-dev libssl-dev pkg-config
    
* Change to the root of the source code directory and build:

        cd sumokoin
        make

    *Optional*: If your machine has several cores and enough memory, enable
    parallel build by running `make -j<number of threads>` instead of `make`. For
    this to be worthwhile, the machine should have one core and about 2GB of RAM
    available per thread.

* The resulting executables can be found in `build/release/bin`

* Add `PATH="$PATH:$HOME/sumokoin/build/release/bin"` to `.profile`

* Run Sumokoin with `sumokoind --detach`

* **Optional**: build and run the test suite to verify the binaries:

        make release-test

    *NOTE*: `coretests` test may take a few hours to complete.

* **Optional**: to build binaries suitable for debugging:

         make debug

* **Optional**: to build statically-linked binaries:

         make release-static

* **Optional**: build documentation in `doc/html` (omit `HAVE_DOT=YES` if `graphviz` is not installed):

        HAVE_DOT=YES doxygen Doxyfile

#### On the Raspberry Pi

Tested on a Raspberry Pi 2 with a clean install of minimal Debian Jessie from https://www.raspberrypi.org/downloads/raspbian/

* `apt-get update && apt-get upgrade` to install all of the latest software

* Install the dependencies for Sumokoin except libunwind and libboost-all-dev

* Increase the system swap size:
```	
	sudo /etc/init.d/dphys-swapfile stop  
	sudo nano /etc/dphys-swapfile  
	CONF_SWAPSIZE=1024  
	sudo /etc/init.d/dphys-swapfile start  
```
* Install the latest version of boost (this may first require invoking `apt-get remove --purge libboost*` to remove a previous version if you're not using a clean install):
```
	cd  
	wget https://sourceforge.net/projects/boost/files/boost/1.62.0/boost_1_62_0.tar.bz2  
	tar xvfo boost_1_62_0.tar.bz2  
	cd boost_1_62_0  
	./bootstrap.sh  
	sudo ./b2  
```
* Wait ~8 hours

	sudo ./bjam install

* Wait ~4 hours

* Change to the root of the source code directory and build:

        cd sumokoin
        make release

* Wait ~4 hours

* The resulting executables can be found in `build/release/bin`

* Add `PATH="$PATH:$HOME/sumokoin/build/release/bin"` to `.profile`

* Run Sumokoin with `sumokoind --detach`

* You may wish to reduce the size of the swap file after the build has finished, and delete the boost directory from your home directory

#### On Windows:

Binaries for Windows are built on Windows using the MinGW toolchain within
[MSYS2 environment](http://msys2.github.io). The MSYS2 environment emulates a
POSIX system. The toolchain runs within the environment and *cross-compiles*
binaries that can run outside of the environment as a regular Windows
application.

**Preparing the Build Environment**

* Download and install the [MSYS2 installer](http://msys2.github.io), either the 64-bit or the 32-bit package, depending on your system.
* Open the MSYS shell via the `MSYS2 Shell` shortcut
* Update packages using pacman:  

        pacman -Syuu  

* Exit the MSYS shell using Alt+F4  
* Edit the properties for the `MSYS2 Shell` shortcut changing "msys2_shell.bat" to "msys2_shell.cmd -mingw64" for 64-bit builds or "msys2_shell.cmd -mingw32" for 32-bit builds
* Restart MSYS shell via modified shortcut and update packages again using pacman:  

        pacman -Syuu  


* Install dependencies:

    To build for 64-bit Windows:

        pacman -S mingw-w64-x86_64-toolchain make mingw-w64-x86_64-cmake mingw-w64-x86_64-boost mingw-w64-x86_64-openssl

    To build for 32-bit Windows:
 
        pacman -S mingw-w64-i686-toolchain make mingw-w64-i686-cmake mingw-w64-i686-boost mingw-w64-i686-openssl

* Open the MingW shell via `MinGW-w64-Win64 Shell` shortcut on 64-bit Windows
  or `MinGW-w64-Win64 Shell` shortcut on 32-bit Windows. Note that if you are
  running 64-bit Windows, you will have both 64-bit and 32-bit MinGW shells.

**Building**

* Make sure path to MSYS2 installed directory in `Makefile` is correct.

* If you are on a 64-bit system, run:

        make release-static-win64

* If you are on a 32-bit system, run:

        make release-static-win32

* The resulting executables can be found in `build/mingw64/release/bin` or `build/mingw32/release/bin` accordingly.

### On FreeBSD:

* Update packages and install the dependencies (on FreeBSD 11.0 x64):
		
		pkg update; pkg install wget git pkgconf gcc49 cmake db6 icu libevent unbound googletest ldns expat bison boost-libs;

* Clone source code, change to the root of the source code directory and build:

        git clone https://github.com/sumoprojects/sumokoin; cd sumokoin; make release-static;


### On OpenBSD:

This has been tested on OpenBSD 5.8.

You will need to add a few packages to your system. `pkg_add db cmake gcc gcc-libs g++ miniupnpc gtest`.

The doxygen and graphviz packages are optional and require the xbase set.

The Boost package has a bug that will prevent librpc.a from building correctly. In order to fix this, you will have to Build boost yourself from scratch. Follow the directions here (under "Building Boost"):
https://github.com/bitcoin/bitcoin/blob/master/doc/build-openbsd.md

You will have to add the serialization, date_time, and regex modules to Boost when building as they are needed by Sumokoin.

To build: `env CC=egcc CXX=eg++ CPP=ecpp DEVELOPER_LOCAL_TOOLS=1 BOOST_ROOT=/path/to/the/boost/you/built make release-static-64`

### Building Portable Statically Linked Binaries

By default, in either dynamically or statically linked builds, binaries target the specific host processor on which the build happens and are not portable to other processors. Portable binaries can be built using the following targets:

* ```make release-static-64``` builds binaries on Linux on x86_64 portable across POSIX systems on x86_64 processors
* ```make release-static-32``` builds binaries on Linux on x86_64 or i686 portable across POSIX systems on i686 processors
* ```make release-static-armv8``` builds binaries on Linux portable across POSIX systems on armv8 processors
* ```make release-static-armv7``` builds binaries on Linux portable across POSIX systems on armv7 processors
* ```make release-static-armv6``` builds binaries on Linux portable across POSIX systems on armv6 processors
* ```make release-static-win64``` builds binaries on 64-bit Windows portable across 64-bit Windows systems
* ```make release-static-win32``` builds binaries on 64-bit or 32-bit Windows portable across 32-bit Windows systems

## Running sumokoind

The build places the binary in `bin/` sub-directory within the build directory
from which cmake was invoked (repository root by default). To run in
foreground:

    ./bin/sumokoind

To list all available options, run `./bin/sumokoind --help`.  Options can be
specified either on the command line or in a configuration file passed by the
`--config-file` argument.  To specify an option in the configuration file, add
a line with the syntax `argumentname=value`, where `argumentname` is the name
of the argument without the leading dashes, for example `log-level=1`.

To run in background:

    ./bin/sumokoind --log-file sumokoind.log --detach

To run as a systemd service, copy
[sumokoind.service](utils/systemd/sumokoind.service) to `/etc/systemd/system/` and
[sumokoind.conf](utils/conf/sumokoind.conf) to `/etc/`. The [example
service](utils/systemd/sumokoind.service) assumes that the user `sumokoin` exists
and its home is the data directory specified in the [example
config](utils/conf/sumokoind.conf).

If you're on Mac, you may need to add the `--max-concurrency 1` option to
sumo-wallet-cli, and possibly sumokoind, if you get crashes refreshing.

## Internationalization

Please see [README.i18n](README.i18n)

## Using Tor

While Sumokoin isn't made to integrate with Tor, it can be used wrapped with torsocks, if you add --p2p-bind-ip 127.0.0.1 to the sumokoind command line. You also want to set DNS requests to go over TCP, so they'll be routed through Tor, by setting DNS_PUBLIC=tcp. You may also disable IGD (UPnP port forwarding negotiation), which is pointless with Tor. To allow local connections from the wallet, you might have to add TORSOCKS_ALLOW_INBOUND=1, some OSes need it and some don't. Example:

`DNS_PUBLIC=tcp torsocks sumokoind --p2p-bind-ip 127.0.0.1 --no-igd`

or:

`DNS_PUBLIC=tcp TORSOCKS_ALLOW_INBOUND=1 torsocks sumokoind --p2p-bind-ip 127.0.0.1 --no-igd`

TAILS ships with a very restrictive set of firewall rules. Therefore, you need to add a rule to allow this connection too, in addition to telling torsocks to allow inbound connections. Full example:

`sudo iptables -I OUTPUT 2 -p tcp -d 127.0.0.1 -m tcp --dport 18081 -j ACCEPT`

`DNS_PUBLIC=tcp torsocks ./sumokoind --p2p-bind-ip 127.0.0.1 --no-igd --rpc-bind-ip 127.0.0.1 --data-dir /home/your/directory/to/the/blockchain`

`./sumo-wallet-cli`

## Using readline

While `sumokoind` and `sumo-wallet-cli` do not use readline directly, most of the functionality can be obtained by running them via `rlwrap`. This allows command recall, edit capabilities, etc. It does not give autocompletion without an extra completion file, however. To use rlwrap, simply prepend `rlwrap` to the command line, eg:

`rlwrap bin/sumo-wallet-cli --wallet-file /path/to/wallet`

Note: rlwrap will save things like your seed and private keys, if you supply them on prompt. You may want to not use rlwrap when you use simplewallet to restore from seed, etc.

# Debugging

This section contains general instructions for debugging failed installs or problems encountered with Sumokoin. First ensure you are running the latest version built from the github repo.

## LMDB

Instructions for debugging suspected blockchain corruption as per @HYC

There is an `mdb_stat` command in the LMDB source that can print statistics about the database but it's not routinely built. This can be built with the following command:

`cd ~/sumokoin/external/db_drivers/liblmdb && make`

The output of `mdb_stat -ea <path to blockchain dir>` will indicate inconsistencies in the blocks, block_heights and block_info table.

The output of `mdb_dump -s blocks <path to blockchain dir>` and `mdb_dump -s block_info <path to blockchain dir>` is useful for indicating whether blocks and block_info contain the same keys.

These records are dumped as hex data, where the first line is the key and the second line is the data.
