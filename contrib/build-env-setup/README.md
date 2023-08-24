
Showcase how to build and install
=================================

WARN: Do NOT perform any of these steps on your host system! This script
      MUST only be run on a system which is a
      just-throw-it-away-if-broken system.

Sometimes happy developers (like me) have no choice but using terribly
restricted systems where setting up tools to run even something as
trivial as configure/make/install becomes a nightmare if not impossible.
I found it to be very handy to have some independent qemu VM at hand
which lets me install whatever I need, neither with any special software
nor any annoying privileges on a host machine. Qemu runs portable and in
user mode even doesn't need any annoying permissions at all.


## Setup a minimal system in your qemu VM

This setup mainly targets debian. Nevertheless it tries to stay POSIX
compatible as far as possible. So setup a minimal install of your system
of choice and then as soon you've SSH access to a (posix) shell, you're
ready for the next step.

Still not sure which system to use? Link below provides some candidates.
HINT: Windows IMHO is a terrible choice. So stop complaining if you go
this route.

https://en.wikipedia.org/wiki/POSIX#POSIX-oriented_operating_systems


## Start VM with SSH access

Easiest way to work with your machine is via SSH. Therefore if you've
chosen to use a qemu VM, make sure you've setup and configured sshd
properly inside the VM. Then just pass args like those to qemu:

  --device e1000,netdev=n0 --netdev user,id=n0,hostfwd=tcp:127.0.0.1:2222-:22

Started this way, the SSHDaemon inside the VM is accessible from your
host via "localhost" at port "2222":

  ssh localhost -p2222


## Finalize by build and install whole project

Run the "./setup" script (which is a posix shell script btw) inside the
freshly setup system. This script does all the work. Like installing
required packages, configure and build the whole project and finally
installing it into the VM so it can be tried out right away from your
VMs shell.
BTW: The script is constructed so it can be copy-pasted into a terminal.
There is no need to transfer a file to the machine beforehand.






