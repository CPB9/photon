hashdeep -c MD5 -l -o f -r ./acsl-photon-utils/opt/ > ./acsl-photon-utils/DEBIAN/md5sums
fakeroot dpkg-deb --build acsl-photon-utils
dpkg-name ./acsl-photon-utils.deb
