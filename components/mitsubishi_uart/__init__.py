import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart, sensor
from esphome.core import CORE
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    CONF_SUPPORTED_MODES,
    CONF_CUSTOM_FAN_MODES,
    CONF_SUPPORTED_FAN_MODES,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)
from esphome.core import coroutine

AUTO_LOAD = ["climate", "select", "sensor"]
DEPENDENCIES = ["uart", "climate", "sensor"]

CONF_HP_UART = "heatpump_uart"

CONF_SENSORS = "sensors"
CONF_SENSORS_CURRENT_TEMP = "current_temperature"

DEFAULT_POLLING_INTERVAL = "5s"

mitsubishi_uart_ns = cg.esphome_ns.namespace("mitsubishi_uart")
MitsubishiUART = mitsubishi_uart_ns.class_("MitsubishiUART", cg.PollingComponent, climate.Climate)

DEFAULT_CLIMATE_MODES = ["HEAT", "DRY", "COOL", "FAN_ONLY", "HEAT_COOL"]
DEFAULT_FAN_MODES = ["AUTO", "LOW", "MEDIUM", "HIGH"]
CUSTOM_FAN_MODES = {
    "QUIET": mitsubishi_uart_ns.FAN_MODE_QUIET,
    "VERYHIGH": mitsubishi_uart_ns.FAN_MODE_VERYHIGH
}

validate_custom_fan_modes = cv.enum(CUSTOM_FAN_MODES, upper=True)

BASE_SCHEMA = cv.polling_component_schema(DEFAULT_POLLING_INTERVAL).extend(climate.CLIMATE_SCHEMA).extend({
    cv.GenerateID(CONF_ID): cv.declare_id(MitsubishiUART),
    cv.Required(CONF_HP_UART): cv.use_id(uart.UARTComponent),
    cv.Optional(CONF_NAME, default="Climate") : cv.string,

    cv.Optional(CONF_SUPPORTED_MODES, default=DEFAULT_CLIMATE_MODES) : cv.ensure_list(climate.validate_climate_mode),
    cv.Optional(CONF_SUPPORTED_FAN_MODES, default=DEFAULT_FAN_MODES): cv.ensure_list(climate.validate_climate_fan_mode),
    cv.Optional(CONF_CUSTOM_FAN_MODES, default=["QUIET","VERYHIGH"]) : cv.ensure_list(validate_custom_fan_modes),
    })

SENSORS = {
    CONF_SENSORS_CURRENT_TEMP: (
        "Current Temperature",
        sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=1,
        )
    )
}

SENSORS_SCHEMA = cv.All({
    cv.Optional(sensor_designator, default={"name": f"{sensor_name}"}): sensor_schema
    for sensor_designator, (sensor_name, sensor_schema) in SENSORS.items()
})

CONFIG_SCHEMA = BASE_SCHEMA.extend({
    cv.Optional(CONF_SENSORS, default={}): SENSORS_SCHEMA
})


@coroutine
async def to_code(config):
    hp_uart_component = await cg.get_variable(config[CONF_HP_UART])
    muart_component = cg.new_Pvariable(config[CONF_ID], hp_uart_component)

    await cg.register_component(muart_component, config)
    await climate.register_climate(muart_component, config)

    traits = muart_component.config_traits()

    if CONF_SUPPORTED_MODES in config:
        cg.add(traits.set_supported_modes(config[CONF_SUPPORTED_MODES]))

    if CONF_SUPPORTED_FAN_MODES in config:
        cg.add(traits.set_supported_fan_modes(config[CONF_SUPPORTED_FAN_MODES]))

    if CONF_CUSTOM_FAN_MODES in config:
        cg.add(traits.set_supported_custom_fan_modes(config[CONF_CUSTOM_FAN_MODES]))

    for sensor_designator in SENSORS:
        sensor_conf = config[CONF_SENSORS][sensor_designator]
        sensor_component = cg.new_Pvariable(sensor_conf[CONF_ID])
        await sensor.register_sensor(sensor_component, sensor_conf)
        cg.add(getattr(muart_component, f"set_{sensor_designator}_sensor")(sensor_component))
