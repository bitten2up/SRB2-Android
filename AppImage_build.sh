#!/bin/bash
set -e

# Make Linux AppImage program data
# https://docs.appimage.org/reference/appdir.html

# PWD: {repo_root}/build
# See deployer.sh for usage

RESET="\e[0m"
FAILURE="\e[91m"
SUCCESS="\e[92m"
NOTICE="\e[93m"
VERBOSE="\e[94m"

verbosity=0 # By default don't print anything unless it's *ultra* important.
quiet=0 # Also don't print anything if they supplied the quiet argument

function print { ((quiet == 0)) && echo "$@" || :; } # echo if not quiet
function coloredText { printf $1; print "${@:2}"; printf $RESET; } # use print to inherit quiet
function success { coloredText $SUCCESS "$@"; } # use coloredText to inherit quiet
function important { coloredText $NOTICE "$@"; } # use coloredText to inherit quiet
function verboseOnly { (( verbosity >= $1 )) && (coloredText $VERBOSE "${@:2}") || :; } # use coloredText to inherit quiet

# Parses an argument with a single dash
function parseSingleDashArg() {
	while read -n 1 char; do
		if [[ $char == '-' || $char == '' ]]; then
			:
		elif [[ $char == 'h' ]]; then
			helppassed=1
		elif [[ $char == 'q' ]]; then
			quiet=1
		elif [[ $char == 'v' ]]; then
			verbosity=$((verbosity + 1))
		elif [[ $char == 'd' ]]; then
			dry=1
		elif [[ $char == 's' ]]; then
			small=1
		else
			echo "$0: invalid option -- '$1'"
			echo "Try '$0 --help' for more information."
			return 1
		fi
	done <<<"$1"
}

# Simple help argument checker.
currentarg=1; # Which argument is --help...
for arg in "$@"; do # For $arg in the argument array...
	if [[ $arg == "--help" ]]; then # Is it a help argument?
		helppassed=1; # Help is passed, set $helppassed!
		set -- "${@:1:$currentarg-1}" "${@:$currentarg+1}"; # Remove argument from list
	elif [[ $arg == "--quiet" ]]; then
		quiet=1
		set -- "${@:1:$currentarg-1}" "${@:$currentarg+1}"; # Remove argument from list
	elif [[ $arg == "--verbose" ]]; then
		verbosity=$((verbosity + 1)) # Increase verbosity!
		set -- "${@:1:$currentarg-1}" "${@:$currentarg+1}"; # Remove argument from list
	elif [[ $arg == "--dry" ]]; then
		dry=1; # Show parameters instead of using them.
		set -- "${@:1:$currentarg-1}" "${@:$currentarg+1}"; # Remove argument from list
	elif [[ $arg == "--small" ]]; then
		small=1; # Don't copy assets to save on bandwidth.
		set -- "${@:1:$currentarg-1}" "${@:$currentarg+1}"; # Remove argument from list
	elif [[ ! $arg == *'--'* && $arg == *'-'* ]]; then
		parseSingleDashArg $arg
		if [[ $? == 0 ]]; then
			set -- "${@:1:$currentarg-1}" "${@:$currentarg+1}"; # Remove argument from list
		fi
	elif [[ $arg == *'--'* ]]; then
		print "$0: invalid option -- '$1'"
		print "Try '$0 --help' for more information."
		return 1;
	else
		currentarg=$((currentarg+1)); # Increment current argument number otherwise...
	fi
done

important "SRB2 AppImage Packager v1.0, by mazmazz and Golden."

# Get root directory from the full path of this script.
__ROOT_DIR=${3:-$(dirname $(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)/$(basename "${BASH_SOURCE[0]}"))}

# If it starts with '.', put the current working directory behind it so things work.
if [[ "$__ROOT_DIR" == '.' || "$__ROOT_DIR" == '..' ]]; then
	__ROOT_DIR=$PWD/$__ROOT_DIR
fi

__PROGRAM_NAME=${PROGRAM_NAME:-Sonic Robo Blast 2}
__PROGRAM_DESCRIPTION=${PROGRAM_DESCRIPTION:-A 3D Sonic the Hedgehog fangame inspired by the original Sonic games on the Sega Genesis.}
__PROGRAM_FILENAME=${PROGRAM_FILENAME:-lsdl2srb2}
__PROGRAM_ASSETS=${PROGRAM_ASSETS:-$__ROOT_DIR/assets/installer}

__BUILD_DIR=${1:-$PWD}
__OUTPUT_FILENAME=${2:-$__PROGRAM_FILENAME.AppImage}

if [[ ! -z $4 ]]; then
	__APPIMAGETOOL="$4" # appimagetool will be automatically downloaded if the path isn't given
fi

# Stuff that prints and exits.

# Prints setup text using method given by arguments.
# Example: printSetup "print" # print "Setup for AppImage...
function printSetup {
	$@ "Setup for AppImage in BUILD_DIR: '$__BUILD_DIR'..."
	$@ "With the OUTPUT_FILENAME: '$__OUTPUT_FILENAME'"
	$@ "And SRB2 Repository Root ROOT_DIR: '$__ROOT_DIR'"
	$@ "Using APPIMAGETOOL: '$([ -z "$__APPIMAGETOOL" ] && echo '(download)' || echo $__APPIMAGETOOL)'"
	$@ ""
	$@ "This should be run from a Makefile or from the directory of the build. If it is not, then change to that directory or specify that directory as an argument."
	$@ "If ROOT_DIR isn't SRB2's repository root, specify that as an argument too."
	$@ "Make must have built the program before you run this script."
	$@ ""
	$@ "Make sure these Environment Variables are correct."
	$@ "If not, then enter: export PROGRAM_VARIABLE=value"
	$@ "PROGRAM_NAME: $__PROGRAM_NAME"
	$@ "PROGRAM_DESCRIPTION: $__PROGRAM_DESCRIPTION"
	$@ "PROGRAM_FILENAME: $__PROGRAM_FILENAME"
	$@ "PROGRAM_ASSETS: $__PROGRAM_ASSETS"
}

if [[ $helppassed ]]; then
	print "Usage: $(basename "$0") [OPTION]... [BUILD_DIR] [OUTPUT_NAME] [ROOT_DIR] [APPIMAGETOOL]"
	print "Packages a SRB2 binary into an AppImage."
	print "OPTIONs may be included anywhere within the arguments."
	print ""
	print "Arguments:"
	print "    -h, --help              display this help and exit."
	print "    -q, --quiet             don't display any text."
	print "    -v, --verbose           make commands more verbose, will always print."
	print "    -s, --small             don't copy SRB2 assets to save on bandwidth."
	print "                            parameters regardless of --dry."
	print "    -d, --dry               print parameters and exit."
	print ""
	if (( $verbosity > 0 )) || [[ $dry ]]; then
		printSetup "print"
	else
		print "This should be run from a Makefile or from the directory of the build."
		print "Make must have built the program before you run this script."
	fi
	exit;
fi

if (( $verbosity > 0 )) || [[ $dry ]]; then
	printSetup "verboseOnly" "0"
	verboseOnly 0 ""

	if [[ $dry ]]; then
		set -n; # Enable debug mode and verbose for dry run.
	fi
fi

# End stuff that prints and exits.

if (( $verbosity >= 4 )); then
	set -v; # Really verbose? Print *everything*!
fi

# Define AppDir structure
mkdir -p $__BUILD_DIR/AppDir/usr/bin
mkdir -p $__BUILD_DIR/AppDir/lib
mkdir -p $__BUILD_DIR/AppDir/usr/share/applications
mkdir -p $__BUILD_DIR/AppDir/usr/share/icons/hicolor/256x256/apps

# Copy program data
if [[ ! $small ]]; then
	verboseOnly 1 "Packaging program assets..."
	cp -r $__PROGRAM_ASSETS/* $__BUILD_DIR/AppDir/usr/bin/
	cp -r $__BUILD_DIR/$__PROGRAM_FILENAME $__BUILD_DIR/AppDir/usr/bin
else
	verboseOnly 1 "Skipping program asset packing, --small or -s passed..."
fi

verboseOnly 1 "Packaging SRB2..."
verboseOnly 1 "Assuming executable name $__PROGRAM_FILENAME"
cp -r $__BUILD_DIR/$__PROGRAM_FILENAME $__BUILD_DIR/AppDir/usr/bin

# Copy required dependencies, but only if the program is dynamically linked.
verboseOnly 1 "Testing if this build is dynamically linked..."
set +e # Disable auto-exit for ldd.
ldd $__BUILD_DIR/$__PROGRAM_FILENAME >> /dev/null
exitcode=$?
set -e
if (( exitcode == 0 )); then
	verboseOnly 1 "This build *is* dynamically linked! Continuing with Python script."
	__LDD_LIST=$(python3 "$__ROOT_DIR/AppImage_prunedepends.py" "$__BUILD_DIR/$__PROGRAM_FILENAME")
	IFS=' ' read -r -a paths <<< "$__LDD_LIST"
	for path in "${paths[@]}";
	do
	    if [ -f "$path" ]; then
	        verboseOnly 2 "Packaging dependency $(basename $path)...";
	        cp "$path" $__BUILD_DIR/AppDir/lib/;
	    else
	        verboseOnly 2 "Dependency $(basename $path) not found";
	    fi;
	done;
else
	verboseOnly 1 "This SRB2 build is statically linked, skipping dependency packing."
fi

cd $__BUILD_DIR/AppDir

# Copy icons
verboseOnly 1 "Packaging resources..."
cp $__ROOT_DIR/srb2.png ./usr/share/icons/hicolor/256x256/apps/$__PROGRAM_FILENAME.png
ln -sf ./usr/share/icons/hicolor/256x256/apps/$__PROGRAM_FILENAME.png ./.DirIcon
ln -sf ./usr/share/icons/hicolor/256x256/apps/$__PROGRAM_FILENAME.png ./$__PROGRAM_FILENAME.png

# Make desktop descriptor
cat > ./usr/share/applications/$__PROGRAM_FILENAME.desktop <<EOF
[Desktop Entry]
Type=Application
Name=${__PROGRAM_NAME}
Comment=${__PROGRAM_DESCRIPTION}
Icon=${__PROGRAM_FILENAME}
Exec=AppRun %F
Categories=Game;
EOF
ln -sf ./usr/share/applications/$__PROGRAM_FILENAME.desktop ./$__PROGRAM_FILENAME.desktop

# Make entry point
echo -e \#\!$(dirname $SHELL)/sh > ./AppRun
echo -e 'HERE="$(dirname "$(readlink -f "${0}")")"' >> ./AppRun
echo -e 'SRB2WADDIR=$HERE/usr/bin LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HERE/lib exec $HERE/usr/bin/'$__PROGRAM_FILENAME' "$@"' >> ./AppRun
chmod +x ./AppRun

cd ..

# Package AppImage
if [ -z "$__APPIMAGETOOL" ] || (! command -v $__APPIMAGETOOL &> /dev/null); then
	verboseOnly 1 "No valid appimagetool path given, downloading appimagetool..."

	# Print notice for internet connection.
	important "If the command hangs here, check your internet connection, or download appimagetool and add it to your \$PATH and try compiling again."

	url="https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
	(( verbosity >= 3 )) && wget $url || wget -q $url

	APPIMAGETOOL=./appimagetool-x86_64.AppImage
	chmod a+x $APPIMAGETOOL
else
	verboseOnly 1 "appimagetool path given, no download required."
	APPIMAGETOOL=$__APPIMAGETOOL
fi

verboseOnly 1 "Packing AppImage $__OUTPUT_FILENAME"

if (( verbosity >= 3 )); then
	$APPIMAGETOOL ./AppDir $__OUTPUT_FILENAME
elif (( verbosity >= 2 )); then
	$APPIMAGETOOL ./AppDir $__OUTPUT_FILENAME >/dev/null
else
	$APPIMAGETOOL ./AppDir $__OUTPUT_FILENAME >/dev/null 2>&1
fi

success "AppImage ready!"
verboseOnly 1 "Cleaning up files..."
rm -r ./AppDir

if [ -z "$__APPIMAGETOOL" ]; then
	rm ./appimagetool-x86_64.AppImage
fi

cd $__ROOT_DIR