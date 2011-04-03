case $# in
0)
    if [ ! -r mp4v2/GNUmakefile ]
    then
	echo 'Specify toolset prefix like "i686-w64-mingw32-", or make mp4v2 first' 1>&2
    else
	eval `sed -n '/^CC =/ s/ //gp' mp4v2/GNUmakefile`
	eval `sed -n '/^CXX =/ s/ //gp' mp4v2/GNUmakefile`
    fi
    ;;
*)
    CC=${1}gcc
    CXX=${1}g++
    ;;
esac

echo "Setting \$CC to ${CC}"
echo "Setting \$CXX to ${CXX}"
export CC
export CXX
