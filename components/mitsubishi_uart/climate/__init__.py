import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.const import (
    CONF_ID,
    CONF_MODE,
    CONF_FAN_MODE,
)
from esphome.core import coroutine
from .. import (
    CONF_MUART_ID,
    MUART_COMPONENT_SCHEMA,
    mitsubishi_uart_ns,
    MUARTComponent,
    MitsubishiUART,
)

AUTO_LOAD = ["climate"]
DEPENDENCIES = ["mitsubishi_uart"]

CONF_SUPPORTS = "supports"

DEFAULT_CLIMATE_MODES = ["HEAT", "DRY", "COOL", "FAN_ONLY", "HEAT_COOL"]
DEFAULT_FAN_MODES = ["AUTO", "QUIET", "LOW", "MIDDLE", "MEDIUM", "HIGH"]

MUARTClimate = mitsubishi_uart_ns.class_("MUARTClimate", MUARTComponent)

CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend(MUART_COMPONENT_SCHEMA).extend(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(MUARTClimate),
        # Set default, but allow override of supports.
        cv.Optional(CONF_SUPPORTS, default={}): cv.Schema(
            {
                cv.Optional(CONF_MODE, default=DEFAULT_CLIMATE_MODES): cv.ensure_list(
                    climate.validate_climate_mode
                ),
                cv.Optional(CONF_FAN_MODE, default=DEFAULT_FAN_MODES): cv.ensure_list(
                    climate.validate_climate_fan_mode
                ),
            }
        ),
    }
)


async def to_code(config):
    muart = await cg.get_variable(config[CONF_MUART_ID])
    var = cg.new_Pvariable(config[CONF_ID])

    supports = config[CONF_SUPPORTS]
    traits = var.config_traits()

    for mode in supports[CONF_MODE]:
        if mode == "OFF":
            continue
        cg.add(traits.add_supported_mode(climate.CLIMATE_MODES[mode]))

    for mode in supports[CONF_FAN_MODE]:
        cg.add(traits.add_supported_fan_mode(climate.CLIMATE_FAN_MODES[mode]))

    # await cg.register_component(var, config)
    await climate.register_climate(var, config)
    cg.add(getattr(muart, "set_climate")(var))
    cg.add(var.set_parent(muart))
