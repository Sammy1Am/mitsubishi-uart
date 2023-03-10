import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import (
    CONF_ID,
)
from .. import climate

DEPENDENCIES = ["uart"]

CONF_MUART_ID = "muart_id"
CONF_VANE_DIRECTION = "vane_direction"
CONF_TEMP_SOURCE = "temp_source"

SELECTS = {
    CONF_VANE_DIRECTION: ("Vane Direction", ["Auto", "1", "2", "3", "4", "5", "Swing"]),
    # CONF_TEMP_SOURCE: (["Internal", "Thermostat"]),
}

MUARTSelect = climate.mitsubishi_uart_ns.class_(
    "MUARTSelect", select.Select, cg.Component
)

MUART_COMPONENT_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_MUART_ID): cv.use_id(climate.MitsubishiUART),
    }
)

MUARTSELECT_SCHEMA = select.SELECT_SCHEMA.extend(cv.COMPONENT_SCHEMA).extend(
    {cv.GenerateID(): cv.declare_id(MUARTSelect)}
)


CONFIG_SCHEMA = MUART_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(select_type, default={"name": select_name}): MUARTSELECT_SCHEMA
        for select_type, (select_name, select_options) in SELECTS.items()
    }
)

# TODO Allow configuration override of options (or prevent because the HP always has certain settings??)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_MUART_ID])

    for select_type, (select_name, s_options) in SELECTS.items():
        if select_type in config:
            s_conf = config[select_type]
            var = cg.new_Pvariable(s_conf[CONF_ID])
            await cg.register_component(var, s_conf)
            await select.register_select(var, s_conf, options=s_options)
            cg.add(getattr(parent, f"set_select_{select_type}")(var))
            cg.add(var.set_parent(parent))
