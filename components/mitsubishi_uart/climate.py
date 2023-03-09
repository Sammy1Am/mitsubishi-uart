import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart
from esphome.const import (
    CONF_ID,
    CONF_OUTPUT,
    CONF_UPDATE_INTERVAL,
    CONF_MODE,
    CONF_FAN_MODE,
    CONF_SWING_MODE,
)
from esphome.core import coroutine

AUTO_LOAD = ["climate"]
DEPENDENCIES = ["uart"]

CONF_HP_UART = "hp_uart"
CONF_SUPPORTS = "supports"

DEFAULT_CLIMATE_MODES = ["HEAT", "DRY", "COOL", "FAN_ONLY", "HEAT_COOL"]
DEFAULT_FAN_MODES = ["AUTO", "QUIET", "LOW", "MIDDLE", "MEDIUM", "HIGH"]
# DEFAULT_SWING_MODES = ["OFF", "VERTICAL"]

DEFAULT_POLLING_INTERVAL = "5s"

mitsubishi_uart_ns = cg.esphome_ns.namespace('mitsubishi_uart')
MitsubishiUART = mitsubishi_uart_ns.class_('MitsubishiUART', climate.Climate, cg.PollingComponent)

CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend({
    cv.GenerateID(CONF_ID): cv.declare_id(MitsubishiUART),
    cv.Required(CONF_HP_UART): cv.use_id(uart.UARTComponent), #TODO Set a default?
    # Set default, but allow override of supports.
    cv.Optional(CONF_SUPPORTS, default={}): cv.Schema(
        {
            cv.Optional(CONF_MODE, default=DEFAULT_CLIMATE_MODES):
                cv.ensure_list(climate.validate_climate_mode),
            cv.Optional(CONF_FAN_MODE, default=DEFAULT_FAN_MODES):
                cv.ensure_list(climate.validate_climate_fan_mode),
            # cv.Optional(CONF_SWING_MODE, default=DEFAULT_SWING_MODES):
            #     cv.ensure_list(climate.validate_climate_swing_mode),
        }
    )
}).extend(cv.polling_component_schema(DEFAULT_POLLING_INTERVAL))

@coroutine
async def to_code(config):
    hp_uart_component = await cg.get_variable(config[CONF_HP_UART])
    var = cg.new_Pvariable(config[CONF_ID], hp_uart_component)

    supports = config[CONF_SUPPORTS]
    traits = var.config_traits()

    for mode in supports[CONF_MODE]:
        if mode == "OFF":
            continue
        cg.add(traits.add_supported_mode(climate.CLIMATE_MODES[mode]))

    for mode in supports[CONF_FAN_MODE]:
        cg.add(traits.add_supported_fan_mode(climate.CLIMATE_FAN_MODES[mode]))

    # for mode in supports[CONF_SWING_MODE]:
    #     cg.add(traits.add_supported_swing_mode(
    #         climate.CLIMATE_SWING_MODES[mode]
    #     ))

    await cg.register_component(var, config)
    await climate.register_climate(var, config)