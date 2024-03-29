# patched-jed for cbrief

For details, CBrief macros and installation, see
https://codeberg.org/nereusx/jedc-macros

## JED - README

README file for JED.

 JED is distributed in two forms:

     jedXXX.tar.gz:
       Contains the complete source to the editor.  It does not
       contain the slang library source.  The library is available
       from ftp://space.mit.edu/pub/davis/slang.

       Building JED requires the use of a C compiler which understands
       function prototypes.  I have sucessfully built JED with `cc' on
       the ULTRIX, VMS, and IRIX operating systems.  In addition, I
       have created it using `gcc' under SunOS and Linux.  See
       src/mkfiles/README for building it under DOS, OS/2, and
       Windows systems.

     jedXXX.zip:
       Contains no source code but it does include pre-compiled
       executables for MSDOS and Windows systems.  The executable is in
       the bin directory and will work only on 386+ PCs with a DPMI
       server, such as provided by Windows (See INSTALL.pc for
       information regarding the server).  The executables were
       created using gcc for win32 and MSDOS (specifically, MINGW32
       and DJGPP).

 For installation instructions on

      Unix:
        read INSTALL.unx

      VMS:
        read INSTALL.vms

      DOS, OS/2, Win9X, NT:
        read INSTALL.pc

 Then, read INSTALL for more installation tips. For changes and new
 features, read changes.txt.

 Addition information about JED is available from
 <http://www.jedsoft.org/jed/>  The documentation for the
 slang library and interpreter is available from <http://www.jedsoft.org/slang/>
 and <http://www.jedsoft.org/slang/>.  Useful newsgroups include
 comp.editors and alt.lang.s-lang.  A mailing list is also available.
 See <http://www.jedsoft.org/jed/mailinglists.html> for more information
 about it.
