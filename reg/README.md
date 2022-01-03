cFp_HelloKale Regression
=========================

A regression is a suite of functional verification tests executed against a project or a set of IP cores. 
Such a regression is typically called by a *Jenkins* server during the software development process.

The regression of the cFp_HelloKale project is executed by calling a shell script `./run_main.sh` which performs a suite of HLS/C-SIMULATION runs followed by a suite of RTL/CO-SIMULATION runs on the HLS-based IP cores.

**Warning**
The above scripts must be executed from the cFp_HelloKale root directory which must be defined ahead 
with the `$cFpHelloKaleRootDir` variable or passed as an argument to `run_main.sh`. Any other environment variable must also be sourced beforehand.

**Example** 
The main cFp_HelloKale regression script can be called as follows:

```
    $ source /tools/Xilinx/Vivado/2017.4/settings64.sh

    $ echo $PWD
    $ export cFpHelloKaleRootDir=$PWD

    $ ./reg/run_main.sh
    or
    $ ./reg/run_main.sh $cFpHelloKaleRootDir
```

**About the regression script**  
The `run_main.sh` script is the main entry point for running a regression. It sets all necessary 
cF-environment variables, like a typical `env/setenv.sh` script would do. Next, if other scripts 
are to be run, they are expected to be called by `run_main.sh`.


