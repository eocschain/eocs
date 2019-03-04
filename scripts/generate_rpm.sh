#! /bin/bash

<<<<<<< HEAD
NAME="${PROJECT}-${VERSION}.x86_64"
=======
>>>>>>> otherb
PREFIX="usr"
SPREFIX=${PREFIX}
SUBPREFIX="opt/${PROJECT}/${VERSION}"
SSUBPREFIX="opt\/${PROJECT}\/${VERSION}"
<<<<<<< HEAD
=======
RELEASE="${VERSION_SUFFIX}"

# default release to "1" if there is no suffix
if [[ -z $RELEASE ]]; then
  RELEASE="1"
fi

NAME="${PROJECT}-${VERSION_NO_SUFFIX}-${RELEASE}"
>>>>>>> otherb

export PREFIX
export SUBPREFIX
export SPREFIX
export SSUBPREFIX

bash generate_tarball.sh ${NAME}.tar.gz

<<<<<<< HEAD
RPMBUILD=`realpath ~/rpmbuild/BUILDROOT/${NAME}-0.x86_64`
=======
RPMBUILD=`realpath ~/rpmbuild/BUILDROOT/${NAME}.x86_64`
>>>>>>> otherb
mkdir -p ${RPMBUILD} 
FILES=$(tar -xvzf ${NAME}.tar.gz -C ${RPMBUILD})
PFILES=""
for f in ${FILES[@]}; do
  if [ -f ${RPMBUILD}/${f} ]; then
    PFILES="${PFILES}/${f}\n"
  fi
done
echo -e ${PFILES} &> ~/rpmbuild/BUILD/filenames.txt

mkdir -p ${PROJECT} 
echo -e "Name: ${PROJECT} 
<<<<<<< HEAD
Version: ${VERSION}.x86_64
License: MIT
Vendor: ${VENDOR} 
Source: ${URL} 
Requires: openssl-devel.x86_64, gmp-devel.x86_64, libstdc++-devel.x86_64, bzip2.x86_64, bzip2-devel.x86_64, mongodb.x86_64, mongodb-server.x86_64
URL: ${URL} 
Packager: ${VENDOR} <${EMAIL}>
Summary: ${DESC}
Release: 0
=======
Version: ${VERSION_NO_SUFFIX}
License: MIT
Vendor: ${VENDOR} 
Source: ${URL} 
Requires: openssl-devel, gmp-devel, libstdc++-devel, bzip2, bzip2-devel, mongodb, mongodb-server
URL: ${URL} 
Packager: ${VENDOR} <${EMAIL}>
Summary: ${DESC}
Release: ${RELEASE}
>>>>>>> otherb
%description
${DESC}
%files -f filenames.txt" &> ${PROJECT}.spec

rpmbuild -bb ${PROJECT}.spec
<<<<<<< HEAD
mv ~/rpmbuild/RPMS/x86_64 ./
rm -r ${PROJECT} ~/rpmbuild/BUILD/filenames.txt ${PROJECT}.spec
=======
BUILDSTATUS=$?
mv ~/rpmbuild/RPMS/x86_64 ./
rm -r ${PROJECT} ~/rpmbuild/BUILD/filenames.txt ${PROJECT}.spec

exit $BUILDSTATUS
>>>>>>> otherb
