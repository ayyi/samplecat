#!/bin/sh

DEBRELEASE=$(head -n1 debian/changelog | cut -d ' ' -f 2 | sed 's/[()]*//g')

TMPDIR=/tmp/samplecat-${DEBRELEASE}
rm -rf ${TMPDIR}

git-buildpackage \
	--git-upstream-branch=master --git-debian-branch=master \
	--git-upstream-tree=branch \
	--git-export-dir=${TMPDIR} \
	--git-force-create \
	-rfakeroot $@ \
	|| exit

lintian -i --pedantic ${TMPDIR}/samplecat_${DEBRELEASE}_*.changes \
	| tee /tmp/samplecat.issues
