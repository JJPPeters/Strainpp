# Create Installer

## About
These scripts are intended to make the actual Strain++ installer. I do not pretend that this is a good way to do it, it is just what I cobbled together.

The core of the scripts is using InnoSetup, with the rest just copying around files.

## Usage

The main (and only) script is `make_installer.py`. This handles everything.

To adapt to different computers, there are several file paths at the top of the script that should be set. Different versions of various libraries may also require modifying the dependency lists - it is all very manual.

There is some finesse for using the automatic GitHub version number, as the release must be created before the build.