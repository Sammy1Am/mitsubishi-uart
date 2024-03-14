import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart, sensor, select, switch
from esphome.core import CORE
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    CONF_SUPPORTED_MODES,
    CONF_CUSTOM_FAN_MODES,
    CONF_SUPPORTED_FAN_MODES,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_FREQUENCY,
    ENTITY_CATEGORY_CONFIG,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_HERTZ,
)
from esphome.core import coroutine

AUTO_LOAD = ["climate", "select", "sensor", "switch"]
DEPENDENCIES = ["uart", "climate", "sensor", "select", "switch"]

CONF_HP_UART = "heatpump_uart"
CONF_TS_UART = "thermostat_uart"

CONF_SENSORS = "sensors"
CONF_SENSORS_CURRENT_TEMP = "current_temperature"
CONF_SENSORS_THERMOSTAT_TEMP = "thermostat_temperature"
CONF_SENSORS_COMPRESSOR_FREQUENCY = "compressor_frequency"

CONF_SELECTS = "selects"
CONF_TEMPERATURE_SOURCE_SELECT = "temperature_source_select" # This is to create a Select object for selecting a source
CONF_VANE_POSITION_SELECT = "vane_position_select"
CONF_HORIZONTAL_VANE_POSITION_SELECT = "horizontal_vane_position_select"

CONF_TEMPERATURE_SOURCES = "temperature_sources" # This is for specifying additional sources

CONF_ACTIVE_MODE_SWITCH = "active_mode_switch"

DEFAULT_POLLING_INTERVAL = "5s"

mitsubishi_uart_ns = cg.esphome_ns.namespace("mitsubishi_uart")
MitsubishiUART = mitsubishi_uart_ns.class_("MitsubishiUART", cg.PollingComponent, climate.Climate)

TemperatureSourceSelect = mitsubishi_uart_ns.class_("TemperatureSourceSelect", select.Select)
VanePositionSelect = mitsubishi_uart_ns.class_("VanePositionSelect", select.Select)
HorizontalVanePositionSelect = mitsubishi_uart_ns.class_("HorizontalVanePositionSelect", select.Select)

ActiveModeSwitch = mitsubishi_uart_ns.class_("ActiveModeSwitch", switch.Switch, cg.Component)

DEFAULT_CLIMATE_MODES = ["OFF", "HEAT", "DRY", "COOL", "FAN_ONLY", "HEAT_COOL"]
DEFAULT_FAN_MODES = ["AUTO", "QUIET", "LOW", "MEDIUM", "HIGH"]
CUSTOM_FAN_MODES = {
    "VERYHIGH": mitsubishi_uart_ns.FAN_MODE_VERYHIGH
}
VANE_POSITIONS = ["Auto","1","2","3","4","5","Swing"]
HORIZONTAL_VANE_POSITIONS = ["<<","<","|",">",">>","<>","Swing"]

INTERNAL_TEMPERATURE_SOURCE_OPTIONS = [mitsubishi_uart_ns.TEMPERATURE_SOURCE_INTERNAL] # These will always be available

validate_custom_fan_modes = cv.enum(CUSTOM_FAN_MODES, upper=True)

BASE_SCHEMA = cv.polling_component_schema(DEFAULT_POLLING_INTERVAL).extend(climate.CLIMATE_SCHEMA).extend({
    cv.GenerateID(CONF_ID): cv.declare_id(MitsubishiUART),
    cv.Required(CONF_HP_UART): cv.use_id(uart.UARTComponent),
    cv.Optional(CONF_TS_UART): cv.use_id(uart.UARTComponent),
    cv.Optional(CONF_NAME, default="Climate") : cv.string,

    cv.Optional(CONF_SUPPORTED_MODES, default=DEFAULT_CLIMATE_MODES) : cv.ensure_list(climate.validate_climate_mode),
    cv.Optional(CONF_SUPPORTED_FAN_MODES, default=DEFAULT_FAN_MODES): cv.ensure_list(climate.validate_climate_fan_mode),
    cv.Optional(CONF_CUSTOM_FAN_MODES, default=["VERYHIGH"]) : cv.ensure_list(validate_custom_fan_modes),
    cv.Optional(CONF_TEMPERATURE_SOURCES, default=[]) : cv.ensure_list(cv.use_id(sensor.Sensor)),
    cv.Optional(CONF_ACTIVE_MODE_SWITCH, default={"name":"Active Mode"}) : switch.switch_schema(
        ActiveModeSwitch,
        entity_category=ENTITY_CATEGORY_CONFIG,
        default_restore_mode="RESTORE_DEFAULT_ON",
        icon="mdi:upload-network")
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
    ),
    CONF_SENSORS_THERMOSTAT_TEMP: (
        "Thermostat Temperature",
        sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=1,
        )
    ),
    CONF_SENSORS_COMPRESSOR_FREQUENCY: (
        "Compressor Frequency",
        sensor.sensor_schema(
            unit_of_measurement=UNIT_HERTZ,
            device_class=DEVICE_CLASS_FREQUENCY,
            state_class=STATE_CLASS_MEASUREMENT
        )
    )
}

SENSORS_SCHEMA = cv.All({
    cv.Optional(sensor_designator, default={"name": f"{sensor_name}"}): sensor_schema
    for sensor_designator, (sensor_name, sensor_schema) in SENSORS.items()
})

SELECTS = {
    CONF_TEMPERATURE_SOURCE_SELECT: (
        "Temperature Source",
        select.select_schema(
            TemperatureSourceSelect,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:thermometer-check"
        ),
        INTERNAL_TEMPERATURE_SOURCE_OPTIONS
    ),
    CONF_VANE_POSITION_SELECT: (
        "Vane Position",
        select.select_schema(
            VanePositionSelect,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:arrow-expand-vertical"
        ),
        VANE_POSITIONS
    ),
    CONF_HORIZONTAL_VANE_POSITION_SELECT: (
        "Horizontal Vane Position",
        select.select_schema(
            HorizontalVanePositionSelect,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:arrow-expand-horizontal"
        ),
        HORIZONTAL_VANE_POSITIONS
    )
}

SELECTS_SCHEMA = cv.All({
    cv.Optional(select_designator, default={"name": f"{select_name}"}): select_schema
    for select_designator, (select_name, select_schema, select_options) in SELECTS.items()
})


CONFIG_SCHEMA = BASE_SCHEMA.extend({
    cv.Optional(CONF_SENSORS, default={}): SENSORS_SCHEMA,
    cv.Optional(CONF_SELECTS, default={}): SELECTS_SCHEMA,
})


@coroutine
async def to_code(config):
    hp_uart_component = await cg.get_variable(config[CONF_HP_UART])
    muart_component = cg.new_Pvariable(config[CONF_ID], hp_uart_component)

    await cg.register_component(muart_component, config)
    await climate.register_climate(muart_component, config)

    # If thermostat defined
    if (CONF_TS_UART in config):
        # Register thermostat with MUART
        ts_uart_component = await cg.get_variable(config[CONF_TS_UART])
        cg.add(getattr(muart_component, f"set_thermostat_uart")(ts_uart_component))
        # Add sensor as source
        SELECTS[CONF_TEMPERATURE_SOURCE_SELECT][2].append("Thermostat")

    # Traits

    traits = muart_component.config_traits()

    if CONF_SUPPORTED_MODES in config:
        cg.add(traits.set_supported_modes(config[CONF_SUPPORTED_MODES]))

    if CONF_SUPPORTED_FAN_MODES in config:
        cg.add(traits.set_supported_fan_modes(config[CONF_SUPPORTED_FAN_MODES]))

    if CONF_CUSTOM_FAN_MODES in config:
        cg.add(traits.set_supported_custom_fan_modes(config[CONF_CUSTOM_FAN_MODES]))

    # Sensors

    for sensor_designator in SENSORS:
        # Only add the thermostat temp if we have a TS_UART
        if (sensor_designator == CONF_SENSORS_THERMOSTAT_TEMP) and (CONF_TS_UART not in config):
            continue

        sensor_conf = config[CONF_SENSORS][sensor_designator]
        sensor_component = cg.new_Pvariable(sensor_conf[CONF_ID])
        await sensor.register_sensor(sensor_component, sensor_conf)
        cg.add(getattr(muart_component, f"set_{sensor_designator}_sensor")(sensor_component))

    ### Selects

    # Add additional configured temperature sensors to the select menu
    for ts_id in config[CONF_TEMPERATURE_SOURCES]:
        ts = await cg.get_variable(ts_id)
        SELECTS[CONF_TEMPERATURE_SOURCE_SELECT][2].append(ts.get_name())
        cg.add(getattr(ts, "add_on_state_callback")(
            # TODO: Is there anyway to do this without a raw expression?
            cg.RawExpression(
                f"[](float v){{{getattr(muart_component, 'temperature_source_report')}({ts.get_name()}, v);}}"
            )
        ))

    # Register selects
    for select_designator, (select_name, select_schema, select_options) in SELECTS.items():
        select_conf = config[CONF_SELECTS][select_designator]
        select_component = cg.new_Pvariable(select_conf[CONF_ID])
        await select.register_select(select_component, select_conf, options=select_options)
        cg.add(getattr(muart_component, f"set_{select_designator}")(select_component))
        await cg.register_parented(select_component, muart_component)

    ### Switches
    if am_switch_conf := config.get(CONF_ACTIVE_MODE_SWITCH):
        switch_component = await switch.new_switch(am_switch_conf)
        await cg.register_component(switch_component, am_switch_conf)
        await cg.register_parented(switch_component,muart_component)

