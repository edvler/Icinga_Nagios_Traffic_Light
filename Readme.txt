Icinga Nagios Traffic Light

This is a simple project for icinga (https://www.icinga.org/) or nagios (http://www.nagios.org/).
Both are monitoring systems for IT infrastructures. Almost everything can be monitored, from a simple host to a complex services.
But at the end it's realy simple. In Icinga an nagios are 3 major possible states avaliabe.
Each service or host could be in one of the following states:

- good (green)
- warning (orange)
- red (critical)

This is where my traffic light comes in.
It is used to bring the actual overall state of the monitoring system to a traffic light.

Two traffic lights available. One can be used for the service status and the other one can be used for hosts status.

How it work's:
1. On the Icinga server or the Nagios server is the script control_traffic_light.pl called with crontab.
2. The script sends messages with the UDP protocol to the Arduino and changes the traffic light corresponding to the overall status of icinga or nagios.
3. It is also possible to turn on and off the traffic lights from the console:
   - control_traffic_light.pl "1 redOn" to turn the red LED of traffic light one on.
   - control_traffic_light.pl "1 redOff" to turn the red LED of traffic light one off.
 
   - control_traffic_light.pl "2 yellowOn" to turn the yellow LED of traffic light TWO on.
   - control_traffic_light.pl "2 yellowOff" to turn the yellow LED of traffic light TWO off.
  
   - control_traffic_light.pl "2 greenOn" to turn the green LED of traffic light TWO on.
   - control_traffic_light.pl "1 greenOn" to turn the grenn LED of traffic light ONE off.
   
For more details please see the well documented source code.

More information is available on www.edvler-blog.de.