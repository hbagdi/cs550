# pipe_driver

Compiling:
		$make
Loading module:
		$sudo insmod ./pipe_drv.ko size=4
	*if size argument is not given, default queue size will be set to 5
	*loading this module will create a device  /dev/pipe-file . This name is hardcoded in driver. If needed, make it an arguemnet to insmod ( like size)
	*This file by default will not have rw access to normal users so do
		$ sudo chmod a+rw /dev/pipe-file
	* I think this can be made in driver itself. check about it.

Writing to pipe:
	Assignment requires a user application to write but to test I just used	
		$echo hello1 > /dev/pipe-file
	This will write 7 bytes to the file. It ends the str with \n instead of \0 so I do manualy replace it to \0 in the driver. Unisng our own program will not need this. 
	check kernel messages using 
		$dmesg
To read from pipe:
	Assignment requires a user application to read but to test I just used
		$head -c 7 /dev/pipe-file
	*this will read 7 bytes from file. Dont use cat for this because it tries to read a large amount of data and when driver returns 1 str it reads again till all the strings in queue are read. However using this will not break our code.

Unloading module:
		$sudo rmmod pipe_drv

