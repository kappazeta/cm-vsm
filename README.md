# cvat-vsm
A raster conversion and segmentation mask vectorization tool for CVAT.

## Dependencies
The following system dependencies are needed:
* g++8 or later
* CMake 3.4.3 or later
* cget (https://github.com/pfultz2/cget)
* OpenJPEG (`libopenjp2-7-dev`)
* GDAL (`libgdal-dev`, `gdal-bin`)
* Expat (`libexpat1-dev`)
* GraphicsMagick++ (`libgraphicsmagick++1-dev`)
* NetCDF (`libnetcdf-dev`)
* nlohmann's JSON (https://github.com/nlohmann/json)

On Ubuntu, the following commands could be used to install the dependencies:

```
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt-get install cmake gcc-9 g++-9 python-pip libopenjp2-7-dev libgdal-dev gdal-bin libexpat1-dev libgraphicsmagick++1-dev libnetcdf-dev
pip install cget
```

## Building
A release version of the tool can be built as follows:

```
cd vsm
cget build -DCMAKE_CXX_COMPILER=g++-9
```

Similarly, a debug version can be built as follows:

```
cd vsm
cget build -DCMAKE_CXX_COMPILER=g++-9
```

## Usage
With the `-d` option, `cvat-vsm` can be used to subtile a Sentinel-2 L2A raster image, for example:

```
./vsm/build/bin/cvat_vsm -d /your/data/path/S2A_MSIL2A_20200529T094041_N0214_R036_T35VLF_20200529T120441.SAFE
```

The image will be processed band by band, and it might take about an hour to process the whole raster.
The results would be stored in another directory with a suffix `.CVAT` instead of the original `.SAFE`.

A CVAT annotations XML file could be rasterized with the `-r` option.
This is to be performed after the successful subtiling of the raster image.
Rasterization of a labelled subtile 1024, 4864, for example:

```
./vsm/build/bin/cvat_vsm -r /your/data/path/S2A_MSIL2A_20200529T094041_N0214_R036_T35VLF_20200529T120441.CVAT/tile_1024_4864/annotations.xml -n /your/data/path/S2A_MSIL2A_20200529T094041_N0214_R036_T35VLF_20200529T120441.CVAT/tile_1024_4864/bands.nc
```

