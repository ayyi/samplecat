[![Build status](https://gitlab.com/ayyi.org/samplecat/badges/master/pipeline.svg)](https://gitlab.com/ayyi.org/samplecat)

Samplecat
=========

http://ayyi.github.io/samplecat/

SampleCat is a program for cataloguing and auditioning audio samples.

SampleCat is available under the GNU General Public License and runs mainly on
GNU/Linux systems. It is written in C and uses the GTK graphics library. 
MySql and Sqlite can be used for the database.

Currently, most basic functionality is in place and working.


Building
--------

If you are not a developer it is recommended to use the latest tarball.
To build from tarball:
```
./configure
make
```

If you are a developer you will want to use the git repository directly.
```
./autogen.sh
./configure
make
```
Each time you do a git pull you will also need to update the submodules:
```
  git submodule update --remote --merge --recursive
  make clean
```

See the file INSTALL more more details.


Gtk-2 EOL
---------

If your system no longer supports Gtk-2, you can try the `gtk-4` branch of Samplecat. It is 'working' but very rough. The port to Gtk-4 is proving troublesome and is not as straightforward as expected.


Usage
-----

* import files or directories using drag and drop.
* if a file is currently mounted, an icon will be shown in the main table. Select Update on the context menu to refresh its status.
* set tags by 1) selecting Edit Tags on the context menu, 2) double clicking the tag area, 3) enter text in the Edit box at the top
  of the window and hit the "Set Category" button.
* to move a file in the filesystem, drag it from the main list to a directory in the directory tree on the left.
* to rename a file in the filesystem, select it, then click on the name.
* auditioning is started by clicking in the waveform view. Jack ports are not created until auditioning is started for the first time.
* import files into your audio editor using dragndrop.
* apply colour coding by dragging from the colour bar. Right click on the colour bar to change the colour palette.
* middle-click on a waveform to set the cursor position.


Auditioning
-----------

3 players are supported, each of which has pro's and con's.

* jack - built in player. See jack_autoconnect in the config file.
* ayyi - system wide [auditioning service](https://gitlab.com/ayyi.org/auditioner).
* cli  - use external command line player if available: afplay, gst-launch, totem-audio-preview

Set auditioner=OPTION in the config file.

If using the jack player, you can use the jack_autoconnect config option to define which jack port to connect to.
The default value is "system:playback_". Set it to "DISABLED" if you dont want auto-connect.


Config
------

You can manually edit the config file
```
~/.config/samplecat/samplecat
```
The config is updated by the application when it quits.
