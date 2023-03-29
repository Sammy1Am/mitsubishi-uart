import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select, sensor
from esphome.core import CORE
from esphome.const import CONF_ID, CONF_NAME, CONF_ESPHOME
from .. import (
    CONF_MUART_ID,
    MUART_COMPONENT_SCHEMA,
    mitsubishi_uart_ns,
    MUARTComponent,
)

DEPENDENCIES = ["mitsubishi_uart"]


CONF_VANE_DIRECTION = "vane_direction"
CONF_TEMPERATURE_SOURCE = "temperature_source"
CONF_SOURCE_SENSOR_IDS = "source_sensor_ids"

SELECTS = {
    CONF_VANE_DIRECTION: ("Vane Direction", ["Auto", "1", "2", "3", "4", "5", "Swing"]),
    CONF_TEMPERATURE_SOURCE: (
        "Temperature Source",
        ["Thermostat", "Internal Temperature"],
    ),
}

MUARTSelect = mitsubishi_uart_ns.class_("MUARTSelect", MUARTComponent)

MUARTSELECT_SCHEMA = select.SELECT_SCHEMA.extend(cv.COMPONENT_SCHEMA).extend(
    {cv.GenerateID(): cv.declare_id(MUARTSelect)}
)

CONFIG_SCHEMA = MUART_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(
            select_type,
            default={"name": f"{CORE.name or 'mUART'} {select_name}"},
        ): MUARTSELECT_SCHEMA
        for select_type, (select_name, select_options) in SELECTS.items()
    }
).extend(
    {cv.Optional(CONF_SOURCE_SENSOR_IDS): cv.ensure_list(cv.use_id(sensor.Sensor))}
)

# TODO Allow configuration override of options (or prevent because the HP always has certain settings??)


async def to_code(config):
    muart = await cg.get_variable(config[CONF_MUART_ID])

    for sensorid in config[CONF_SOURCE_SENSOR_IDS]:
        sens = await cg.get_variable(sensorid)
        SELECTS[CONF_TEMPERATURE_SOURCE][1].append(sens.get_name())
        cg.add(getattr(muart, "add_temperature_source")(sens))
        cg.add(
            getattr(sens, "add_on_raw_state_callback")(
                cg.RawExpression(
                    "[](float s){heat_pump->report_remote_temperature(fake_temp->get_name(), s);}"
                )
            )
        )

    for select_type, (select_name, s_options) in SELECTS.items():
        if select_type in config:
            s_conf = config[select_type]
            var = cg.new_Pvariable(s_conf[CONF_ID])
            # await cg.register_component(var, s_conf)
            await select.register_select(var, s_conf, options=s_options)
            cg.add(getattr(muart, f"set_select_{select_type}")(var))
            cg.add(var.set_parent(muart))
