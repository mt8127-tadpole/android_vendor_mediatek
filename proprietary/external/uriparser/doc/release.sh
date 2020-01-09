#! /usr/bin/env bash
(
cd $(dirname $(which "$0")) || exit 1

distdir="uriparser-0.7.7-doc"
[ -z $MAKE ] && MAKE=make

# Clean up
rm -Rf "${distdir}" "${distdir}.zip"

# Generate
"${MAKE}" || exit 1

# Copy
mkdir -p "${distdir}/html"
cp \
	html/*.css \
	html/*.html \
	html/*.png \
	\
	"${distdir}/html/" || exit 1

# Package
zip -r "${distdir}.zip" "${distdir}" || exit 1

cat <<INFO
=================================================
${distdir} archives ready for distribution:
${distdir}.zip
=================================================

INFO

# Clean up
rm -Rf "${distdir}"

)
exit $?
