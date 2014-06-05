#!/usr/bin/python

import os

# traverse root directory, and list directories as dirs and files as files
for root, dirs, files in os.walk("."):
    path = root.split('/')
    dname = os.path.basename(root)       
    if(False == os.path.isdir(dname) or dname.startswith("test") == False):
	    continue
    print len(path)*'---', dname
    command = "./run_test.sh > /dev/null"

    command = "cd " + dname + ";" + command
    os.system(command)
    #os.system("cd -")

