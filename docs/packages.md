# Installation using Package Managers

## conan

You can download and install glog using the [conan](https://conan.io)
package manager:

``` bash
pip install conan
conan install -r conancenter glog/<glog-version>@
```

The glog recipe in conan center is kept up to date by conan center index
community contributors. If the version is out of date, please create an
issue or pull request on the
[conan-center-index](https://github.com/conan-io/conan-center-index)
repository.

## vcpkg

You can download and install glog using the
[vcpkg](https://github.com/Microsoft/vcpkg) dependency manager:

``` bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
./vcpkg install glog
```

The glog port in vcpkg is kept up to date by Microsoft team members and
community contributors. If the version is out of date, please create an
issue or pull request on the vcpkg repository.
