# lab-ses-hab
Software for the LAB-SES HAB (wip). It's really messy as of now. 

Watchdog function hasn't been tested yet, but I'm pretty sure it works.

Since the arduino will be running in a near space environment, a watchdog is needed to prevent *bit flips* and *errors*. A NASA study found that on average a HAB could get struck by a high-energy perticle every 2 hours. Since the average flight time of our HAB is predicted to be 4 hours we need to be prepared, especially with the solar cycle rising. 
