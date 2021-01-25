# Building Moxa Linux kernel ioThinx 4530 imx7d-linux-4.4 project

Below you will find instructions to build and install the imx7-linux-4.4 project.

## Install the Required Packages

To install these packages run the following command as root:

```
# sudo apt-get install unzip git build-essential bc kmod cpio flex libncurses5-dev libelf-dev libssl-dev lzop
```

## Install toolchain

To download the toolchain from [MOXA website](https://www.moxa.com/en/products/industrial-edge-connectivity/controllers-and-ios/advanced-controllers-and-i-os/iothinx-4530-series#resources) and install it.

```
# unzip moxa-iothinx-4530-series-tool-chain-utility-v1.2.zip
# sudo ./ioThinx_4530_toolchain_V1.2_Build19080715.sh
```

## Download source

To obtain the ioThinx 4530 imx7-linux-4.4 sources you must clone them as below:

```
# git clone https://github.com/MoxaCorp/iothinx-4530-imx7d-linux-4.4.git
# git clone https://github.com/MoxaCorp/xenomai.git
```

## Building

### Setup environment variables

To setup environment variables:

```
# cd iothinx-4530-imx7d-linux-4.4
# export PATH=$PATH:/usr/local/arm-linux-gnueabihf/bin
# export ARCH=arm
# export CROSS_COMPILE=arm-linux-gnueabihf-
# export INSTALL_MOD_PATH=`pwd`/modules_install_path
```

### Build kernel modules

Execute the following commands to build kernel modules:

```
# make ioThinx4530_defconfig
# make menuconfig
# make zImage dtbs -j4
# make modules
# make modules_install
```

Once build process complete, you can find `.ko` files under `./modules_install_path` directory.

## Updating

### Upload the kernel modules to the device

Find the target .ko and upload the file to the device.

### Insert the kernel modules

```
# insmod *.ko
```
