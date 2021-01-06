HOST / py
==========================
**Python scripts for interacting with the role of the cFp_Monolithic project.**

## ABOUT
This directory contains a set of _Python_ scripts for a host CPU to interact with the role of the 
_cFp_Monolithic_ project.

## Environment requirements
To ease the execution of the local python test scripts, we recommend _Python v3.6_ or 
higher, and the use of a virtual environment.  

## Step-1: How to create a Virtual Environment
This section is an extract from the [Python Packaging User Guide](https://packaging.python.org/guides/installing-using-pip-and-virtual-environments/).
It summarises the installation procedure of packages using `pip` and the creation of a virtual 
environment manager (venv) for Python 3.

Skip this part and goto <[Step-2](#step-2-how-to-python)> if you have already created a 
_py/venv_ directory.

#### Step-1.1: How to install pip 
_pip_ is the reference Python package manager. It’s used to install and update packages. 

Ensure that your user installation of `pip` is up-to-date.
```
    $ python3 -m pip install --user --upgrade pip
```
Afterwards, you should have the newest pip installed in your user site:
```
    $ python3 -m pip --version
    pip 20.3.3 from $HOME/.local/lib/python3.6/site-packages/pip (python 3.6)
```

#### Step-1.2: How to create a virtual environment
Make sure you are in the _HOST/py_ directory and run the following command.
```
    $ cd HOST/py
    $ python3 -m venv venv
```

#### Step-1.3: How to activate a virtual environment
Before you can start installing or using packages in your virtual environment you need to activate
it. Activating a virtual environment will put the virtual environment-specific python and pip 
executables into your shell’s PATH.
```
    $ source ./venv/bin/activate
```
You can confirm you’re in your virtual environment if your python interpreter points to your `venv`
directory.
```
    $ which python3
    python3 is $PWD/venv/bin/python3 
```

#### Step-1.4: How to install required packages
Now that you’re in the virtual environment of this project, let's install the dependent packages as
listed in the `requirements.txt` file. 
```
    $ pip3 install -r requirements.txt
``` 


## Step-2: How To Python 
In the remaining of part of this document, we assume (and we recommend) that you  are using 
_python3.6_ or above.

##### Step-2.1: Activate the virtual environment
Change to _HOST/py_ directory and activate the virtual environment of this project by running the 
following command.
```
    $ cd HOST/py
    $ source venv/bin/activate
```
If the _py/venv_ directory does not exist, you first need to create it by following the procedure 
described in <[Step-1](#step-1-how-to-create-a-virtual-environment)>.

##### Step-2.2: To execute a UDP or TCP transmit test
Enter one of the following commands from the _HOST/py_ directory. 
```
    $ python3 tc_UdpSend.py --help
    $ python3 tc_UdpSend.py --fpga_ipv4 <11.22.33.44> --inst_id <42> --user_name <xyz> --user_passwd <xyz's_pw> -lc 250
 
    $ python3 tc_TcpSend.py --help
    $ python3 tc_TcpSend.py --fpga_ipv4 <11.22.33.44> --inst_id <42> --user_name <xyz> --user_passwd <xyz's_pw> -lc 250
```

##### Step-2.3: To execute a UDP or TCP receive test
Enter one of the following commands from the _HOST/py_ directory. 
```
    $ python3 tc_UdpRecv.py --help
    $ python3 tc_UdpRecv.py --fpga_ipv4 <11.22.33.44> --inst_id <42> --user_name <xyz> --user_passwd <xyz's_pw> -lc 250
 
    $ python3 tc_TcpRecv.py --help
    $ python3 tc_TcpRecv.py --fpga_ipv4 <11.22.33.44> --inst_id <42> --user_name <xyz> --user_passwd <xyz's_pw> -lc 250
```

##### Step-2.4: To execute a UDP or TCP echo test
Enter one of the following commands from the _HOST/py_ directory. 
```
    $ python3 tc_UdpEcho.py --help
    $ python3 tc_UdpEcho.py --fpga_ipv4 <11.22.33.44> --inst_id <42> --user_name <xyz> --user_passwd <xyz's_pw> -lc 250 -v
 
    $ python3 tc_TcpEcho.py --help
    $ python3 tc_TcpEcho.py --fpga_ipv4 <11.22.33.44> --inst_id <42> --user_name <xyz> --user_passwd <xyz's_pw> -lc 250 -v
```

