Python Test Scripts
==========================
**Python3 scripts for interacting with the role of the cFp_BringUp project.**


### About Python *virtualenv*
**To ease the execution of the local python test scripts, the use of a *virtualenv* 
or *venv* is recommended.** 


#### How To Install the Python Virtualenv Tool
**[Info]** - We recommend to use python **v3.6** or higher. 

```
    $ sudo python3.6 -m ensurepip
    $ sudo pip3 install virtualenv
```

**[Info]** - On some installations, you may need to update the access permissions of 
the newly created files and directories with the following command:
```
    $ chmod 755 -R /usr/lib/python3.<YourVersion>/site-packages/virtualenv*
```

#### How To Create your Virtual Environment
 
```
    $ cd HOST/py
    $ python3 -m venv venv
```

#### How To Activate this Environment
```
    $ source venv/bin/activate
    $ pip3 install -r requirements.txt
```




