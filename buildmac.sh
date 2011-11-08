#!/bin/bash
PRODUCT_NAME="samplecat"
VERSION=0.1.2+rg

GTKDIR=${GTKDIR:-"$HOME/gtk/inst"}
GTKLOADER=gtk-2.0/2.10.0/loaders
PANGOPATH=pango/1.6.0/modules

DMGFILE=/tmp/${PRODUCT_NAME}_v$VERSION.dmg
echo "building: $DMGFILE"

#NOREBUILD=1
#NOCLEAN=1

##############################################################################

if test -d $GTKDIR; then
	export PKG_CONFIG_PATH=$GTKDIR/lib/pkgconfig/:$PKG_CONFIG_PATH
else
	echo "no GTK installation-prefix was specified."
	exit;
fi

if test -z "$NOREBUILD"; then
  CFLAGS="-arch i386 -arch x86_64 -arch ppc" ./configure --disable-dependency-tracking
	make clean
  make || exit
fi

file src/.libs/samplecat
test -f src/.libs/samplecat || exit

##############################################################################
follow_dependencies () {
    libname=$1
    cd "${TARGET_BUILD_DIR}/${PRODUCT_NAME}.app/Contents/Frameworks"
    dependencies=`otool -arch all -L "$libname" | egrep '\/((opt|usr)\/local\/lib|gtk\/inst\/lib)' | awk '{print $1}'`
    for l in $dependencies; do
        depname=`basename $l`
        deppath=`dirname $l`
        if [ ! -f "${TARGET_BUILD_DIR}/${PRODUCT_NAME}.app/Contents/Frameworks/$depname" ]; then
            deploy_lib $depname "$deppath"
        fi
    done
}

update_links () {
    libname=$1
    libpath=$2
    for n in `ls ${LIBS_PATH}/*`; do
        install_name_tool \
            -change "$libpath/$libname" \
            @executable_path/../Frameworks/$libname \
            "$n"
    done
}

deploy_lib () {
    libname=$1
    libpath=$2
    check=`echo $INSTALLED | grep $libname`
    if [ "X$check" = "X" ]; then
        if [ ! -f "${TARGET_BUILD_DIR}/${PRODUCT_NAME}.app/Contents/Frameworks/$libname" ]; then
            cp -f "$libpath/$libname" "${TARGET_BUILD_DIR}/${PRODUCT_NAME}.app/Contents/Frameworks/$libname"
            install_name_tool \
                -id @executable_path/../Frameworks/$libname \
                "${TARGET_BUILD_DIR}/${PRODUCT_NAME}.app/Contents/Frameworks/$libname"
            follow_dependencies $libname
        fi
        export INSTALLED="$INSTALLED $libname"
    fi
    update_links $libname $libpath
}

update_executable() {
    echo "updating executable ${TARGET}"
    LIBS=`otool -arch all -L "$TARGET" | egrep '\/((opt|usr)\/local\/lib|gtk\/inst\/lib)' | awk '{print $1}'`
    for l in $LIBS; do
        libname=`basename $l`
        libpath=`dirname $l`
        deploy_lib $libname $libpath
        install_name_tool \
            -change $libpath/$libname \
            @executable_path/../Frameworks/$libname \
            "$TARGET"
    done
}
##############################################################################

echo "------------------------------------"

export TARGET_BUILD_DIR="$(pwd)/build"
export PRODUCT_NAME
export INSTALLED=""
export CONTENTS_PATH="$TARGET_BUILD_DIR/${PRODUCT_NAME}.app/Contents"
export LIBS_PATH="${CONTENTS_PATH}/Frameworks"
export TARGET_PATH="${CONTENTS_PATH}/MacOS"
export TARGET="${TARGET_PATH}/${PRODUCT_NAME}"
BGPIC=$(pwd)/icons/dmgbg.png
TOPDIR=$(pwd)
SRCDIR=${TOPDIR}/src

echo "BASEDIR: ${TARGET_BUILD_DIR}"
echo "TARGET : ${TARGET}"
echo "LIBDIR : ${LIBS_PATH}"

rm -rf "$TARGET_BUILD_DIR"
mkdir -p "${CONTENTS_PATH}/Resources/share/"
mkdir -p "$LIBS_PATH"
mkdir -p "$TARGET_PATH"

echo "------------------------------------"
echo "installing files"

cp -v ${TOPDIR}/icons/${PRODUCT_NAME}.icns ${CONTENTS_PATH}/Resources

cat > "${TARGET_BUILD_DIR}/${PRODUCT_NAME}.app/Contents/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleExecutable</key>
  <string>$PRODUCT_NAME</string>
  <key>CFBundleName</key>
  <string>$PRODUCT_NAME</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>CFBundleSignature</key>
  <string>RG42</string>
  <key>CFBundleIdentifier</key>
  <string>org.gareus.samplecat</string>
  <key>CFBundleVersion</key>
  <string>1.0</string>
  <key>CFBundleIconFile</key>
  <string>$PRODUCT_NAME</string>
  <key>CSResourcesFileMapped</key>
  <true/>
</dict>
</plist>
EOF

# install binaries
install -m 755 ${SRCDIR}/.libs/${PRODUCT_NAME} ${TARGET}
update_executable
#file "$TARGET"

echo "------------------------------------"
echo "installing dependent libs..."

cd $LIBS_PATH && MORELIBS=`otool -arch all -L * | egrep '\/((opt|usr)\/local\/lib|gtk\/inst\/lib)' | awk '{print $1}'` && cd - > /dev/null
test -n "${MORELIBS}" && MORELIBS="$MORELIBS "
while [ "X$MORELIBS" != "X" ]; do
    for l in $MORELIBS; do
        libname=`basename $l`
        libpath=`dirname $l`
        deploy_lib "$libname" "$libpath"
    done
    cd $LIBS_PATH && MORELIBS=`otool -arch all -L * | egrep '\/((opt|usr)\/local\/lib|gtk\/inst\/lib)'  | awk '{print $1}'` && cd - > /dev/null
    test -n "${MORELIBS}" && MORELIBS="$MORELIBS "
done

echo "------------------------------------"
echo "updating references in executable"
update_executable
echo "------------------------------------"
otool -arch all -L "$TARGET"

echo "------------------------------------"
echo "installing GTK/Pango modules"

mkdir -p "${CONTENTS_PATH}/Resources/etc/pango"
mkdir -p  ${LIBS_PATH}/${PANGOPATH}

mkdir -p ${CONTENTS_PATH}/Resources/etc/gtk-2.0/
mkdir -p ${LIBS_PATH}/${GTKLOADER}

cp -R ${GTKDIR}/lib/${PANGOPATH}/*.so ${LIBS_PATH}/${PANGOPATH}/
cp -R ${GTKDIR}/lib/${GTKLOADER}/*.so ${LIBS_PATH}/${GTKLOADER}/

if test -d "$HOME/data/gtk-shared/"; then
  cp -R $HOME/data/gtk-shared/mime "${CONTENTS_PATH}/Resources/share/"
  cp -R $HOME/data/gtk-shared/samplecat-icons ${CONTENTS_PATH}/Resources/share/icons
else
  cp -R $GTKDIR/share/mime "${CONTENTS_PATH}/Resources/share/"
  cp -R $GTKDIR/share/icons "${CONTENTS_PATH}/Resources/share/"
fi

# generate resource config files
cat > "${CONTENTS_PATH}/Resources/etc/pango/pango.rc" << EOF
[Pango]
ModuleFiles=../Resources/etc/pango/pango.modules
EOF

ESCAPEDGTKDIR="`echo \"$GTKDIR\" | sed 's/\\//\\\\\//g'`"

sed  's/'"$ESCAPEDGTKDIR"'\/lib/..\/Frameworks/g' \
  ${GTKDIR}/etc/pango/pango.modules > ${CONTENTS_PATH}/Resources/etc/pango/pango.modules

sed  's/'"$ESCAPEDGTKDIR"'\/lib/..\/Frameworks/g' \
  ${GTKDIR}/etc/gtk-2.0/gdk-pixbuf.loaders > ${CONTENTS_PATH}/Resources/etc/gtk-2.0/gdk-pixbuf.loaders

# change symbols links of plugins
for n in ${LIBS_PATH}/${PANGOPATH}/*.so; do
	for m in `otool -L $n | grep "gtk/inst" | awk '{print $1}'`; do
		BN=`basename $m`;
		install_name_tool -change "$m" "@executable_path/../Frameworks/$BN" "$n";
	done
done

for n in ${LIBS_PATH}/${GTKLOADER}/*.so; do
	for m in `otool -L $n | grep "gtk/inst" | awk '{print $1}'`; do
		BN=`basename $m`;
		install_name_tool -change "$m" "@executable_path/../Frameworks/$BN" "$n";
	done
done


echo "------------------------------------"
echo "generate startup-script"
mv -vf ${TARGET} ${TARGET}-bin
cat > "${TARGET}" << EOF
#!/bin/bash
CWD="\`/usr/bin/dirname \"\$0\"\`"
export XDG_DATA_HOME="\$CWD/../Resources/share/"
export PANGO_RC_FILE="\$CWD/../Resources/etc/pango/pango.rc"
export GDK_PIXBUF_MODULE_FILE="\$CWD/../Resources/etc/gtk-2.0/gdk-pixbuf.loaders"
cd "\$CWD"
exec "\$CWD/${PRODUCT_NAME}-bin"
EOF

chmod +x "${TARGET}"

##############################################################################
#roll a DMG
echo "------------------------------------"
echo "generating DMG.."
TMPFILE=/tmp/sctmp.dmg
MNTPATH=/tmp/mnt/
VOLNAME=samplecat
APPNAME="${PRODUCT_NAME}.app"

mkdir -p $MNTPATH
if [ -e $TMPFILE -o -e $DMGFILE -o ! -d $MNTPATH ]; then
  echo
  echo "could not create DMG. tmp-file or destination file exists."
  echo "please clean up previous builds:"
	ls -dl $TMPFILE $DMGFILE $MNTPATH
  exit;
fi


hdiutil create -megabytes 200 $TMPFILE
DiskDevice=$(hdid -nomount "${TMPFILE}" | grep Apple_HFS | cut -f 1 -d ' ')
newfs_hfs -v "${VOLNAME}" "${DiskDevice}"
mount -t hfs "${DiskDevice}" "${MNTPATH}"

cp -R ${TARGET_BUILD_DIR}/${PRODUCT_NAME}.app ${MNTPATH}/
mkdir ${MNTPATH}/.background
BGFILE=$(basename $BGPIC)
cp -vi ${BGPIC} ${MNTPATH}/.background/${BGFILE}

echo '
   tell application "Finder"
     tell disk "'${VOLNAME}'"
	   open
	   set current view of container window to icon view
	   set toolbar visible of container window to false
	   set statusbar visible of container window to false
	   set the bounds of container window to {400, 200, 800, 440}
	   set theViewOptions to the icon view options of container window
	   set arrangement of theViewOptions to not arranged
	   set icon size of theViewOptions to 64
	   set background picture of theViewOptions to file ".background:'${BGFILE}'"
	   make new alias file at container window to POSIX file "/Applications" with properties {name:"Applications"}
	   set position of item "'${APPNAME}'" of container window to {100, 100}
	   set position of item "Applications" of container window to {310, 100}
	   close
	   open
	   update without registering applications
	   delay 5
	   eject
     end tell
   end tell
' | osascript

sync

# Umount the image
umount "${DiskDevice}"
hdiutil eject "${DiskDevice}"

# Create a read-only version, use zlib compression
hdiutil convert -format UDZO "${TMPFILE}" -imagekey zlib-level=9 -o "${DMGFILE}"

# Delete the temporary files
if [ -z "$KEEPTMP" ]; then
  rm $TMPFILE
fi
rmdir $MNTPATH

echo
echo "packaging suceeded."
if [ -n "$KEEPTMP" ]; then
 ls -l $TMPFILE
fi
ls -l $DMGFILE
ls -lh $DMGFILE

#/bin/echo -n " upload?[Enter|CTRL-C]"
#read 
#echo "rsync -Pc $DMGFILE rg42.org:/home/data/download/"
#rsync -Pc $DMGFILE rg42.org:/home/data/download/
