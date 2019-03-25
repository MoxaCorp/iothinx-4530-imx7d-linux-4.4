#!/bin/bash
set -o errexit
#source env.sh
#make ioThinx4530_defconfig
make zImage dtbs -j4
./its/imx7d-moxa-iothinx-4530
