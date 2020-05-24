# ElevatorSimulation


1.first, you need to run ElevatorSimulation.sln

2.You have two modes(Normal,HC5 Test) to choose. You need to press 1 or 2 to choose one.

3.If you choose Normal mode, program will never end, it will show you elevators running processing and buttons panel status.

4.If you choose Normal mode, program will stop at 500 seconds, the timing is started when the scheduler assigns tasks(not running time).

5.All two mode will show parameter like Run time, Responsed Task Count,Arrived Target Count,AWT(average waiting time),AI_INT(average running interval time).

6.Only HC5 Test will show HC5(five minute handling capacity) parameter.

7.Considering the observability and saving time, I reduced the time involved in all time operations by 10 times.
(When the parameter is displayed, I restored it by expanding 10 times)

8.There are three output file. 
log.txt is mainly to record every important operation.(most important)
test_HC5_log.txt only record opreatorion about HC5.
time.txt is to record elevator operator on each timepoint.
You should delete all of them befor you restart the program.

9.test_case_out folder is used to save some sample output.

---
For more details, you could review ***ElevatorSimulation_report.pdf*** file

<img src="https://github.com/mmyduckx/ElevatorSimulation/blob/master/readme.md" width=600 height=800 />
