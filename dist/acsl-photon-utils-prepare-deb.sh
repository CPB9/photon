build_path=../build
dest_path=/opt/acsl/
pkg_path=./acsl-photon-utils/$dest_path

mkdir -p $pkg_path/bin/
cp ../build/photon-proxy $pkg_path/bin/
cp ../build/photon-client $pkg_path/bin/
cp ../build/photon-client-serial $pkg_path/bin/
cp ../build/photon-client-udp $pkg_path/bin/
strip -s $pkg_path/bin/*

