import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    UNIT_CELSIUS,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
)
from .. import (
    CONF_MUART_ID,
    MUART_COMPONENT_SCHEMA,
    mitsubishi_uart_ns,
    MUARTComponent,
)

DEPENDENCIES = ["mitsubishi_uart"]

INTERNAL_TEMPERATURE = "internal_temperature"
THERMOSTAT_TEMPERATURE = "thermostat_temperature"
LOOP_STATUS = "loop_status"
STAGE = "stage"
COMPRESSOR_FREQUENCY = "compressor_frequency"

MUARTSensor = mitsubishi_uart_ns.class_("MUARTSensor", MUARTComponent)

MUARTSENSOR_SCHEMA = cv.Schema({cv.GenerateID(): cv.declare_id(MUARTSensor)})

SENSORS = {
    INTERNAL_TEMPERATURE: (
        "Internal Temperature",
        sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ).extend(MUARTSENSOR_SCHEMA),
    ),
    THERMOSTAT_TEMPERATURE: (
        "Thermostat Temperature",
        sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ).extend(MUARTSENSOR_SCHEMA),
    ),
    LOOP_STATUS: ("Loop Status", sensor.sensor_schema().extend(MUARTSENSOR_SCHEMA)),
    STAGE: ("Stage", sensor.sensor_schema().extend(MUARTSENSOR_SCHEMA)),
    COMPRESSOR_FREQUENCY: (
        "Compressor Frequency",
        sensor.sensor_schema().extend(MUARTSENSOR_SCHEMA),
    ),
}

CONFIG_SCHEMA = MUART_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(sensor_type, default={"name": sensor_name}): sensor_schema
        for sensor_type, (sensor_name, sensor_schema) in SENSORS.items()
    }
)


async def to_code(config):
    muart = await cg.get_variable(config[CONF_MUART_ID])

    for sensor_type in SENSORS:
        if sensor_type in config:
            s_conf = config[sensor_type]
            var = cg.new_Pvariable(s_conf[CONF_ID])
            await sensor.register_sensor(var, s_conf)
            cg.add(getattr(muart, f"set_sensor_{sensor_type}")(var))
            cg.add(var.set_parent(muart))
