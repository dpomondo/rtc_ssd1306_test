#! /bin/bash
# we need to invoke bash to run this, as regular old POSIX shell has no arrays

# to get this where we need to go:
#       cd build/
#       cp ${PERSONAL_PICO_PROJECTS}/shared/open.sh .
# 

# shopt turns on a shell option, nullglob ensures that if globbing returns
# nothing, the array won't contain the `*.elf` pattern itself
shopt -s nullglob
declare -a elf_files
elf_files=(*.elf)
shopt -u nullglob   # turn off the nullglob!
echo "Info: total number of valid files: ${#elf_files[@]}: ${elf_files[@]}"

# how many options specified on the command line? Too many, too few, or just right?
#
# first, too few command-line options, but we might still be saved 
# IF $elf_files has only one member
if [ "$#" -eq 0 ]       
then
    # prepending a `#` to a variable returns the variable length
    if [ ${#elf_files[@]} -gt 1 ]   # too many
    then
        # to reference the entire array we specify using `@` as the index
        # a bare `${elf_files}` will only return the first item
        echo "ERROR: no unique file name specified, possibilities are: ${elf_files[@]}"
        exit 1
    elif [ ${#elf_files[@]} -eq 1 ]
    then
        program=${elf_files[0]} # works best if we do this in the right order
        echo "Warn: no file specified, defaulting to only valid available file: ${program}"
    elif [ ${#elf_files[@]} -eq 0 ]
    then
        echo "ERROR: No appropriate files available"
        exit 1
    else 
        echo "ERROR: how did we even get here"
        exit 1
    fi
elif [ "$#" -gt 1 ]     # too many!
then
    echo "ERROR: Too many arguments provided: $#"
    exit 1
# `$1` refers to the first command line option
elif [[ "$#" -eq 1 && $1 = *.elf ]] # just right! and the right format too!
then
    program=$1
elif [[ $1 != *.elf ]]
then
    echo "ERROR: file must be an .elf file"
    exit 1
else
    echo "ERROR: how did we even get here"
    exit 1
fi

if [ ! -f "$program" ]
then
    echo "ERROR: File does not exist"
    exit 1
# the `##` will delete the longest matching part and return the rest

elif [ "${program##*.}" != "elf" ]      # see Shell Scripting page 114
then
    echo "ERROR: File must be in .elf format"
    exit 1
fi

here=${PWD##/*/}
if [ -n "$PICO_BOARD" ]
then 
    case "$PICO_BOARD" in
        "pico"|"pico_w")    target="rp2040"; echo "Info: upload target is $target";;
        "pico2"|"pico2_w")  target="rp2350"; echo "Info: upload target is $target";;
        *)                  echo "ERROR: unknown board type!"; exit 1;;
    esac
elif [ $here = "build" ]
then
    target="rp2040"
elif [ $here = "build_2" || $here = "build2" ]
then
    target="rp2350"
else
    echo "ERROR: can't determine pico board type"
    exit 1
fi
echo "Info: location is $here, trying to upload $program to $target"
# the .elf needs to be the first argument
# sudo openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000" -c "program $1 verify reset exit"
# sudo openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "program $1 verify reset exit"
sudo openocd -f interface/cmsis-dap.cfg -f target/$target.cfg -c "adapter speed 5000" -c "program $program verify reset exit"
