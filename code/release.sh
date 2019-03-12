#!/bin/sh

FRC_BUILD_VERSION=`./build_version.sh $*`

if [ $? != 0  ]; then
    echo "ERROR: $FRC_BUILD_VERSION"
    exit 1
fi


export FRC_BUILD_VERSION=$FRC_BUILD_VERSION

echo 

VERSION=V1.01_"$FRC_BUILD_VERSION"

FRC_PREFIX="barley_$VERSION" 

echo "FRC_BUILD_VERSION=$FRC_BUILD_VERSION, VERSION = $VERSION"

cd $OCTDRV
make clean
make
cp $OCTDRV/Module.symvers $FRCDRV
cd $FRCDRV
make clean
make 
cd $FRCORE
make clean
make 
cd $FRCAPI
make clean
make 
cd $FRCTWEAK
make clean
make 
cd $FRCDIR

rm -rf $FRC_PREFIX 
mkdir $FRC_PREFIX 

cp -rf  $OCTDRV/octeon_drv.ko   $FRC_PREFIX
cp -rf  include                 $FRC_PREFIX
cp -rf  frcapi			$FRC_PREFIX
rm -rf  $FRC_PREFIX/frcapi/frcapi.a
rm -rf  $FRC_PREFIX/frcapi/frc_api.o
cp -rf  frcdrv/frcdrv.ko        $FRC_PREFIX
cp -rf  frcore/frcore.strip     $FRC_PREFIX
cp -rf  frctweak/frctweak       $FRC_PREFIX/barley
#cp -rf  frlooper/frlooper       $FRC_PREFIX
cp -rf  bin/frc_load.sh         $FRC_PREFIX
cp -rf  bin/frc_unload.sh       $FRC_PREFIX
cp -rf  bin/oct-pci-bootcmd     $FRC_PREFIX
cp -rf  bin/oct-pci-load        $FRC_PREFIX
cp -rf  bin/oct-remote-bootcmd  $FRC_PREFIX
cp -rf  bin/oct-remote-load     $FRC_PREFIX
cp -rf  bin/pcie_control_reset.sh $FRC_PREFIX
cp -rf  bin/recovery.sh         $FRC_PREFIX
cp -rf  bin/README.txt          $FRC_PREFIX
#cp -rf  tools/loop.sh           $FRC_PREFIX
#cp -rf  tools/update_mac.sh     $FRC_PREFIX

chmod 777 $FRC_PREFIX/* 

FILE_NAME="../release/"$FRC_PREFIX".tar.gz"
mkdir -p ../release
tar czvf $FILE_NAME $FRC_PREFIX --exclude ".svn"
echo "FILE_NAME = $FILE_NAME"
rm -rf $FRC_PREFIX

