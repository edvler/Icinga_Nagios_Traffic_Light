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

Four traffic lights available. One and two have 3 signs, three and four have 2 signs.

How it work's:
1. On the Icinga server or the Nagios server is the script control_traffic_light.pl called with crontab.
2. The script sends messages with the UDP protocol to the Arduino and changes the traffic light corresponding to the overall status of icinga or nagios.
3. It is also possible to turn on and off the traffic lights from the console:
   - control_traffic_light.pl 1redOn       #to turn the red LED of the traffic light one on.
   - control_traffic_light.pl 1redOff      #to turn the red LED of the traffic light one off.
 
   - control_traffic_light.pl 2yellowOn    #to turn the yellow LED of traffic the light TWO on.
   - control_traffic_light.pl 2yellowOff   #to turn the yellow LED of traffic the light TWO off.
  
   - control_traffic_light.pl 2greenOn     #to turn the green LED of traffic the light TWO on.
   - control_traffic_light.pl 1greenOn     #to turn the grenn LED of traffic the light ONE off.

   - control_traffic_light.pl 3greenOn     #to turn the green LED of traffic the light TWO on.
   - control_traffic_light.pl 4greenOn     #to turn the grenn LED of traffic the light ONE off.

... and so on.
   
   - control_traffic_light.pl allOn        #to turn all LED's of the traffic light on
   - control_traffic_light.pl allOff       #to turn all LED's of the traffic light on
   
For more details please see the well documented source code.

More information is available on www.edvler-blog.de.