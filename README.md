# Mitsubishi UART
First-class ESPHome component targeting Mitsubishi heat pumps via UART.

Building on the work of the seminal [9SwiCago/HeatPump](https://github.com/SwiCago/HeatPump) and [geoffdavis/esphome-mitsubishiheatpump](https://github.com/geoffdavis/esphome-mitsubishiheatpump), this library aims to take advantage of more recent developments (e.g. parity support in ESPHome's software UART) to provide a more compact ESPHome component for heatpump contol.

### Features
(roughly in order of priority)
- Support for software UART (in progress)
- Support for connecting a thermostat / Kumo Cloud to a second UART port (planned)
- Parity with above mentioned libraries for features (planned)

### Potential Future Goals
- Support for new packets and controls
- Other mitsubishi products? (Do they make water heaters too?  I don't know.)

The orignal impetus for this project was to allow switching our central heatpump's sensed-temperature between upstairs and downstairs based on time of day, or setting up more complicated logic (e.g. preventing hot-spot rooms from getting too hot during a heating cycle).  Most of that logic will likely end up outside of this component unless it makes the most sense for it to be here.
