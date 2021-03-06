# OCC Faceter

OCC Faceter is a coolection of tools used for the production, modification and serialisation of geometries for [DAGMC](https://svalinn.github.io/DAGMC/)


### Installation

OCC Faceter has a number of dependencies, including [CGAL](https://cgal.org/), [MOAB](https://press3.mcs.anl.gov/sigma/moab-library/) and [OCC](https://www.opencascade.com).

To install MOAB and its dependencies:

```
sudo apt install libhdf5-dev build-essential gfortran autoconf libtool liblapack-dev
git clone https://bitbucket.org/fathomteam/moab.git
cd moab
autoreconf -fi
mkdir bld; cd bld
../configure --enable-optimize --enable-shared --with-hdf5=/usr/lib/x86_64-linux-gnu/hdf5/serial --prefix=/opt/moab
make -j4
make check
sudo mkdir /opt/moab
sudo chown $USER /opt/moab
make install
cd ../..
```

To install OCC:

```sh
sudo apt install software-properties-common # for add-apt-repository
sudo add-apt-repository ppa:freecad-maintainers/freecad-stable # for occ
sudo apt install libocct*-dev occt*
```

To install occ_faceter and remaining dependencies:

```
sudo apt install cmake libcgal-dev libtbb-dev
git clone https://github.com/makeclean/occ_faceter.git
cd occ_faceter
mkdir build
cd build
export LD_LIBRARY_PATH=/opt/moab/lib
cmake .. -DCMAKE_INSTALL_PREFIX=..
make -j4
make test
make install
```

### Usage

After installation the bin directory should contain occ_faceter and other tools. To use occ_faceter you will need a CAD geometry,
either in BREP format (output from PPP, with an associated JSON file), or a JSON list of STEP files (with associated materials).

The example below assumes you have two files which were output by PPP, `test.brep` and the associated `test_metadata.json`.
The default values are used for faceting tolerance (relative to edge size) and output filename.  Run `./occ_faceter --help` to view the options and their default values.

mbconvert is used to convert the new h5m file into an vtk file which can then be visualized in Paraview (or Visit).  occ_faceter's optional add_mat_ids flag is used, so that different materials can be given different colours in Paraview.

```
cd ../bin
./occ_faceter --add_mat_ids test.brep
mbconvert dagmc_not_watertight.h5m test.vtk
paraview test.vtk
```
