Icinga Nagios Traffic Light

GitHub: https://github.com/edvler/Icinga_Nagios_Traffic_Light
Web-Blog: http://www.edvler-blog.de/icinga_nagios_traffic_light_ampel

Author: Matthias Maderer
Date: 29.01.2013

This is a project for icinga (https://www.icinga.org/) or nagios (http://www.nagios.org/).
Both are monitoring systems for IT infrastructures. Almost everything can be monitored, from a simple host to a complex services.
But at the end it's realy simple. In icinga an nagios are 3 major possible states avaliabe.
Each service or host could be in one of the following states:

- good (green)
- warning (orange)
- red (critical)

This is where my traffic light comes in.
It is used to bring the actual overall state of the monitoring system to a traffic light.

Four traffic lights available. One and two have 3 signs, three and four have 2 signs.

The traffic light is controlled over network! It's possible to configure the network settings over webinterface.
The default IP for the Webinterface is 192.168.0.111.

It's possible to send commands via UDP or per Webrequest over a browser or wget.

For details how to send commands please look at the source code file Arduino_code\Icinga_Nagios_Traffic_Light\Icinga_Nagios_Traffic_Light.pde

Greets
EDVler