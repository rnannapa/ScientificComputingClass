Contents of m7378.tar:
  README-build.txt
  libm6378.c
  m6378.h
  posn-unnamed-sem.h
  sys5-unnamed-sem.h
  m6378run
  m6378-example1.c

Create the share library 
> gcc -shared -O -fPIC -o libm6378.so libm6378.c -lpthread -lrt

Until you decide if you want to install this permanently, I suggest 
you leave the following files

  libm6378.so, m6378.h and m6378run

here in this directory, and set the following environment variables

  M6378_HOME=/home/rajgopal/mpi
  export PATH="$PATH":$M6378_HOME
  export C_INCLUDE_PATH=$M6378_HOME
  export LIBRARY_PATH=$M6378_HOME
  export LD_LIBRARY_PATH=$M6378_HOME

You might want to put these in your .bashrc file. Otherwise you'll
have to do this in every new bash shell.

Now you can easily compile and run m6378 programs

> gcc m6378-example1.c -lm6378
> m6378run 4 a.out


I'll show you how to make the install permanent later.
