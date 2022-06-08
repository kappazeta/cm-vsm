# cm-vsm
A raster conversion and segmentation mask vectorization tool for KappaZeta cloudmask.

## Dependencies
The following system dependencies are needed:
* g++8 or later
* CMake 3.4.3 or later
* cget (https://github.com/pfultz2/cget)
* OpenJPEG (`libopenjp2-7-dev`)
* GDAL (`libgdal-dev`, `gdal-bin`, `gdal-data`)
* Expat (`libexpat1-dev`)
* GraphicsMagick++ (`libgraphicsmagick++1-dev`)
* NetCDF (`libnetcdf-dev`)
* nlohmann's JSON (https://github.com/nlohmann/json)
* CppUnit (`libcppunit-dev`)
* Doxygen (for documentation only)

On Ubuntu, the following commands could be used to install the dependencies:

```
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt-get install cmake gcc-9 g++-9 python3-pip libgdal-dev gdal-bin libexpat1-dev libgraphicsmagick++1-dev
pip3 install cget
```

Under CentOS-8, run

```
sudo yum install cmake gcc gcc-c++ python3-pip openjpeg2-devel libpng-devel gdal gdal-devel expat-devel GraphicsMagick-c++-devel netcdf-devel libjpeg-turbo-devel jbigkit-libs jbigkit-devel m4
```

Make sure that your `GDAL_DATA` environment variable has been set, according to your GDAL version, which is indicated by the placeholder `YOUR_GDAL_VERSION` below:

```
GDAL_DATA=/usr/share/gdal/YOUR_GDAL_VERSION
```

## Building

Ensure that there is no active Conda environment (except `base`) while building cm-vsm.

A release version of the tool can be built as follows:

```
cd vsm/build
cget install
cmake .. -DCMAKE_CXX_COMPILER=g++-9
make
sudo make install
```

Similarly, a debug version can be built as follows:

```
cd vsm/build
cget install
cmake .. -DCMAKE_CXX_COMPILER=g++-9 -DCMAKE_BUILD_TYPE=Debug
make
sudo make install
```

Once CMake has already been run in the build directory, the documentation can be built as follows:

```
cd vsm/build
make doc
```

## Building in Visual Studio Code (Ubuntu Linux)
For the program to run additional packages from the "Extensions" tab are requred:
* C/C++ (Microsoft)
* CMake Tools (Microsoft)
* CMake (twxs)

Using terminal navigate to `~/cm-vsm/vsm/build` and run:
```
cget install
```

To configure the project press `Ctrl + Shift + P` in Visual Studio Code and run:
```
>CMake: Configure
```
If CMake cannot find `CMakeLists.txt`, navigate to it manually or reopen vsm folder as a project.

To build the project press `Ctrl + Shift + P` in Visual Studio Code and run:
```
>CMake: Build
```

To debug the project set a breakpoint, press `Ctrl + Shift + P` in Visual Studio Code and run:
```
>CMake: Debug
```

To avoid #include errors caused by IntelliSense, press `Ctrl + Shift + P` in Visual Studio Code and run:
```
C/C++: Edit configurations (UI)
```
In the configuration window specify an additional include path:
```
/usr/include/GraphicsMagick
```
and select C++17 as a C++ standard down to the bottom.

## Usage
With the `-d` option, `cvat-vsm` can be used to subtile a Sentinel-2 L2A raster image, for example:

```
./vsm/build/bin/cvat_vsm -d /your/data/path/S2A_MSIL2A_20200529T094041_N0214_R036_T35VLF_20200529T120441.SAFE
```

The image will be processed band by band, and it might take about an hour to process the whole raster.
In order to limit processing to specific bands, the `-b comma,separated,list,of,band,names` argument could be used.
The results would be stored in another directory with a suffix `.CVAT` instead of the original `.SAFE`.

A CVAT annotations XML file could be rasterized with the `-r` option.
This is to be performed after the successful subtiling of the raster image.
Rasterization of a labelled subtile 2, 3, for example:

```
./vsm/build/bin/cvat_vsm -r /your/data/path/S2A_MSIL2A_20200529T094041_N0214_R036_T35VLF_20200529T120441.CVAT/tile_2_3/annotations.xml -n /your/data/path/S2A_MSIL2A_20200529T094041_N0214_R036_T35VLF_20200529T120441.CVAT/tile_2_3/T35VLF_tile_2_3.nc
```

## Supported bands
The following bands / layers are supported:

| No. | Name       | Ground resolution | Bit-depth | Format    | Description                                                 |
| --- | ---------- | ----------------- | --------- | --------- | ----------------------------------------------------------- |
|     |      Label |              10 m |         8 |      Mask | Segmentation mask from one of the supported labeling tools  |
|   1 |        TCI |              10 m |         8 |       RGB | True Color Image                                            |
|   2 |        SCL |              20 m |         8 |      Mask | Sen2Cor Scene Classification Image                          |
|   3 |        AOT |              10 m |        16 | Grayscale | Aerosol Optical Thickness map                               |
|   4 |        B01 |              60 m |        16 | Grayscale | 433 - 453 nm                                                |
|   5 |        B02 |              10 m |        16 | Grayscale | 457.5 - 522.5 nm                                            |
|   6 |        B03 |              10 m |        16 | Grayscale | 542.5 - 577.5 nm                                            |
|   7 |        B04 |              10 m |        16 | Grayscale | 650 - 680 nm                                                |
|   8 |        B05 |              20 m |        16 | Grayscale | 697.5 - 712.5 nm                                            |
|   9 |        B06 |              20 m |        16 | Grayscale | 732.5 - 747.5 nm                                            |
|  10 |        B07 |              20 m |        16 | Grayscale | 773 - 793 nm                                                |
|  11 |        B08 |              10 m |        16 | Grayscale | 784.5 - 899.5 nm                                            |
|  12 |        B8A |              20 m |        16 | Grayscale | 855 - 875 nm                                                |
|  13 |        B09 |              60 m |        16 | Grayscale | 935 - 955 nm                                                |
|  14 |        B10 |              60 m |        16 | Grayscale | 1358 - 1389 nm                                              |
|  15 |        B11 |              20 m |        16 | Grayscale | 1565 - 1655 nm                                              |
|  16 |        B12 |              20 m |        16 | Grayscale | 2100 - 2280 nm                                              |
|  17 |        WVP |              10 m |        16 | Grayscale | Water Vapour map                                            |
|  18 |        GML |                   |           |    Vector | GML vector mask                                             |
|  19 |       S2CC |              20 m |         8 |      Mask | Sen2Cor cloud probabilities                                 |
|  20 |       S2CS |              20 m |         8 |      Mask | Sen2Cor snow probabilities                                  |
|  21 |        FMC |              20 m |         8 |      Mask | Fmask classification map                                    |
|  22 |       SS2C |              60 m |         8 |      Mask | Sinergise S2Cloudless classification map                    |
|  23 |      SS2CC |              60 m |         8 |      Mask | Sinergise S2Cloudless cloud probabilities                   |
|  24 |      MAJAC |              10 m |         8 |      Mask | CNES MAJA cloud classification map                          |
|  25 |        BHC |              60 m |         8 |      Mask | Baetens & Hagolle classification map                        |
|  26 |       FMSC |              20 m |         8 |      Mask | Francis, Mrziglod, and Sidiropoulos classification map      |
|  27 |       GSFC |              10 m |         8 |      Mask | NASA GSFC Vector mask converted into 10 m resolution raster |
|  28 | DL-L8S2-UV |              10 m |         8 |      Mask | IPL-UV DL-L8S2-UV rgbiswir classification mask              |
