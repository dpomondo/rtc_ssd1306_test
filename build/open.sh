#! /bin/sh

# the .elf needs to be the first argument
sudo openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000" -c "program $1 verify reset exit"
