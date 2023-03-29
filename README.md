# Mitsubishi UART
First-class ESPHome component targeting Mitsubishi heat pumps via UART.

### For the impatient:
**Check out [getting started for users](https://github.com/Sammy1Am/mitsubishi-uart/wiki/Getting-Started) or [for developers](https://github.com/Sammy1Am/mitsubishi-uart/wiki) on the wiki.**

-----

Building on the work of the seminal [9SwiCago/HeatPump](https://github.com/SwiCago/HeatPump) and [geoffdavis/esphome-mitsubishiheatpump](https://github.com/geoffdavis/esphome-mitsubishiheatpump), this library aims to take advantage of more recent developments (e.g. parity support in ESPHome's software UART) to provide a more compact ESPHome component for heatpump contol.  I'm also planning to support using this along with an MHK thermostat (basically [akamali/mhk1_mqtt](https://github.com/akamali/mhk1_mqtt), but built in an ESPHome-native way, rather than via MQTT).

__Why not use use / contribute to those?__
Largely, all of the above libraries *work* fine, but each lacks a little bit of something I'm looking for:
- SwiCago/HeatPump just provides direct-control, and requires a hardware serial.
- geoffdavis/esphome-mitsubishiheatpump supports ESPHome control, but still requires hardware serial and doesn't support thermostats.
- akamali/mhk1_mqtt also requires a hardware serial, and requires everything pass through MQTT.  Under most circumstances this would be fine, but since part of the motivation for creating this project was that Mitsubishi's controllers are unreliable when the network is sketchy, I'd like to be able to take advantage of ESP-now for direct communication between multiple remote temperature sensors and the controller.  ESPHome provides easy integration for this via something like [tomrusteze/esphome-esp-now](https://github.com/tomrusteze/esphome-esp-now), and could still provide MQTT bridging if required.

tl;dr: ESPHome has enough integrations and features (that can run locally on the microcontroller without MQTT or HA), that I've been convinced it's a solid platform to build this on directly.

### Features
- Supports adding additional ESPHome sensors as remote temperature sources
- Support for software UART
- Support for connecting a thermostat / Kumo Cloud to a second UART port (MHK2 (and probably 1) supported)
- Parity with above mentioned libraries for features (pretty much there)

### Potential Future Goals
- Support for new packets and controls (check out [the wiki](https://github.com/Sammy1Am/mitsubishi-uart/wiki/Decoding-Packets) for what we know so far)
- Other mitsubishi products? (Do they make water heaters too?  I don't know.)

The orignal impetus for this project was to allow switching our central heatpump's sensed-temperature between upstairs and downstairs based on time of day, or setting up more complicated logic (e.g. preventing hot-spot rooms from getting too hot during a heating cycle).  Most of that logic will likely end up outside of this component unless it makes the most sense for it to be here.
