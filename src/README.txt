  ________       __ 
 / ___/ _ )___  / /_
/ /__/ _  / _ \/ __/
\___/____/\___/\__/
R E A D M E
================================

PURPOSE
To run tasks on Windows at specific dates/times

HOW TO USE
Launch "cbot_default.cmd" file to run the default .cbt file.
To customize it to your preferences, you may edit the config\test.cfg file or create your own in the same format. 
Then just add runnable files (.bat,.cmd,.exe,...) to the scripts\ directory and configure them in your config file. 

DIRECTORIES
cbt\: Holds all cbt files which describe what tasks to run and when to run them.
logs\: Holds a main log and an error log. cbot uses these to figure out what tasks have run already.
scripts\: Holds runnable files for execution. It isn't required that you put your batch files in here, but it's recommended.

The cbt format is described below:
=========================================================
# This is a comment, indicated by the pound sign at the start of the line
# You can put as many of these as you want in your config file without causing issues

# This is a "task". It has three parameters. The first is the "time rule", the second is "script path", and the third specifies if the tasks has logging enabled or not. 1 = enabled. 0 = disabled.
@yyyy-04-01 08.00.00; scripts\notify.cmd; 1

# You can add as many tasks as you want. The following will run the same script quarterly.
@yyyy-04-01 08.00.00; scripts\notify.cmd; 1
@yyyy-07-01 08.00.00; scripts\notify.cmd; 1
@yyyy-10-01 08.00.00; scripts\notify.cmd; 1
@yyyy-01-01 08.00.00; scripts\notify.cmd; 1

==========================================================
The time rule format is yyyy-MM-dd HH.mm.ss and cannot be altered yet. Hour component is 01-24. 
Fill out the components for your purposes.

Here are some example time rules:

@2016-12-01 07:30:15		: Run a single time at this exact date/time
@2016-MM-15 08.00.00		: Run every month in year 2016 on the 15th day at 8 o'clock
@yyyy-MM-dd HH.00.00		: Run every hour on the hour
